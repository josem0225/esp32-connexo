#include <Arduino.h>
#include <ETH.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "Config.h"

// ============================================================
// NexusControl — Firmware Puerta Peatonal
//
// Ethernet + mDNS + HTTP server + LED/rele
// Endpoints: GET /status, POST /open, GET /sensor
// ============================================================

// --- Estado global ---
static bool ethConnected = false;
static bool ethHasIP = false;
static bool servicesStarted = false;
static String deviceMAC = "";
static String deviceIP = "";

// --- Mini-PC descubierto por mDNS ---
static String miniPcIP = "";
static int miniPcPort = NEXUS_MINI_PC_PORT;

// --- Heartbeat ---
static unsigned long lastHeartbeat = 0;
static int heartbeatFailures = 0;
static bool fallbackMode = false;

// --- Rele/LED ---
static bool relayActive = false;
static unsigned long relayOffAt = 0;

// --- Boton de salida ---
static bool lastButtonState = HIGH;  // Pull-up: HIGH = no presionado

// --- OTA (Over-The-Air update via Ethernet) ---
static bool otaRequested = false;
static String otaUrl = "";
static String _otaBody = "";
static bool _otaParseOk = false;
static String _otaParseError = "";

// --- HTTP Server (creado en heap, no como global) ---
static AsyncWebServer* server = nullptr;

// --- Rele/LED ---

void relayActivate() {
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? HIGH : LOW);
    relayOffAt = millis() + RELAY_OPEN_DURATION_MS;
    relayActive = true;
    Serial.println("[RELAY] LED/Rele ON");
}

void relayDeactivate() {
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? LOW : HIGH);
    relayActive = false;
    Serial.println("[RELAY] LED/Rele OFF");
}

void relayTick() {
    if (relayActive && millis() >= relayOffAt) {
        relayDeactivate();
    }
}

// --- HTTP Handlers ---

void handleStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["mac"] = deviceMAC;
    doc["ip"] = deviceIP;
    doc["tipo"] = NEXUS_DEVICE_TYPE;
    doc["firmware"] = NEXUS_FIRMWARE_VERSION;
    doc["uptime_seconds"] = millis() / 1000;
    doc["relay_active"] = relayActive;
    doc["door_sensor"] = digitalRead(DOOR_SENSOR_PIN) == LOW ? "closed" : "open";
    doc["ethernet"] = ethConnected;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleOpen(AsyncWebServerRequest* request) {
    if (relayActive) {
        request->send(409, "application/json",
            "{\"error\":\"already_open\",\"message\":\"Ya esta activo\"}");
        return;
    }
    relayActivate();
    request->send(200, "application/json",
        "{\"success\":true,\"duration_ms\":3000,\"message\":\"LED/Rele activado\"}");
}

void handleSensor(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["door_state"] = digitalRead(DOOR_SENSOR_PIN) == LOW ? "closed" : "open";
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// --- OTA: descarga firmware por HTTP y reflashea ---

void performOTA() {
    Serial.println("[OTA] ========================================");
    Serial.println("[OTA] Descargando firmware desde:");
    Serial.println("[OTA] " + otaUrl);
    Serial.println("[OTA] ========================================");

    WiFiClient client;
    httpUpdate.rebootOnUpdate(true);

    t_httpUpdate_return ret = httpUpdate.update(client, otaUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] FALLO: (%d) %s\n",
                httpUpdate.getLastError(),
                httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] Sin actualizacion disponible");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("[OTA] Exito — reiniciando...");
            // El ESP32 reinicia automaticamente
            break;
    }
}

void _otaBodyHandler(AsyncWebServerRequest* request, uint8_t* data,
                     size_t len, size_t index, size_t total) {
    if (index == 0) {
        _otaBody = "";
        _otaParseOk = false;
        _otaParseError = "";
    }
    _otaBody.concat((char*)data, len);

    if (index + len >= total) {
        JsonDocument doc;
        if (deserializeJson(doc, _otaBody) != DeserializationError::Ok) {
            _otaParseError = "invalid_json";
            return;
        }
        if (!doc["url"].is<String>() || doc["url"].as<String>().length() == 0) {
            _otaParseError = "url_required";
            return;
        }
        otaUrl = doc["url"].as<String>();
        _otaParseOk = true;
    }
}

