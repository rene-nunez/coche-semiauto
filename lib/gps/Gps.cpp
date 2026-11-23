#include "Gps.h"
#include "Motor.h"
#include "Mqtt.h"

Gps::Gps(Motor& motor, Mqtt& mqtt, int rx, int tx)
    : _motor(motor), _mqtt(mqtt), _gpsSerial(2), _rxPin(rx), _txPin(tx) {}

void Gps::begin() {
    _gpsSerial.begin(9600, SERIAL_8N1, _rxPin, _txPin);
}

void Gps::update() {
    while (_gpsSerial.available() > 0) {
        if (!_gps.encode(_gpsSerial.read()) || !_gps.location.isValid()) continue;

        _lat = _gps.location.lat();
        _lon = _gps.location.lng();
        _satellites = _gps.satellites.isValid() ? _gps.satellites.value() : 0;

        if (millis() - _lastPublish <= PUBLISH_MS) continue;

        JsonDocument doc;
        doc["lat"] = _lat;
        doc["lon"] = _lon;
        doc["error"] = _headingError;
        doc["sat"] = _satellites;
        doc["destino"] = false;
        _mqtt.publish(topics.ubicacion, doc);
        _lastPublish = millis();
    }
}

void Gps::navigate() {
    if (_targetLat != _lastTargetLat || _targetLon != _lastTargetLon) {
        _lastTargetLat = _targetLat;
        _lastTargetLon = _targetLon;
        _lastCourseUpdate = 0;
        _correctionApplied = false;
        if (hasFix()) {
            _distance = _gps.distanceBetween(_lat, _lon, _targetLat, _targetLon);
            _targetCourse = _gps.courseTo(_lat, _lon, _targetLat, _targetLon);
        }
    }

    updateMetrics();
    if (_distance <= ARRIVAL_RADIUS_M) {
        finish();
        return;
    }

    unsigned long elapsed = millis() - _lastCourseUpdate;

    // Cada 6 s se mide el rumbo real recorrido; se ignoran desplazamientos
    // menores a 0.5 m para filtrar el ruido del GPS.
    if (elapsed > COURSE_INTERVAL_MS) {
        if (_hasPreviousFix &&
            _gps.distanceBetween(_previousLat, _previousLon, _lat, _lon) > MIN_DISPLACEMENT_M) {
            _actualCourse = _gps.courseTo(_previousLat, _previousLon, _lat, _lon);
            _lastCourseUpdate = millis();
            _correctionApplied = false;
        }
        _previousLat = _lat;
        _previousLon = _lon;
        _hasPreviousFix = true;
    }

    if (elapsed < CORRECTION_MS && _hasPreviousFix) {
        if (!_correctionApplied) {
            correctHeading(_actualCourse, _targetCourse);
            _correctionApplied = true;
        }
        return;
    }

    _motor.drive(0, CRUISE_SPEED);
}

void Gps::updateMetrics() {
    static unsigned long lastCalculation = 0;
    if (millis() - lastCalculation <= PUBLISH_MS || !_gps.location.isValid()) return;

    _distance = _gps.distanceBetween(_lat, _lon, _targetLat, _targetLon);
    _targetCourse = _gps.courseTo(_lat, _lon, _targetLat, _targetLon);
    lastCalculation = millis();
}

void Gps::correctHeading(double actual, double target) {
    _headingError = target - actual;
    if (_headingError > 180) _headingError -= 360;
    else if (_headingError < -180) _headingError += 360;

    float turn = abs(_headingError) < 30 ? 0 : constrain(_headingError / 90.0, -0.20, 0.20);
    float speed = abs(_headingError) > 45 ? 0.25 : CRUISE_SPEED;
    _motor.drive(turn, speed);
}

void Gps::finish() {
    _active = false;
    _headingError = 0;
    _motor.drive(0, 0);

    if (!_mqtt.isConnected()) return;

    JsonDocument doc;
    doc["lat"] = _lat;
    doc["lon"] = _lon;
    doc["error"] = _headingError;
    doc["sat"] = _satellites;
    doc["destino"] = true;
    _mqtt.publish(topics.ubicacion, doc);
}

void Gps::setTarget(double lat, double lon) {
    _targetLat = lat;
    _targetLon = lon;
    _active = (lat != 0 || lon != 0);
    // El rumbo inicial debe medirse desde la posición actual, no del destino anterior.
    _hasPreviousFix = false;
}

void Gps::start() {
    _active = true;
}

void Gps::stop() {
    _active = false;
    _motor.drive(0, 0);
}

void Gps::resume() {
    _active = true;
}

bool Gps::isActive() const {
    return _active;
}

bool Gps::hasFix() const {
    return _gps.location.isValid();
}

double Gps::headingError() const {
    return _headingError;
}
