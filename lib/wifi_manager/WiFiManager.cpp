#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* ssid, const char* password)
    : _ssid(ssid), _password(password) {}

void WiFiManager::conectar() {
    delay(10);
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(_ssid);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _password);

    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED) {
        // No bloquear el arranque indefinidamente si la red no está disponible.
        if (millis() - inicio > 15000) {
            Serial.println();
            Serial.println("WiFi no disponible tras 15 s; se continuará intentando en segundo plano");
            return;
        }
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void WiFiManager::actualizar() {
    static unsigned long ultimoIntento = 0;

    if (WiFi.status() == WL_CONNECTED) return;

    // WiFi.setAutoReconnect() ya lo intenta, pero un reconnect explícito
    // cada 5 s recupera también caídas del AP que la pila WiFi no detecta.
    if (millis() - ultimoIntento < 5000) return;
    ultimoIntento = millis();

    WiFi.reconnect();
}