void handleOta(AsyncWebServerRequest* request) {
    if (_otaParseError.length() > 0) {
        String err = "{\"error\":\"" + _otaParseError + "\"}";
        request->send(400, "application/json", err);
        return;
    }
    if (!_otaParseOk) {
        request->send(400, "application/json", "{\"error\":\"no_body\"}");
        return;
    }
    otaRequested = true;
    JsonDocument resp;
    resp["success"] = true;
    resp["message"] = "OTA iniciado, el ESP32 se reiniciara en segundos";
    resp["url"] = otaUrl;
    resp["current_version"] = NEXUS_FIRMWARE_VERSION;
    String response;
    serializeJson(resp, response);
    request->send(200, "application/json", response);
}

// --- Ethernet callback (firma con 2 parametros) ---

void onEthEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("[ETH] Iniciado");
            ETH.setHostname("nexus-esp32");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("[ETH] Cable conectado");
            ethConnected = true;
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            ethHasIP = true;
            deviceIP = ETH.localIP().toString();
            deviceMAC = ETH.macAddress();
            Serial.print("[ETH] IP: ");
            Serial.print(deviceIP);
            Serial.print(" MAC: ");
            Serial.println(deviceMAC);
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("[ETH] Desconectado");
            ethConnected = false;
            ethHasIP = false;
            break;
        default:
            break;
    }
}

// --- Descubrimiento mDNS del Mini-PC ---

String getMiniPcBaseUrl() {
    if (miniPcIP.length() > 0) {
        return String("http://") + miniPcIP + ":" + miniPcPort;
    }
    return String("http://") + NEXUS_MINI_PC_IP + ":" + NEXUS_MINI_PC_PORT;
}

void discoverMiniPc() {
    Serial.println("[mDNS] Buscando Mini-PC (_nexusminipc._tcp)...");
    int n = MDNS.queryService("_nexusminipc", "_tcp");
    if (n > 0) {
        miniPcIP = MDNS.IP(0).toString();
        miniPcPort = MDNS.port(0);
        Serial.print("[mDNS] Mini-PC encontrado: ");
        Serial.print(miniPcIP);
        Serial.print(":");
        Serial.println(miniPcPort);
    } else {
        Serial.print("[mDNS] No encontrado, usando fallback: ");
        Serial.println(NEXUS_MINI_PC_IP);
    }
}

// --- Notificar al Mini-PC que se abrio por boton de salida ---

void sendExitEvent() {
    if (!ethHasIP) return;

    HTTPClient http;
    String url = getMiniPcBaseUrl() + "/api/esp32/exit-event";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(3000);

    JsonDocument doc;
    doc["esp32_id"] = deviceMAC;
    doc["event"] = "exit_button";

    String body;
    serializeJson(doc, body);

    int httpCode = http.POST(body);
    http.end();

    if (httpCode == 200) {
        Serial.println("[BTN] Mini-PC notificado (exit_button)");
    } else {
        Serial.print("[BTN] Mini-PC no respondio (HTTP ");
        Serial.print(httpCode);
        Serial.println(") — best-effort, puerta ya abrio");
    }
}

// --- Boton de salida (GPIO14, pull-up, LOW = presionado) ---

void checkExitButton() {
    bool currentState = digitalRead(EXIT_BUTTON_PIN);
    // Detectar flanco: HIGH → LOW (presionado)
    if (lastButtonState == HIGH && currentState == LOW) {
        if (!relayActive) {
            Serial.println("[BTN] Boton de salida presionado — abriendo puerta");
            relayActivate();
            sendExitEvent();
        }
    }
    lastButtonState = currentState;
}

// --- Heartbeat al Mini-PC ---

