#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "config.h"

class Motor;
class Sonar;

class Obstacle {
    public:
        Obstacle(Motor& motor, Sonar& sonar);

        void update(float speed);

    private:
        Motor& _motor;
        Sonar& _sonar;

        enum State { FORWARD, REVERSE, TURN };

        State _state = FORWARD;
        unsigned long _stateStart = 0;

        static constexpr unsigned long REVERSE_MS = 600;
        static constexpr unsigned long TURN_MS = 1000;
        static constexpr float OBSTACLE_CM = 20.0f;
};

#endif
