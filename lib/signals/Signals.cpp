#include "Signals.h"

namespace {
const int HORN_TONES[] = {800, 1000, 1200};
const unsigned long HORN_DURATIONS[] = {50, 50, 50};
const int CONFIRM_TONES[] = {600, 900};
const unsigned long CONFIRM_DURATIONS[] = {80, 80};

constexpr unsigned long BRAKE_MS = 500;
constexpr unsigned long WRITE_DELAY_MS = 50;
}

Signals::Signals() {}

void Signals::begin() {
    // Direccionales y freno son activos en bajo; el RGB es activo en alto.
    if (!_pcf.begin(0xFF)) {
        Serial.println(F("PCF8574 no detectado en la dirección 0x20"));
    }
    rgb(lights.offColor);

    ledcSetup(_buzzerChannel, buzzerFrequency, pwmResolution);
    ledcAttachPin(buzzerPin, _buzzerChannel);
}

// El tono se avanza desde update(): delays aquí bloquearían MQTT y el watchdog.
void Signals::play(const int* tones, const unsigned long* durations, uint8_t steps) {
    _tones = tones;
    _durations = durations;
    _steps = steps;
    _step = 0;
    _playing = steps > 0;

    if (!_playing) {
        ledcWriteTone(_buzzerChannel, 0);
        return;
    }

    ledcWriteTone(_buzzerChannel, _tones[0]);
    _nextTone = millis() + _durations[0];
    _step = 1;
}

void Signals::horn() {
    play(HORN_TONES, HORN_DURATIONS, 3);
}

void Signals::confirmHorn() {
    play(CONFIRM_TONES, CONFIRM_DURATIONS, 2);
}

void Signals::rgb(const int color[3]) {
    _pcf.write(lights.redPin, color[0]);
    _pcf.write(lights.greenPin, color[1]);
    _pcf.write(lights.bluePin, color[2]);
}

void Signals::setModeColor(const String& mode) {
    if (mode == "manual") rgb(lights.manualColor);
    else if (mode == "seguidor") rgb(lights.lineColor);
    else if (mode == "obstaculos") rgb(lights.obstacleColor);
    else if (mode == "navegacion") rgb(lights.navigationColor);
    else rgb(lights.offColor);
}

// La web repite el mismo tipo cada 500 ms para producir el parpadeo.
void Signals::indicator(const char* command) {
    if (strcmp(command, _lastCommand) != 0) {
        _rightOn = false;
        _leftOn = false;
        strncpy(_lastCommand, command, sizeof(_lastCommand) - 1);
        _lastCommand[sizeof(_lastCommand) - 1] = '\0';
    }

    if (strcmp(command, "der") == 0) {
        _leftOn = false;
        _rightOn = !_rightOn;
    } else if (strcmp(command, "izq") == 0) {
        _rightOn = false;
        _leftOn = !_leftOn;
    } else if (strcmp(command, "prev") == 0) {
        if (_rightOn != _leftOn) {
            _rightOn = false;
            _leftOn = false;
        }
        _rightOn = !_rightOn;
        _leftOn = _rightOn;
    } else {
        _rightOn = false;
        _leftOn = false;
    }

    // PCF8574: LOW enciende el LED.
    _pcf.write(lights.rightPin, _rightOn ? LOW : HIGH);
    _pcf.write(lights.leftPin, _leftOn ? LOW : HIGH);
}

void Signals::off() {
    if (_rightOn || _leftOn) {
        _rightOn = false;
        _leftOn = false;
        _pcf.write(lights.rightPin, HIGH);
        _pcf.write(lights.leftPin, HIGH);
    }

    if (_brakeSeen || _brakeOn) {
        _brakeSeen = false;
        _brakeOn = false;
        _lastSpeed = 0;
        _pcf.write(lights.brakeRightPin, HIGH);
        _pcf.write(lights.brakeLeftPin, HIGH);
    }
}

void Signals::brake(float speedY, float deadZone) {
    bool slowing = abs(speedY) < abs(_lastSpeed) - 0.05f || (_lastSpeed > 0 && speedY < 0);
    if (slowing && abs(_lastSpeed) > deadZone) {
        _brakeStart = millis();
        _brakeSeen = true;
    }
    _lastSpeed = speedY;
}

void Signals::updateBrake() {
    if (!_brakeSeen) return;

    bool on = millis() - _brakeStart < BRAKE_MS;
    if (on == _brakeOn) return;
    if (millis() - _lastWrite <= WRITE_DELAY_MS) return;

    _pcf.write(lights.brakeRightPin, on ? LOW : HIGH);
    _pcf.write(lights.brakeLeftPin, on ? LOW : HIGH);
    _brakeOn = on;
    _lastWrite = millis();
    if (!on) _brakeSeen = false;
}

void Signals::update() {
    updateBrake();

    if (!_playing || millis() < _nextTone) return;

    if (_step < _steps) {
        ledcWriteTone(_buzzerChannel, _tones[_step]);
        _nextTone = millis() + _durations[_step];
        ++_step;
    } else {
        ledcWriteTone(_buzzerChannel, 0);
        _playing = false;
    }
}
