#ifndef GPS_H
#define GPS_H

#include "config.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>

class Motor;
class Mqtt;

class Gps {
    public:
        Gps(Motor& motor, Mqtt& mqtt, int rx, int tx);

        void begin();
        void update();
        void navigate();

        void setTarget(double lat, double lon);
        void start();
        void stop();
        void resume();

        bool isActive() const;
        bool hasFix() const;
        double headingError() const;

    private:
        Motor& _motor;
        Mqtt& _mqtt;
        HardwareSerial _gpsSerial;
        TinyGPSPlus _gps;

        int _rxPin, _txPin;

        double _targetLat = 0;
        double _targetLon = 0;
        double _lastTargetLat = 0;
        double _lastTargetLon = 0;
        bool _active = false;
        double _headingError = 0;

        double _lat = 0;
        double _lon = 0;
        double _previousLat = 0;
        double _previousLon = 0;
        double _distance = 0;
        double _targetCourse = 0;
        double _actualCourse = 0;
        int _satellites = 0;

        bool _hasPreviousFix = false;
        bool _correctionApplied = false;

        unsigned long _lastPublish = 0;
        unsigned long _lastCourseUpdate = 0;

        static constexpr float ARRIVAL_RADIUS_M = 2.5f;
        static constexpr unsigned long PUBLISH_MS = 1000;
        static constexpr unsigned long COURSE_INTERVAL_MS = 6000;
        static constexpr unsigned long CORRECTION_MS = 1000;
        static constexpr double MIN_DISPLACEMENT_M = 0.5;
        static constexpr double CRUISE_SPEED = 0.45;

        void updateMetrics();
        void correctHeading(double actual, double target);
        void finish();
};

#endif
