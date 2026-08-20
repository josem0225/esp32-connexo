#pragma once

// ============================================================
// EthernetManager — Inicializa Ethernet LAN8720A del WT32-ETH01
//
// Responsabilidad unica: conectar Ethernet con DHCP o IP estatica,
// manejar eventos de conexion/desconexion, exponer estado e IP.
// ============================================================

#include <Arduino.h>
#include <ETH.h>
#include "Config.h"

class EthernetManager {
public:
    void begin() {
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
            this->handleEvent(event);
        });

        ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC,
                  ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);

        if (!NEXUS_USE_DHCP) {
            IPAddress ip, gateway, subnet, dns;
            ip.fromString(NEXUS_ESP32_IP);
            gateway.fromString(NEXUS_ESP32_GATEWAY);
            subnet.fromString(NEXUS_ESP32_SUBNET);
            dns.fromString(NEXUS_ESP32_DNS);
            ETH.config(ip, gateway, subnet, dns);
        }

        Serial.println("[ETH] Inicializando Ethernet...");
    }

    bool isConnected() const { return _connected; }
    bool hasIP() const { return _hasIP; }
    String localIP() const { return ETH.localIP().toString(); }
    String macAddress() const { return ETH.macAddress(); }

private:
    volatile bool _connected = false;
    volatile bool _hasIP = false;

    void handleEvent(WiFiEvent_t event) {
        switch (event) {
            case ARDUINO_EVENT_ETH_START:
                Serial.println("[ETH] Iniciado");
                ETH.setHostname("nexus-esp32");
                break;
            case ARDUINO_EVENT_ETH_CONNECTED:
                Serial.println("[ETH] Cable conectado");
                _connected = true;
                break;
            case ARDUINO_EVENT_ETH_GOT_IP:
                _hasIP = true;
                Serial.print("[ETH] IP: ");
                Serial.println(ETH.localIP());
                Serial.print("[ETH] MAC: ");
                Serial.println(ETH.macAddress());
                break;
            case ARDUINO_EVENT_ETH_DISCONNECTED:
                Serial.println("[ETH] Cable desconectado");
                _connected = false;
                _hasIP = false;
                break;
            default:
                break;
        }
    }
};
