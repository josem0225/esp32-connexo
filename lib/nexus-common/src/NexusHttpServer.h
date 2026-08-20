#pragma once

// ============================================================
// NexusHttpServer — Servidor HTTP del ESP32 (ESPAsyncWebServer)
//
// Responsabilidad unica: servir endpoints REST.
// No contiene logica de negocio — delega al RelayController.
//
// Endpoints:
//   GET  /status → JSON con estado del dispositivo
//   POST /open   → activa rele/LED por RELAY_OPEN_DURATION_MS
//   GET  /sensor → estado del sensor magnetico
// ============================================================

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "RelayController.h"
#include "EthernetManager.h"

class NexusHttpServer {
public:
    NexusHttpServer(EthernetManager& eth, RelayController& relay)
        : _server(NEXUS_ESP32_PORT), _eth(eth), _relay(relay) {}

    void begin() {
        _server.on("/status", HTTP_GET,
            [this](AsyncWebServerRequest* request) { handleStatus(request); });

        _server.on("/open", HTTP_POST,
            [this](AsyncWebServerRequest* request) { handleOpen(request); });

        _server.on("/sensor", HTTP_GET,
            [this](AsyncWebServerRequest* request) { handleSensor(request); });

        _server.onNotFound([](AsyncWebServerRequest* request) {
            request->send(404, "application/json", "{\"error\":\"not_found\"}");
        });

        _server.begin();
        Serial.print("[HTTP] Servidor en puerto ");
        Serial.println(NEXUS_ESP32_PORT);
    }

private:
    AsyncWebServer _server;
    EthernetManager& _eth;
    RelayController& _relay;

    void handleStatus(AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["mac"] = _eth.macAddress();
        doc["ip"] = _eth.localIP();
        doc["tipo"] = NEXUS_DEVICE_TYPE;
        doc["firmware"] = NEXUS_FIRMWARE_VERSION;
        doc["uptime_seconds"] = millis() / 1000;
        doc["relay_active"] = _relay.isActive();
        doc["door_sensor"] = readDoorSensor();
        doc["ethernet"] = _eth.isConnected();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }

    void handleOpen(AsyncWebServerRequest* request) {
        if (_relay.isActive()) {
            request->send(409, "application/json",
                "{\"error\":\"already_open\",\"message\":\"Rele ya esta activo\"}");
            return;
        }

        _relay.activate();

        JsonDocument doc;
        doc["success"] = true;
        doc["duration_ms"] = RELAY_OPEN_DURATION_MS;
        doc["message"] = "Rele activado";

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }

    void handleSensor(AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["door_state"] = readDoorSensor();
        doc["sensor_pin"] = DOOR_SENSOR_PIN;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }

    static const char* readDoorSensor() {
        return digitalRead(DOOR_SENSOR_PIN) == LOW ? "closed" : "open";
    }
};
