#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include "config.h"

class Wifi {
    public:
        Wifi(const char* ssid, const char* password);

        void connect();
        void update();

    private:
        const char* _ssid;
        const char* _password;
};

#endif