void sendHeartbeat() {
    if (!ethHasIP) return;

    HTTPClient http;
    String url = getMiniPcBaseUrl() + "/api/esp32/status";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["esp32_id"] = deviceMAC;
    doc["status"] = fallbackMode ? "fallback" : "online";
    doc["door_state"] = digitalRead(DOOR_SENSOR_PIN) == LOW ? "closed" : "open";
    doc["uptime_seconds"] = millis() / 1000;
    doc["firmware_version"] = NEXUS_FIRMWARE_VERSION;

    String body;
    serializeJson(doc, body);

    int httpCode = http.POST(body);
    http.end();

    if (httpCode == 200) {
        if (heartbeatFailures > 0) {
            Serial.print("[HB] Recuperado tras ");
            Serial.print(heartbeatFailures);
            Serial.println(" fallos");
        }
        heartbeatFailures = 0;
        if (fallbackMode) {
            fallbackMode = false;
            Serial.println("[HB] Saliendo de modo FALLBACK");
        }
    } else {
        heartbeatFailures++;
        Serial.print("[HB] Fallo #");
        Serial.print(heartbeatFailures);
        Serial.print(" (HTTP ");
        Serial.print(httpCode);
        Serial.println(")");

        if (heartbeatFailures >= HEARTBEAT_TIMEOUT_COUNT && !fallbackMode) {
            fallbackMode = true;
            Serial.println("[HB] === MODO FALLBACK ACTIVADO ===");
            // Re-descubrir Mini-PC por si cambió de IP
            discoverMiniPc();
        }
    }
}

// --- Iniciar servicios cuando hay IP ---

void startNetworkServices() {
    Serial.println("[MAIN] Iniciando servicios...");

    // mDNS
    String macClean = deviceMAC;
    macClean.replace(":", "");
    String suffix = macClean.substring(macClean.length() - 6);
    suffix.toLowerCase();
    String hostname = "nexus-" + suffix;

    if (MDNS.begin(hostname.c_str())) {
        MDNS.addService(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, NEXUS_ESP32_PORT);
        MDNS.addServiceTxt(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, "mac", deviceMAC.c_str());
        MDNS.addServiceTxt(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, "tipo", NEXUS_DEVICE_TYPE);
        MDNS.addServiceTxt(NEXUS_MDNS_SERVICE, NEXUS_MDNS_PROTOCOL, "version", NEXUS_FIRMWARE_VERSION);
        Serial.print("[mDNS] ");
        Serial.print(hostname);
        Serial.println(".local");
    }

    // HTTP
    server = new AsyncWebServer(NEXUS_ESP32_PORT);
    server->on("/status", HTTP_GET, handleStatus);
    server->on("/open", HTTP_POST, handleOpen);
    server->on("/sensor", HTTP_GET, handleSensor);
    server->on("/ota", HTTP_POST, handleOta, NULL, _otaBodyHandler);
    server->onNotFound([](AsyncWebServerRequest* r) {
        r->send(404, "application/json", "{\"error\":\"not_found\"}");
    });
    server->begin();
    Serial.print("[HTTP] Puerto ");
    Serial.println(NEXUS_ESP32_PORT);

    // Descubrir Mini-PC por mDNS (sin IP hardcodeada)
    discoverMiniPc();

    servicesStarted = true;
    Serial.println("\n===== LISTO =====");
    Serial.print("curl http://");
    Serial.print(deviceIP);
    Serial.println("/status");
    Serial.print("curl -X POST http://");
    Serial.print(deviceIP);
    Serial.println("/open");
}

// --- Setup & Loop ---

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("\n========================================");
    Serial.println("  NexusControl — Puerta Peatonal v1");
    Serial.println("========================================");

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? LOW : HIGH);
    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    pinMode(EXIT_BUTTON_PIN, INPUT_PULLUP);
    Serial.println("[MAIN] GPIO OK (relay=IO2, sensor=IO4, boton=IO14)");

    WiFi.onEvent(onEthEvent);
    ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC,
              ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);
    Serial.println("[MAIN] Ethernet iniciado, esperando DHCP...");
}

void loop() {
    if (ethHasIP && !servicesStarted) {
        startNetworkServices();
    }
    relayTick();
    checkExitButton();

    // OTA: se ejecuta en loop() porque httpUpdate.update() es bloqueante
    if (otaRequested) {
        otaRequested = false;
        delay(500);  // Dar tiempo a que la respuesta HTTP se envíe
        performOTA();
    }

    // Heartbeat al Mini-PC cada HEARTBEAT_INTERVAL_MS
    if (servicesStarted && millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = millis();
        sendHeartbeat();
    }

    delay(10);
}
