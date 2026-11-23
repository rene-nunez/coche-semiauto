#include "Car.h"

namespace {
constexpr unsigned long COMMAND_TIMEOUT_MS = 500;
}

Car* Car::_instance = nullptr;

Car::Car()
    : _wifi(ssid, password)
    , _mqtt(mqtt_server, port)
    , _encoder(sensorVelIzq, sensorVelDer)
    , _motor()
    , _sonar(trig, echo)
    , _signals()
    , _line(_motor)
    , _obstacle(_motor, _sonar)
    , _gps(_motor, _mqtt, gpsRX, gpsTX) {
    _instance = this;
    _motor.setEncoder(&_encoder);
}

void Car::setup() {
    Serial.begin(115200);
    analogReadResolution(8);

    _wifi.connect();
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    _mqtt.begin();
    _mqtt.setCallback(onMessage);

    Wire.begin(21, 22);
    Wire.setClock(400000);

    _signals.begin();
    _gps.begin();
    _motor.begin();
    _line.begin();
    _sonar.begin();
    _encoder.begin();
}

void Car::loop() {
    _wifi.update();

    if (!_mqtt.isConnected()) {
        _mqtt.connect();
    }
    _mqtt.loop();

    // Sin MQTT no se puede detener el coche en remoto.
    if (!_mqtt.isConnected()) {
        stop();
    }

    _signals.update();
    _gps.update();
    _encoder.read();

    if (_mode == "manual") {
        if (millis() - _lastCommand > COMMAND_TIMEOUT_MS) stop();
        if (millis() - _lastIndicator > timeoutLuces) _signals.off();
    } else if (_mode == "seguidor") {
        if (_speed > 0) _line.follow(_speed);
        else stop();
    } else if (_mode == "obstaculos") {
        if (_speed > 0) _obstacle.update(_speed);
        else stop();
    } else if (_mode == "navegacion") {
        if (_gps.hasFix() && _gps.isActive()) _gps.navigate();
        else stop();
    } else {
        stop();
    }
}

void Car::stop() {
    _motor.drive(0, 0);
    _speed = 0;
    _gps.stop();
    _signals.off();
}

void Car::onMessage(const char* topic, JsonDocument& doc) {
    if (_instance) _instance->handleMessage(topic, doc);
}

void Car::handleMessage(const char* topic, JsonDocument& doc) {
    if (strcmp(topic, topics.manual) == 0) {
        _lastCommand = millis();
        float x = doc["x"] | 0.0f;
        float y = doc["y"] | 0.0f;
        _motor.drive(x, y);
        _signals.brake(y, Motor::DEAD_ZONE);

    } else if (strcmp(topic, topics.modo) == 0) {
        const char* mode = doc["modo"];
        if (!mode) return;
        _mode = String(mode);
        stop();
        _signals.confirmHorn();
        _signals.setModeColor(mode);

    } else if (strcmp(topic, topics.seguidor) == 0) {
        const char* action = doc["accion"];
        bool active = action && strcmp(action, "activar") == 0;
        _speed = active ? 0.42 : 0;
        _line.resetMomentum();

    } else if (strcmp(topic, topics.obstaculos) == 0) {
        const char* action = doc["accion"];
        bool active = action && strcmp(action, "activar") == 0;
        _speed = active ? 0.45 : 0;

    } else if (strcmp(topic, topics.navegacion) == 0) {
        const char* action = doc["accion"];
        if (!action) return;
        if (strcmp(action, "iniciar") == 0) {
            double lat = doc["lat"] | 0.0;
            double lon = doc["lon"] | 0.0;
            _gps.setTarget(lat, lon);
            _gps.start();
        } else if (strcmp(action, "detener") == 0) {
            _gps.stop();
        } else if (strcmp(action, "reanudar") == 0) {
            _gps.resume();
        }

    } else if (strcmp(topic, topics.luces) == 0) {
        const char* type = doc["tipo"] | "off";
        _lastIndicator = millis();
        _signals.indicator(type);

    } else if (strcmp(topic, topics.claxon) == 0) {
        if ((doc["estado"] | 1) != 0) _signals.horn();
    }
}
