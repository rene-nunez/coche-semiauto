#ifndef SMART_CAR_H
#define SMART_CAR_H

#include "config.h"
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "Encoder.h"
#include "MotorDriver.h"
#include "Sonar.h"
#include "FeedbackController.h"
#include "LineFollower.h"
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
        WiFiManager _wifi;
        MQTTManager _mqtt;
        Encoder _speedSensor;
        MotorDriver _motor;
        Sonar _ultrasonic;
        FeedbackController _feedback;
        LineFollower _lineFollower;
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