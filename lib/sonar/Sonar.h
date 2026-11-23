#ifndef SONAR_H
#define SONAR_H

#include "config.h"

class Sonar {
    public:
        Sonar(int trigPin, int echoPin);

        void begin();
        float read();
        float readFiltered();

    private:
        int _trig, _echo;
};

#endif
