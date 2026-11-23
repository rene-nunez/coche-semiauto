#ifndef FEEDBACK_CONTROLLER_H
#define FEEDBACK_CONTROLLER_H

#include "config.h"
#include <PCF8574.h>
#include <Wire.h>

class FeedbackController {
    public:
        FeedbackController();

        void iniciar();

        void claxon();
        void claxonConfirmar();

        void ledModo(const String& modo);
        void direccional(const char* instruccion);
        void preventiva(float velocidadY, float zonaMuerta);
        void apagar();
        void actualizar();

    private:
        PCF8574 _pcf{0x20};

        int _canalBuzzer = canalBuzzer;

        void ledRGB(const int color[3]);
        void actualizarFreno();
        void iniciarSecuencia(const int* tonos, const unsigned long* duraciones, uint8_t pasos);

        bool _direccionalDerActiva = false;
        bool _direccionalIzqActiva = false;
        char _ultimaInstruccion[8] = "";

        float _ultimaVelocidad = 0.0f;
        bool _ultimoEstadoFreno = false;
        unsigned long _ultimaEscrituraFreno = 0;
        unsigned long _tiempoFreno = 0;
        bool _frenoRegistrado = false;

        const int* _tonos = nullptr;
        const unsigned long* _duraciones = nullptr;
        uint8_t _numTonos = 0;
        uint8_t _pasoClaxon = 0;
        bool _claxonActivo = false;
        unsigned long _siguienteTono = 0;
};

#endif