#include "config.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const char* mqttHost = "BROKER_HOST";
const int mqttPort = 1883;

const Topics topics = {
    .manual = "coche-semiauto/modo/manual",
    .line = "coche-semiauto/modo/seguidor",
    .obstacle = "coche-semiauto/modo/obstaculos",
    .navigation = "coche-semiauto/modo/navegacion",
    .mode = "coche-semiauto/accion/modo",
    .horn = "coche-semiauto/accion/claxon",
    .lights = "coche-semiauto/accion/luces",
    .location = "coche-semiauto/estado/ubicacion"
};

const int motorA1 = 18;
const int motorA2 = 19;
const int motorB1 = 13;
const int motorB2 = 4;

const int channelA1 = 0;
const int channelA2 = 1;
const int channelB1 = 2;
const int channelB2 = 3;
const int channelBuzzer = 4;

const int pwmFrequency = 5000;
const int pwmResolution = 8;

const int buzzerPin = 5;
const int buzzerFrequency = 2000;

const int trigPin = 14;
const int echoPin = 23;

const int linePin1 = 32;
const int linePin2 = 33;
const int linePin3 = 25;
const int linePin4 = 26;
const int linePin5 = 27;

const int encoderPinLeft = 34;
const int encoderPinRight = 35;

const int gpsRx = 16;
const int gpsTx = 17;

const LightConfig lights {
    .redPin = 0,
    .greenPin = 1,
    .bluePin = 2,
    .rightPin = 4,
    .leftPin = 3,
    .brakeRightPin = 5,
    .brakeLeftPin = 6,
    .manualColor = {1, 0, 1},
    .lineColor = {0, 1, 0},
    .obstacleColor = {1, 0, 0},
    .navigationColor = {1, 1, 0},
    .offColor = {0, 0, 0}
};

const unsigned long lightsTimeout = 3000;
