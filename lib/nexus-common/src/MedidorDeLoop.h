#pragma once

// ============================================================
// MedidorDeLoop — la vuelta más larga del loop() desde que arrancó.
//
// Por qué existe (2026-10-05, connexo-documentacion/fallos/07): el loop del ESP32 de
// Suárez León estuvo 17 días trabado sin que nada lo mostrara. Va en /status, el box lo
// lee y lo deja en su log. Sin Arduino: se prueba con `pio test -e native`.
// ============================================================

class MedidorDeLoop {
public:
    void vuelta(unsigned long ahoraMs) {
        if (_hayAnterior) {
            unsigned long duracion = ahoraMs - _anterior;     // sin signo: tolera el desborde
            if (duracion > _maximo) { _maximo = duracion; _cuando = ahoraMs; }
        }
        _anterior = ahoraMs;
        _hayAnterior = true;
    }

    unsigned long maximoMs() const { return _maximo; }
    unsigned long cuandoMs() const { return _cuando; }

private:
    unsigned long _anterior = 0;
    bool _hayAnterior = false;
    unsigned long _maximo = 0;
    unsigned long _cuando = 0;
};
