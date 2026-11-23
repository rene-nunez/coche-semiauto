#ifndef SIGNALS_H
#define SIGNALS_H

#include "config.h"
#include <PCF8574.h>
#include <Wire.h>

class Signals {
    public:
        Signals();

        void begin();
        void horn();
        void confirmHorn();
        void setModeColor(const String& mode);
        void indicator(const char* command);
        void brake(float speedY, float deadZone);
        void off();
        void update();

    private:
        PCF8574 _pcf{0x20};
        int _buzzerChannel = canalBuzzer;

        void rgb(const int color[3]);
        void updateBrake();
        void play(const int* tones, const unsigned long* durations, uint8_t steps);

        bool _rightOn = false;
        bool _leftOn = false;
        char _lastCommand[8] = "";

        float _lastSpeed = 0;
        bool _brakeSeen = false;
        bool _brakeOn = false;
        unsigned long _brakeStart = 0;
        unsigned long _lastWrite = 0;

        const int* _tones = nullptr;
        const unsigned long* _durations = nullptr;
        uint8_t _steps = 0;
        uint8_t _step = 0;
        bool _playing = false;
        unsigned long _nextTone = 0;
};

#endif
