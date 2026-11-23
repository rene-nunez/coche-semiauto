#ifndef SMART_CAR_H
#define SMART_CAR_H

#include "config.h"
#include "Wifi.h"
#include "Mqtt.h"
#include "Encoder.h"
#include "Motor.h"
#include "Sonar.h"
#include "Signals.h"
#include "Line.h"
#include "ObstacleAvoidance.h"
#include "GPSNavigation.h"

#include <ArduinoJson.h>
#include <Wire.h>

class SmartCar {
    public:
        SmartCar();
        void setup();
        void loop();

    private:
        Wifi _wifi;
        Mqtt _mqtt;
        Encoder _speedSensor;
        Motor _motor;
        Sonar _ultrasonic;
        Signals _feedback;
        Line _lineFollower;
        ObstacleAvoidance _obstacleAvoidance;
        GPSNavigation _gpsNavigation;

        String _modo = "indefinido";
        float _velocidadConstante = 0.0;
        unsigned long _ultimaVezRecibido = 0;
        unsigned long _ultimaVezLuces = 0;

        static SmartCar* _instance;
        static void onMessage(const char* topic, JsonDocument& doc);
        void handleMessage(const char* topic, JsonDocument& doc);
};

#endif