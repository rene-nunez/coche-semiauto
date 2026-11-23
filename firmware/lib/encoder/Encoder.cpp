#include "Encoder.h"

volatile uint32_t Encoder::_pulsesRight = 0;
volatile uint32_t Encoder::_pulsesLeft = 0;

Encoder::Encoder(int leftPin, int rightPin)
    : _leftPin(leftPin), _rightPin(rightPin) {}

void Encoder::begin() {
    pinMode(_leftPin, INPUT);
    pinMode(_rightPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_rightPin), countRight, RISING);
    attachInterrupt(digitalPinToInterrupt(_leftPin), countLeft, RISING);
}

// IRAM_ATTR evita esperas a flash que perderían pulsos.
void IRAM_ATTR Encoder::countRight() {
    ++_pulsesRight;
}

void IRAM_ATTR Encoder::countLeft() {
    ++_pulsesLeft;
}

void Encoder::read() {
    unsigned long now = millis();
    unsigned long elapsed = now - _lastRead;
    if (elapsed < 120) return;

    noInterrupts();
    uint32_t right = _pulsesRight;
    uint32_t left = _pulsesLeft;
    _pulsesRight = 0;
    _pulsesLeft = 0;
    interrupts();

    float minutes = elapsed / 60000.0f;
    _rightRpm = (right / (float)PULSES_PER_REV) / minutes;
    _leftRpm = (left / (float)PULSES_PER_REV) / minutes;
    _lastRead = now;
}

void Encoder::reset() {
    noInterrupts();
    _pulsesRight = 0;
    _pulsesLeft = 0;
    interrupts();
    _rightRpm = 0;
    _leftRpm = 0;
}

float Encoder::leftRpm() const { return _leftRpm; }
float Encoder::rightRpm() const { return _rightRpm; }
