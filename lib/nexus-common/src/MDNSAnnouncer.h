#pragma once

// ============================================================
// MDNSAnnouncer — Registro de servicio mDNS para autodescubrimiento
//
// Responsabilidad unica: anunciar el ESP32 en la LAN como
// servicio _nexuscontrol._tcp con metadata en TXT records.
// El Mini-PC usa zeroconf (Python) para descubrirlo.
// ============================================================

#include <Arduino.h>
#include <ESPmDNS.h>
#include "Config.h"

class MDNSAnnouncer {
public:
    bool begin(const String& mac) {
        // Nombre mDNS: nexus-AABBCC (ultimos 6 chars del MAC sin ":")
        String macClean = mac;
        macClean.replace(":", "");
        String suffix = macClean.substring(macClean.length() - 6);
        suffix.toLowerCase();
        _hostname = "nexus-" + suffix;

        if (!MDNS.begin(_hostname.c_str())) {
            Serial.println("[mDNS] ERROR: no se pudo iniciar");
            return false;
        }

        MDNS.addService(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, NEXUS_ESP32_PORT);
        MDNS.addServiceTxt(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, "mac", mac.c_str());
        MDNS.addServiceTxt(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, "tipo", NEXUS_DEVICE_TYPE);
        MDNS.addServiceTxt(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, "version", NEXUS_FIRMWARE_VERSION);

        Serial.print("[mDNS] Anunciando: ");
        Serial.print(_hostname);
        Serial.println(".local");
        _running = true;
        return true;
    }

    bool isRunning() const { return _running; }
    const String& hostname() const { return _hostname; }

private:
    String _hostname;
    bool _running = false;
};
