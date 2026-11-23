#include "Line.h"
#include "Motor.h"

Line::Line(Motor& motor) : _motor(motor) {}

void Line::begin() {
    pinMode(linePin1, INPUT);
    pinMode(linePin2, INPUT);
    pinMode(linePin3, INPUT);
    pinMode(linePin4, INPUT);
    pinMode(linePin5, INPUT);
    _lastRun = millis();
}

void Line::resetMomentum() {
    _momentum = 0;
}

void Line::follow(float speed) {
    unsigned long now = millis();
    float dt = (float)(now - _lastRun);
    _lastRun = now;
    if (dt < 0.1f || dt > 500.0f) return;

    // TCRT5000: LOW sobre blanco, HIGH sobre negro. Invertimos para 1 = línea.
    int s1 = !digitalRead(linePin1);
    int s2 = !digitalRead(linePin2);
    int s3 = !digitalRead(linePin3);
    int s4 = !digitalRead(linePin4);
    int s5 = !digitalRead(linePin5);

    int sensed = s1 + s2 + s3 + s4 + s5;

    if (sensed == 0) {
        // Sin línea, mantenemos la dirección previa un breve instante.
        _momentum *= expf(-dt / DECAY_MS);
        _motor.drive(constrain(_momentum * 0.5f, -1.0f, 1.0f), speed * 0.5f);
        return;
    }

    float error = (s1 * -1.0f + s2 * -0.45f + s4 * 0.45f + s5 * 1.0f) / sensed;
    float derivative = (error - _previousError) / dt;
    float kd = fabsf(error) < fabsf(_previousError) ? KD * 1.5f : KD;
    float correction = KP * error + kd * derivative * 10.0f;

    float decay = expf(-dt / DECAY_MS);
    _momentum = _momentum * decay + correction * (1.0f - decay);
    _previousError = error;

    float turnSpeed = speed * (1.0f - fabsf(error) * 0.45f);
    _motor.drive(constrain(_momentum, -1.0f, 1.0f), turnSpeed);
}
