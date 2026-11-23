#ifndef LINE_H
#define LINE_H

#include "config.h"

class Motor;

class Line {
    public:
        Line(Motor& motor);

        void begin();
        void follow(float speed);
        void resetMomentum();

    private:
        Motor& _motor;

        static constexpr float KP = 1.00f;
        static constexpr float KD = 0.55f;
        static constexpr float DECAY_MS = 120.0f;

        float _previousError = 0;
        float _momentum = 0;
        unsigned long _lastRun = 0;
};

#endif
