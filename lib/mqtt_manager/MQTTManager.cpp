#include "MQTTManager.h"

MQTTManager* MQTTManager::_instance = nullptr;

MQTTManager::MQTTManager(const char* server, int port)
    : _client(_wifiClient) {
    _client.setServer(server, port);
}

// PubSubClient requiere una callback de estilo C (función libre); _instance es el puente hacia el método miembro
void MQTTManager::iniciar() {
    _instance = this;
    _client.setCallback(onMessage);
}

void MQTTManager::conectar() {
    static unsigned long ultimaReconexion = 0;
    static bool primerIntento = true;

    if (!_client.connected()) {
        unsigned long ahora = millis();
        // Primer intento inmediato y luego uno cada 5 segundos para no saturar el broker con CONNECT packets
        if (primerIntento || ahora - ultimaReconexion > 5000) {
            primerIntento = false;
            ultimaReconexion = ahora;

            // ID único por dispositivo basado en la MAC; evita conflictos si hay múltiples smart-cars en el mismo broker
            String clientId = "smartcar-" + String((uint32_t)ESP.getEfuseMac(), HEX);

            if (_client.connect(clientId.c_str())) {
                // Se resuscriben todos los tópicos en cada conexión porque no usamos sesiones persistentes
                _client.subscribe(topics.manual);
                _client.subscribe(topics.seguidor);
                _client.subscribe(topics.obstaculos);
                _client.subscribe(topics.navegacion);
                _client.subscribe(topics.modo);
                _client.subscribe(topics.claxon);
                _client.subscribe(topics.luces);
            }
        }
    }
}

void MQTTManager::procesar() {
    _client.loop();
}

void MQTTManager::publicar(const char* topic, JsonDocument& doc) {
    char payload[160];
    size_t longitud = serializeJson(doc, payload, sizeof(payload));
    if (longitud == 0 || longitud >= sizeof(payload)) {
        Serial.println(F("Payload MQTT demasiado largo; no se publica"));
        return;
    }
    _client.publish(topic, payload, longitud);
}

void MQTTManager::asignarCallback(MessageCallback cb) {
    _callback = cb;
}

bool MQTTManager::estaConectado() {
    return _client.connected();
}

void MQTTManager::onMessage(char* topic, uint8_t* payload, unsigned int length) {
    if (_instance && _instance->_callback) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
            Serial.print(F("Error JSON: "));
            Serial.println(error.f_str());
            return;
        }
        _instance->_callback(topic, doc);
    }
}