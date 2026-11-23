#include "FeedbackController.h"

namespace {
const int TONOS_CLAXON[] = {800, 1000, 1200};
const unsigned long DURACIONES_CLAXON[] = {50, 50, 50};
const int TONOS_CONFIRMAR[] = {600, 900};
const unsigned long DURACIONES_CONFIRMAR[] = {80, 80};
}

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

// Secuencias no bloqueantes: el loop sigue atendiendo MQTT, GPS y el watchdog
// mientras suena el claxon.
void FeedbackController::iniciarSecuencia(const int* tonos, const unsigned long* duraciones, uint8_t pasos) {
    _tonos = tonos;
    _duraciones = duraciones;
    _numTonos = pasos;
    _pasoClaxon = 0;
    _claxonActivo = pasos > 0;

    if (!_claxonActivo) {
        ledcWriteTone(_canalBuzzer, 0);
        return;
    }

    ledcWriteTone(_canalBuzzer, _tonos[0]);
    _siguienteTono = millis() + _duraciones[0];
    _pasoClaxon = 1;
}

void FeedbackController::claxon() {
    iniciarSecuencia(TONOS_CLAXON, DURACIONES_CLAXON, 3);
}

void FeedbackController::claxonConfirmar() {
    iniciarSecuencia(TONOS_CONFIRMAR, DURACIONES_CONFIRMAR, 2);
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
    if (_direccionalDerActiva || _direccionalIzqActiva) {
        _direccionalDerActiva = false;
        _direccionalIzqActiva = false;
        _pcf.write(lucesConf.pinLedDer, HIGH);
        _pcf.write(lucesConf.pinLedIzq, HIGH);
    }

    if (_frenoRegistrado || _ultimoEstadoFreno) {
        _frenoRegistrado = false;
        _ultimoEstadoFreno = false;
        _ultimaVelocidad = 0.0f;
        _pcf.write(lucesConf.pinFrenoDer, HIGH);
        _pcf.write(lucesConf.pinFrenoIzq, HIGH);
    }
}

// Registra una desaceleración (frenado). La duración de 500 ms se evalúa en
// actualizar(), porque el último comando del joystick no se repite y el freno
// se quedaba encendido para siempre.
void FeedbackController::preventiva(float velocidadY, float zonaMuerta) {
    // Se considera frenado si: la velocidad absoluta baja más de 0.05, o se invierte la dirección
    bool frenando = abs(velocidadY) < abs(_ultimaVelocidad) - 0.05f || (_ultimaVelocidad > 0 && velocidadY < 0);
    bool enMovimiento = abs(_ultimaVelocidad) > zonaMuerta;

    if (frenando && enMovimiento) {
        _tiempoFreno = millis();
        _frenoRegistrado = true;
    }

    _ultimaVelocidad = velocidadY;
}

void FeedbackController::actualizarFreno() {
    if (!_frenoRegistrado) return;

    bool luzFreno = (millis() - _tiempoFreno) < 500;
    if (luzFreno == _ultimoEstadoFreno) return;

    // Debounce de 50 ms: el PCF8574 por I2C puede generar NACK si se escribe muy rápido
    unsigned long ahora = millis();
    if (ahora - _ultimaEscrituraFreno <= 50) return;

    _pcf.write(lucesConf.pinFrenoDer, luzFreno ? LOW : HIGH);
    _pcf.write(lucesConf.pinFrenoIzq, luzFreno ? LOW : HIGH);
    _ultimoEstadoFreno = luzFreno;
    _ultimaEscrituraFreno = ahora;

    if (!luzFreno) {
        _frenoRegistrado = false;
    }
}

void FeedbackController::actualizar() {
    actualizarFreno();

    if (!_claxonActivo || millis() < _siguienteTono) return;

    if (_pasoClaxon < _numTonos) {
        ledcWriteTone(_canalBuzzer, _tonos[_pasoClaxon]);
        _siguienteTono = millis() + _duraciones[_pasoClaxon];
        ++_pasoClaxon;
    } else {
        ledcWriteTone(_canalBuzzer, 0);
        _claxonActivo = false;
    }
}
