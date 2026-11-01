#include "FeedbackController.h"

FeedbackController::FeedbackController() {}

void FeedbackController::iniciar() {
    // Direccionales y freno son activos en bajo; el LED RGB es activo en alto.
    // 0xFF apaga los primeros y ledRGB(colorNull) apaga el segundo.
    if (!_pcf.begin(0xFF)) {
        Serial.println(F("PCF8574 no detectado en la dirección 0x20"));
    }
    ledRGB(lucesConf.colorNull);

    ledcSetup(_canalBuzzer, freqBuzzer, resolucion);
    ledcAttachPin(pinBuzzer, _canalBuzzer);
}

void FeedbackController::claxon() {
    ledcWriteTone(_canalBuzzer, 800);
    delay(50);
    ledcWriteTone(_canalBuzzer, 1000);
    delay(50);
    ledcWriteTone(_canalBuzzer, 1200);
    delay(50);
    ledcWriteTone(_canalBuzzer, 0);
}

void FeedbackController::claxonConfirmar() {
    ledcWriteTone(_canalBuzzer, 600);
    delay(80);
    ledcWriteTone(_canalBuzzer, 900);
    delay(80);
    ledcWriteTone(_canalBuzzer, 0);
}

void FeedbackController::ledRGB(const int color[3]) {
    _pcf.write(lucesConf.pinR, color[0]);
    _pcf.write(lucesConf.pinG, color[1]);
    _pcf.write(lucesConf.pinB, color[2]);
}

void FeedbackController::ledModo(const String& modo) {
    if (modo == "manual") {
        ledRGB(lucesConf.colorManual);
    } else if (modo == "seguidor") {
        ledRGB(lucesConf.colorSeguidor);
    } else if (modo == "obstaculos") {
        ledRGB(lucesConf.colorObstaculos);
    } else if (modo == "navegacion") {
        ledRGB(lucesConf.colorNavegacion);
    } else {
        ledRGB(lucesConf.colorNull);
    }
}

// La web reenvía el mismo tipo cada 500 ms para producir el parpadeo intencional.
// Cada mensaje alterna el LED correspondiente; si cambia el tipo, se reinicia el estado.
void FeedbackController::direccional(const char* instruccion) {
    if (strcmp(instruccion, _ultimaInstruccion) != 0) {
        _direccionalDerActiva = false;
        _direccionalIzqActiva = false;
        strncpy(_ultimaInstruccion, instruccion, sizeof(_ultimaInstruccion) - 1);
        _ultimaInstruccion[sizeof(_ultimaInstruccion) - 1] = '\0';
    }

    if (strcmp(instruccion, "der") == 0) {
        _direccionalIzqActiva = false;
        _direccionalDerActiva = !_direccionalDerActiva;
    } else if (strcmp(instruccion, "izq") == 0) {
        _direccionalDerActiva = false;
        _direccionalIzqActiva = !_direccionalIzqActiva;
    } else if (strcmp(instruccion, "prev") == 0) {
        // Preventivas: si solo una direccional está activa, sincronizar ambas antes de alternar
        if (_direccionalDerActiva != _direccionalIzqActiva) {
            _direccionalDerActiva = false;
            _direccionalIzqActiva = false;
        }
        _direccionalDerActiva = !_direccionalDerActiva;
        _direccionalIzqActiva = _direccionalDerActiva;
    } else {
        _direccionalDerActiva = false;
        _direccionalIzqActiva = false;
    }

    // PCF8574 usa lógica invertida: LOW = LED encendido
    _pcf.write(lucesConf.pinLedDer, _direccionalDerActiva ? LOW : HIGH);
    _pcf.write(lucesConf.pinLedIzq, _direccionalIzqActiva ? LOW : HIGH);
}

void FeedbackController::apagar() {
    // Evita reescribir el PCF8574 en cada iteración del loop cuando ya están apagadas.
    if (!_direccionalDerActiva && !_direccionalIzqActiva) return;

    _direccionalDerActiva = false;
    _direccionalIzqActiva = false;
    _pcf.write(lucesConf.pinLedDer, HIGH);
    _pcf.write(lucesConf.pinLedIzq, HIGH);
}

// Detecta desaceleración (frenado) para encender las luces de freno durante 500 ms.
// Usa eventos no polling: solo escribe al PCF8574 cuando el estado cambia, con debounce de 50 ms (el I2C del PCF8574 es lento)
void FeedbackController::preventiva(float velocidadY, float zonaMuerta) {
    static float ultimaVelocidad = 0;
    static bool ultimoEstado = false;
    static unsigned long ultimaEscritura = 0;
    static unsigned long tiempoFreno = 0;
    static bool frenoRegistrado = false;

    // Se considera frenado si: la velocidad absoluta baja más de 0.05, o se invierte la dirección
    bool frenando = abs(velocidadY) < abs(ultimaVelocidad) - 0.05f || (ultimaVelocidad > 0 && velocidadY < 0);
    bool enMovimiento = abs(ultimaVelocidad) > zonaMuerta;
    bool estadoFreno = frenando && enMovimiento;

    if (estadoFreno) {
        tiempoFreno = millis();
        frenoRegistrado = true;
    }
    bool luzFreno = frenoRegistrado && (millis() - tiempoFreno) < 500;

    unsigned long ahora = millis();
    
    // Debounce de 50 ms: el PCF8574 por I2C puede generar NACK si se escribe muy rápido
    if (luzFreno != ultimoEstado && (ahora - ultimaEscritura) > 50) {
        _pcf.write(lucesConf.pinFrenoDer, luzFreno ? LOW : HIGH);
        _pcf.write(lucesConf.pinFrenoIzq, luzFreno ? LOW : HIGH);
        ultimoEstado = luzFreno;
        ultimaEscritura = ahora;
    }
    ultimaVelocidad = velocidadY;
}
