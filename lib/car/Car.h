#ifndef CAR_H
#define CAR_H

#include "config.h"
#include "Wifi.h"
#include "Mqtt.h"
#include "Encoder.h"
#include "Motor.h"
#include "Sonar.h"
#include "Signals.h"
#include "Line.h"
#include "Obstacle.h"
#include "Gps.h"

#include <ArduinoJson.h>
#include <Wire.h>

class Car {
    public:
        Car();
        void setup();
        void loop();

    private:
        void stop();

        Wifi _wifi;
        Mqtt _mqtt;
        Encoder _encoder;
        Motor _motor;
        Sonar _sonar;
        Signals _signals;
        Line _line;
        Obstacle _obstacle;
        Gps _gps;

        String _mode = "indefinido";
        float _speed = 0;
        unsigned long _lastCommand = 0;
        unsigned long _lastIndicator = 0;

        static Car* _instance;

        static void onMessage(const char* topic, JsonDocument& doc);
        void handleMessage(const char* topic, JsonDocument& doc);
};

#endif
