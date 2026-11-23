#ifndef MQTT_H
#define MQTT_H

#include "config.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

class Mqtt {
    public:
        using Callback = void (*)(const char* topic, JsonDocument& doc);

        Mqtt(const char* server, int port);

        void begin();
        void connect();
        void loop();
        void publish(const char* topic, JsonDocument& doc);
        void setCallback(Callback cb);
        bool isConnected();

    private:
        WiFiClient _wifiClient;
        PubSubClient _client;
        Callback _callback = nullptr;

        static Mqtt* _instance;
        static void onMessage(char* topic, uint8_t* payload, unsigned int length);
};

#endif
