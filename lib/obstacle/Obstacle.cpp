#include "Obstacle.h"
#include "Motor.h"
#include "Sonar.h"

Obstacle::Obstacle(Motor& motor, Sonar& sonar)
    : _motor(motor), _sonar(sonar) {}

// Avanzar -> retroceder -> girar -> avanzar. Retroceder evita rozar el
// objeto y girar cambia el rumbo antes de reanudar la marcha.
void Obstacle::update(float speed) {
    float distance = _sonar.readFiltered();
    unsigned long now = millis();

    switch (_state) {
        case FORWARD:
            if (distance < OBSTACLE_CM && distance > 2) {
                _state = REVERSE;
                _stateStart = now;
            }
            break;

        case REVERSE:
            if (now - _stateStart > REVERSE_MS) {
                _state = TURN;
                _stateStart = now;
            }
            break;

        case TURN:
            if (now - _stateStart > TURN_MS) {
                _state = FORWARD;
            }
            break;
    }

    switch (_state) {
        case FORWARD:
            _motor.drive(0, speed);
            break;

        case REVERSE:
            _motor.drive(0, -speed);
            break;

        case TURN:
            // Avance lento + dirección: giro amplio sin pivotar sobre el sitio.
            _motor.drive(0.20f, 0.25f);
            break;
    }
}
