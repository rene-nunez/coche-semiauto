#ifndef ENCODER_H
#define ENCODER_H

#include "config.h"

class Encoder {
    public:
        Encoder(int leftPin, int rightPin);

        void begin();
        void read();
        void reset();

        float leftRpm() const;
        float rightRpm() const;

    private:
        int _leftPin, _rightPin;

        static constexpr int PULSES_PER_REV = 20;

        static volatile uint32_t _pulsesRight;
        static volatile uint32_t _pulsesLeft;

        float _rightRpm = 0;
        float _leftRpm = 0;
        unsigned long _lastRead = 0;

        static void IRAM_ATTR countRight();
        static void IRAM_ATTR countLeft();
};

#endif
