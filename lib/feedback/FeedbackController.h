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

    private:
        PCF8574 _pcf{0x20};

        int _canalBuzzer = canalBuzzer;

        void ledRGB(const int color[3]);

        bool _direccionalDerActiva = false;
        bool _direccionalIzqActiva = false;
        char _ultimaInstruccion[8] = "";
};

#endif