#ifndef MOTOR_H
#define MOTOR_H

#include "config.h"

class Encoder;

class Motor {
    public:
        static constexpr float DEAD_ZONE = 0.1f;

        Motor();

        void begin();
        void drive(float x, float y);
        void stop();

        void setEncoder(Encoder* encoder);
        float lastY() const;

    private:
        Encoder* _encoder = nullptr;

        static constexpr int MIN_PWM = 140;
        static constexpr int MAX_PWM = 255;
        static constexpr int PWM_RANGE = MAX_PWM - MIN_PWM;

        static constexpr float KP = 0.0008f;
        static constexpr float KI = 0.0003f;
        static constexpr float INTEGRAL_MAX = 0.3f;
        static constexpr float BIAS_FORWARD = 0.06f;
        static constexpr float BIAS_REVERSE = 0.02f;
        static constexpr float MIN_CORRECTION_RPM = 23.0f;
        static constexpr float ERROR_DEADBAND_RPM = 20.0f;

        float _integralError = 0;
        unsigned long _lastCorrection = 0;
        float _lastY = 0;

        static int _lastPwm[16];

        int pwm(float value);
        void applyDirection(float value, int speed, int channel1, int channel2);
        void writePwm(int channel, int value);
};

#endif
