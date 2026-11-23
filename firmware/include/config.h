#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

extern const char* ssid;
extern const char* password;

extern const char* mqttHost;
extern const int mqttPort;

struct Topics {
    const char* manual;
    const char* line;
    const char* obstacle;
    const char* navigation;
    const char* mode;
    const char* horn;
    const char* lights;
    const char* location;
};
extern const Topics topics;

extern const int motorA1, motorA2, motorB1, motorB2;
extern const int channelA1, channelA2, channelB1, channelB2, channelBuzzer;
extern const int pwmFrequency, pwmResolution;

extern const int buzzerPin, buzzerFrequency;

extern const int trigPin, echoPin;
extern const int linePin1, linePin2, linePin3, linePin4, linePin5;
extern const int encoderPinLeft, encoderPinRight;
extern const int gpsRx, gpsTx;

struct LightConfig {
    int redPin, greenPin, bluePin;
    int rightPin, leftPin;
    int brakeRightPin, brakeLeftPin;
    int manualColor[3];
    int lineColor[3];
    int obstacleColor[3];
    int navigationColor[3];
    int offColor[3];
};
extern const LightConfig lights;
extern const unsigned long lightsTimeout;

#endif
