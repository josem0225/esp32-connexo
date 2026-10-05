#pragma once

// ============================================================
// ReleTemporizado — cuándo encender y apagar el relé de la puerta.
//
// Por qué existe (2026-10-05, connexo-documentacion/fallos/07): con el relé activo,
// una orden nueva recibía 409 y se descartaba. Con el loop trabado el relé se
// apagaba justo cuando la segunda persona iba a pasar. Ahora una orden nueva
// ALARGA la apertura desde ese momento.
//
// Sin Arduino: lo usa main.cpp y se prueba con `pio test -e native`.
// Tolera el desborde de millis() (~49 días) con aritmética sin signo.
// ============================================================

class ReleTemporizado {
public:
    explicit ReleTemporizado(unsigned long duracionMs) : _duracion(duracionMs) {}

    // Enciende o alarga la apertura. Devuelve true si hay que ENCENDER el pin
    // (estaba apagado); false si ya estaba encendido y solo se alargó.
    bool activar(unsigned long ahoraMs) {
        bool estabaApagado = !_activo;
        _activo = true;
        _apagaEn = ahoraMs + _duracion;
        if (estabaApagado) _activaciones++;
        return estabaApagado;
    }

    // true (una sola vez) cuando toca apagar el pin.
    bool debeApagar(unsigned long ahoraMs) {
        if (!_activo) return false;
        if ((long)(ahoraMs - _apagaEn) < 0) return false;
        _activo = false;
        return true;
    }

    bool activo() const { return _activo; }
    unsigned long activaciones() const { return _activaciones; }

private:
    unsigned long _duracion;
    unsigned long _apagaEn = 0;
    bool _activo = false;
    unsigned long _activaciones = 0;
};
