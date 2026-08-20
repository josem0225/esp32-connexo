#pragma once

// ============================================================
// RelayController — Control de GPIO para rele/LED
//
// Responsabilidad unica: activar GPIO por N milisegundos,
// desactivar automaticamente al cumplir el tiempo.
// Usar tick() en loop() para el control de temporización.
// ============================================================

#include <Arduino.h>
#include "Config.h"

class RelayController {
public:
    void begin() {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? LOW : HIGH);
        Serial.println("[RELAY] GPIO configurado");
    }

    void activate(unsigned long durationMs = RELAY_OPEN_DURATION_MS) {
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? HIGH : LOW);
        _activeUntil = millis() + durationMs;
        _active = true;
        Serial.print("[RELAY] Activado por ");
        Serial.print(durationMs);
        Serial.println(" ms");
    }

    void deactivate() {
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? LOW : HIGH);
        _active = false;
        _activeUntil = 0;
        Serial.println("[RELAY] Desactivado");
    }

    // Llamar en cada iteracion de loop()
    void tick() {
        if (_active && millis() >= _activeUntil) {
            deactivate();
        }
    }

    bool isActive() const { return _active; }

private:
    bool _active = false;
    unsigned long _activeUntil = 0;
};
