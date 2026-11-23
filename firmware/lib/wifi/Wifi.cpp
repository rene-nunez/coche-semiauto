#include "Wifi.h"

namespace {
constexpr unsigned long CONNECT_TIMEOUT_MS = 15000;
constexpr unsigned long RETRY_MS = 5000;
}

Wifi::Wifi(const char* ssid, const char* password)
    : _ssid(ssid), _password(password) {}

void Wifi::connect() {
    Serial.print("Conectando a ");
    Serial.println(_ssid);

    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > CONNECT_TIMEOUT_MS) {
            Serial.println("WiFi no disponible; se reintentará en segundo plano");
            return;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void Wifi::update() {
    static unsigned long lastAttempt = 0;

    if (WiFi.status() == WL_CONNECTED) return;
    if (millis() - lastAttempt < RETRY_MS) return;

    // La pila WiFi ya reintenta, pero un reconnect explícito también recupera
    // caídas del punto de acceso que no llega a detectar.
    lastAttempt = millis();
    WiFi.reconnect();
}
