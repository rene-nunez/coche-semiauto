#include "SmartCar.h"

SmartCar* SmartCar::_instance = nullptr;

SmartCar::SmartCar()
    : _wifi(ssid, password)
    , _mqtt(mqtt_server, port)
    , _speedSensor(sensorVelIzq, sensorVelDer)
    , _motor()
    , _ultrasonic(trig, echo)
    , _feedback()
    , _lineFollower(_motor)
    , _obstacleAvoidance(_motor, _ultrasonic)
    , _gpsNavigation(_motor, _mqtt, gpsRX, gpsTX)
{
    // Instancia estática para que la callback libre de PubSubClient pueda reenviar el mensaje a handleMessage()
    _instance = this;
    _motor.setEncoder(&_speedSensor);
}

void SmartCar::setup() {
    Serial.begin(115200);
    analogReadResolution(8);

    _wifi.connect();
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    _mqtt.begin();
    _mqtt.setCallback(onMessage);

    Wire.begin(21, 22);
    Wire.setClock(400000);

    _feedback.begin();
    _gpsNavigation.iniciar();
    _motor.begin();
    _lineFollower.iniciar();
    _ultrasonic.begin();
    _speedSensor.begin();
}

void SmartCar::loop() {
    _wifi.update();

    // Reconexión automática (limitada internamente a 1 intento cada 5 segundos para no saturar el broker)
    if (!_mqtt.isConnected()) {
        _mqtt.connect();
    }

    _mqtt.loop();

    // Sin conexión MQTT no hay forma de detener el vehículo de forma remota:
    // se detiene y se desactivan los modos de asistencia hasta que el usuario los reactive.
    if (!_mqtt.isConnected()) {
        _motor.drive(0, 0);
        _velocidadConstante = 0.0f;
        _gpsNavigation.parar();
        _feedback.off();
    }

    // El claxon y las luces de freno usan temporizadores no bloqueantes
    _feedback.update();

    // GPS y encoders se actualizan continuamente independientemente del modo
    _gpsNavigation.actualizar();
    _speedSensor.read();

    if (_modo == "manual") {
        // Timeout de seguridad: si no llega comando manual en 500 ms, detener el auto
        if (millis() - _ultimaVezRecibido > 500) {
            _motor.drive(0, 0);
        }
        if (millis() - _ultimaVezLuces > timeoutLuces) {
            _feedback.off();
        }
    } else if (_modo == "seguidor") {
        // _velocidadConstante > 0 es la señal de que el web habilitó el modo (evita arranques no intencionados)
        if (_velocidadConstante > 0.0) {
            _lineFollower.ejecutar(_velocidadConstante);
        } else {
            _motor.drive(0, 0);
        }
    } else if (_modo == "obstaculos") {
        if (_velocidadConstante > 0.0) {
            _obstacleAvoidance.actualizar(_velocidadConstante);
        } else {
            _motor.drive(0, 0);
        }
    } else if (_modo == "navegacion") {
        if (_gpsNavigation.hasValidLocation() && _gpsNavigation.isActive()) {
            _gpsNavigation.navegar();
        } else {
            _motor.drive(0, 0);
        }
    } else {
        _motor.drive(0, 0);
    }
}

void SmartCar::onMessage(const char* topic, JsonDocument& doc) {
    if (_instance) {
        _instance->handleMessage(topic, doc);
    }
}

void SmartCar::handleMessage(const char* topic, JsonDocument& doc) {
    if (strcmp(topic, topics.manual) == 0) {
        _ultimaVezRecibido = millis();
        float valorX = doc["x"] | 0.0f;
        float valorY = doc["y"] | 0.0f;
        _motor.drive(valorX, valorY);
        _feedback.brake(valorY, Motor::DEAD_ZONE);

    } else if (strcmp(topic, topics.modo) == 0) {
        const char* nuevoModo = doc["modo"];
        if (nuevoModo) {
            _modo = String(nuevoModo);
            _velocidadConstante = 0.0;
            _gpsNavigation.parar();
            _feedback.off();
            _feedback.confirmHorn();
            _feedback.setModeColor(nuevoModo);
        }
    } else if (strcmp(topic, topics.seguidor) == 0) {
        const char* accion = doc["accion"];
        bool activo = (accion && strcmp(accion, "activar") == 0);
        _velocidadConstante = activo ? 0.42 : 0.0;
        _lineFollower.reiniciarMomentum();

    } else if (strcmp(topic, topics.obstaculos) == 0) {
        const char* accion = doc["accion"];
        bool activo = (accion && strcmp(accion, "activar") == 0);
        _velocidadConstante = activo ? 0.45 : 0.0;

    } else if (strcmp(topic, topics.navegacion) == 0) {
        const char* accion = doc["accion"];
        if (accion) {
            if (strcmp(accion, "iniciar") == 0) {
                double lat = doc["lat"] | 0.0;
                double lon = doc["lon"] | 0.0;
                _gpsNavigation.colocarDestino(lat, lon);
                _gpsNavigation.avanzar();
            } else if (strcmp(accion, "detener") == 0) {
                _gpsNavigation.parar();
            } else if (strcmp(accion, "reanudar") == 0) {
                _gpsNavigation.reanudar();
            }
        }
    } else if (strcmp(topic, topics.luces) == 0) {
        const char* tipo = doc["tipo"] | "off";
        _ultimaVezLuces = millis();
        _feedback.indicator(tipo);

    } else if (strcmp(topic, topics.claxon) == 0) {
        int estado = doc["estado"] | 1;
        if (estado != 0) {
            _feedback.horn();
        }
    }
}