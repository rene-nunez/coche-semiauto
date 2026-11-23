#include "Mqtt.h"

namespace {
constexpr unsigned long RECONNECT_MS = 5000;
constexpr size_t PAYLOAD_SIZE = 160;
}

Mqtt* Mqtt::_instance = nullptr;

Mqtt::Mqtt(const char* server, int port)
    : _client(_wifiClient) {
    _client.setServer(server, port);
}

void Mqtt::begin() {
    _instance = this;
    _client.setCallback(onMessage);
}

void Mqtt::connect() {
    static unsigned long lastAttempt = 0;
    static bool firstAttempt = true;

    if (_client.connected()) return;
    unsigned long now = millis();
    if (!firstAttempt && now - lastAttempt < RECONNECT_MS) return;

    firstAttempt = false;
    lastAttempt = now;

    String clientId = "coche-semiauto-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (!_client.connect(clientId.c_str())) return;

    _client.subscribe(topics.manual);
    _client.subscribe(topics.line);
    _client.subscribe(topics.obstacle);
    _client.subscribe(topics.navigation);
    _client.subscribe(topics.mode);
    _client.subscribe(topics.horn);
    _client.subscribe(topics.lights);
}

void Mqtt::loop() {
    _client.loop();
}

void Mqtt::publish(const char* topic, JsonDocument& doc) {
    char payload[PAYLOAD_SIZE];
    size_t length = serializeJson(doc, payload, sizeof(payload));
    if (length == 0 || length >= sizeof(payload)) {
        Serial.println(F("Payload MQTT demasiado largo; no se publica"));
        return;
    }
    _client.publish(topic, payload, length);
}

void Mqtt::setCallback(Callback cb) {
    _callback = cb;
}

bool Mqtt::isConnected() {
    return _client.connected();
}

void Mqtt::onMessage(char* topic, uint8_t* payload, unsigned int length) {
    if (!_instance || !_instance->_callback) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.print(F("Error JSON: "));
        Serial.println(error.f_str());
        return;
    }

    _instance->_callback(topic, doc);
}
