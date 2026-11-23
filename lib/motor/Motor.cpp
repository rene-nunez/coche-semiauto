#include "Motor.h"
#include "Encoder.h"

int Motor::_lastPwm[16] = {-1};

Motor::Motor() {}

void Motor::begin() {
    ledcSetup(canalA1, freq, resolucion);
    ledcSetup(canalA2, freq, resolucion);
    ledcSetup(canalB1, freq, resolucion);
    ledcSetup(canalB2, freq, resolucion);

    ledcAttachPin(motorA1, canalA1);
    ledcAttachPin(motorA2, canalA2);
    ledcAttachPin(motorB1, canalB1);
    ledcAttachPin(motorB2, canalB2);
}

void Motor::setEncoder(Encoder* encoder) {
    _encoder = encoder;
}

float Motor::lastY() const {
    return _lastY;
}

int Motor::pwm(float value) {
    if (abs(value) > DEAD_ZONE) {
        return MIN_PWM + (int)(abs(value) * PWM_RANGE);
    }
    return 0;
}

void Motor::applyDirection(float value, int speed, int channel1, int channel2) {
    int first = 0;
    int second = 0;

    if (value > DEAD_ZONE) {
        second = speed;
    } else if (value < -DEAD_ZONE) {
        first = speed;
    }

    writePwm(channel1, first);
    writePwm(channel2, second);
}

void Motor::writePwm(int channel, int value) {
    if (_lastPwm[channel] == value) return;
    ledcWrite(channel, value);
    _lastPwm[channel] = value;
}

void Motor::drive(float x, float y) {
    _lastY = y;

    float left = constrain(y + x, -1.0f, 1.0f);
    float right = constrain(y - x, -1.0f, 1.0f);

    if (_encoder) {
        float leftRpm = _encoder->leftRpm();
        float rightRpm = _encoder->rightRpm();
        float avgRpm = (leftRpm + rightRpm) / 2.0f;

        // Solo en línea recta: al girar, las ruedas deben ir a distinta velocidad.
        bool straight = abs(x) < 0.05f && abs(y) > DEAD_ZONE;
        bool moving = leftRpm > MIN_CORRECTION_RPM && rightRpm > MIN_CORRECTION_RPM;

        if (straight && moving) {
            unsigned long now = millis();
            float dt = constrain((now - _lastCorrection) / 1000.0f, 0.0f, 0.2f);
            _lastCorrection = now;

            // En reversa el mismo desequilibrio gira el coche al contrario.
            float error = (leftRpm - rightRpm) * (y >= 0.0f ? 1.0f : -1.0f);
            if (abs(error) < ERROR_DEADBAND_RPM) error = 0;

            float speedFactor = constrain(avgRpm / (MIN_CORRECTION_RPM * 4.0f), 0.0f, 1.0f);
            float effectiveKi = KI * speedFactor;

            _integralError = constrain(_integralError + error * dt,
                                       -INTEGRAL_MAX / KI, INTEGRAL_MAX / KI);

            float correction = KP * error + effectiveKi * _integralError;
            left -= correction;
            right += correction;

            float bias = y > 0 ? BIAS_FORWARD : BIAS_REVERSE;
            right -= bias * abs(y) * (y > 0 ? 1.0f : -1.0f);

            left = constrain(left, -1.0f, 1.0f);
            right = constrain(right, -1.0f, 1.0f);
        } else {
            _integralError = 0;
            _lastCorrection = millis();
        }
    }

    applyDirection(left, pwm(left), canalA1, canalA2);
    applyDirection(right, pwm(right), canalB1, canalB2);
}

void Motor::stop() {
    _lastY = 0;
    drive(0, 0);
}
