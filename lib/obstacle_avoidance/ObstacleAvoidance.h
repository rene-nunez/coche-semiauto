#ifndef OBSTACLE_AVOIDANCE_H
#define OBSTACLE_AVOIDANCE_H

#include "config.h"

class Motor;
class Sonar;

class ObstacleAvoidance {
    public:
        ObstacleAvoidance(Motor& motor, Sonar& sonic);

        void actualizar(float velocidadConstante);

    private:
        Motor& _motor;
        Sonar& _sonic;

        enum State { AVANZAR, RETROCEDER, GIRAR };

        State _estado = AVANZAR;
        unsigned long _tiempoEstado = 0;

        static constexpr unsigned long TIEMPO_RETROCEDER = 600;
        static constexpr unsigned long TIEMPO_GIRAR = 1000;
        static constexpr float OBSTACULO_DISTANCIA = 20.0;
};

#endif