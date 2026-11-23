#include "Sonar.h"

namespace {
constexpr unsigned long ECHO_TIMEOUT_US = 25000;
constexpr float NO_ECHO_CM = 400.0f;
constexpr int SAMPLES = 3;
}

Sonar::Sonar(int trigPin, int echoPin)
    : _trig(trigPin), _echo(echoPin) {}

void Sonar::begin() {
    pinMode(_trig, OUTPUT);
    pinMode(_echo, INPUT);
}

float Sonar::read() {
    digitalWrite(_trig, LOW);
    delayMicroseconds(2);
    digitalWrite(_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trig, LOW);

    long echo = pulseIn(_echo, HIGH, ECHO_TIMEOUT_US);
    return echo == 0 ? NO_ECHO_CM : (echo * 0.0343f) / 2.0f;
}

float Sonar::readFiltered() {
    float total = 0;
    for (int i = 0; i < SAMPLES; ++i) {
        total += read();
        delay(5);
    }
    return total / SAMPLES;
}
