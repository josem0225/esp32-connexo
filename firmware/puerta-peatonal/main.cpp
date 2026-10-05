#include <Arduino.h>
#include <ETH.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <esp_system.h>
#include "Config.h"
#include "ReleTemporizado.h"
#include "BusquedaDelBox.h"
#include "MedidorDeLoop.h"

// ============================================================
// NexusControl — Firmware Puerta Peatonal
//
// Ethernet + mDNS + HTTP server + LED/rele
// Endpoints: GET /status, POST /open, GET /sensor
//
// fallos/07 (2026-10-05): el loop() SOLO maneja relé y botón. Todo lo que habla por la
// red (latido, búsqueda del box, aviso del botón) corre en `tareaRed`, con timeouts
// cortos: un box caído ya no traba la puerta. Ver lib/nexus-common/src/*.h.
// ============================================================

// --- Estado global ---
static bool ethConnected = false;
static bool ethHasIP = false;
static bool servicesStarted = false;
static String deviceMAC = "";
static String deviceIP = "";

// --- Box (Mini-PC) al que se late: solo por mDNS, sin IP quemada ---
static BusquedaDelBox busqueda(DISCOVERY_RETRY_MS, HEARTBEAT_TIMEOUT_COUNT);
static portMUX_TYPE muxBusqueda = portMUX_INITIALIZER_UNLOCKED;

// --- Heartbeat ---
static unsigned long lastHeartbeat = 0;

// --- Rele/LED: /open corre en el hilo del servidor y el apagado en loop() ---
static ReleTemporizado rele(RELAY_OPEN_DURATION_MS);
static portMUX_TYPE muxRele = portMUX_INITIALIZER_UNLOCKED;

// --- Boton de salida ---
static bool lastButtonState = HIGH;  // Pull-up: HIGH = no presionado
static volatile unsigned long pulsacionesBoton = 0;
static volatile bool exitEventPendiente = false;   // lo manda tareaRed, no loop()

// --- Diagnóstico ---
static MedidorDeLoop medidorLoop;

// --- OTA (Over-The-Air update via Ethernet) ---
static bool otaRequested = false;
static String otaUrl = "";
static String _otaBody = "";
static bool _otaParseOk = false;
static String _otaParseError = "";

// --- HTTP Server (creado en heap, no como global) ---
static AsyncWebServer* server = nullptr;

// --- Rele/LED ---

// Enciende o ALARGA la apertura. Devuelve true si estaba apagado.
bool relayActivate() {
    portENTER_CRITICAL(&muxRele);
    bool encender = rele.activar(millis());
    if (encender) digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? HIGH : LOW);
    portEXIT_CRITICAL(&muxRele);
    Serial.println(encender ? "[RELAY] ON" : "[RELAY] apertura alargada");
    return encender;
}

void relayTick() {
    portENTER_CRITICAL(&muxRele);
    bool apagar = rele.debeApagar(millis());
    if (apagar) digitalWrite(RELAY_PIN, RELAY_ACTIVE_HIGH ? LOW : HIGH);
    portEXIT_CRITICAL(&muxRele);
    if (apagar) Serial.println("[RELAY] OFF");
}

bool relayIsActive() {
    portENTER_CRITICAL(&muxRele);
    bool activo = rele.activo();
    portEXIT_CRITICAL(&muxRele);
    return activo;
}

const char* motivoDeReinicio() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:  return "encendido";
        case ESP_RST_SW:       return "software";
        case ESP_RST_PANIC:    return "panico";
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:      return "watchdog";
        case ESP_RST_BROWNOUT: return "bajo_voltaje";
        case ESP_RST_EXT:      return "externo";
        default:               return "otro";
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
    doc["relay_active"] = relayIsActive();
    doc["door_sensor"] = digitalRead(DOOR_SENSOR_PIN) == LOW ? "closed" : "open";
    doc["ethernet"] = ethConnected;

    // Diagnóstico (fallos/07): el box lo lee y lo deja en su log.
    JsonObject diag = doc["diag"].to<JsonObject>();
    portENTER_CRITICAL(&muxBusqueda);
    String destino = busqueda.tieneDestino() ? String(busqueda.ip()) + ":" + busqueda.puerto() : String("");
    int fallos = busqueda.fallos();
    bool fallback = busqueda.enFallback();
    portEXIT_CRITICAL(&muxBusqueda);
    diag["mini_pc"] = destino;
    diag["fallos_latido"] = fallos;
    diag["fallback"] = fallback;
    diag["loop_max_ms"] = medidorLoop.maximoMs();
    diag["loop_max_hace_s"] = (millis() - medidorLoop.cuandoMs()) / 1000;
    portENTER_CRITICAL(&muxRele);
    diag["activaciones_rele"] = rele.activaciones();
    portEXIT_CRITICAL(&muxRele);
    diag["pulsaciones_boton"] = pulsacionesBoton;
    diag["motivo_reinicio"] = motivoDeReinicio();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

// Con el relé activo ya NO responde 409: alarga la apertura (fallos/07). Antes, la orden
// de la segunda persona se descartaba y el relé se apagaba justo cuando iba a pasar.
void handleOpen(AsyncWebServerRequest* request) {
    bool encendio = relayActivate();
    request->send(200, "application/json", encendio
        ? "{\"success\":true,\"duration_ms\":3000,\"message\":\"LED/Rele activado\"}"
        : "{\"success\":true,\"duration_ms\":3000,\"message\":\"Apertura alargada\",\"alargada\":true}");
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

// "" si todavía no se conoce el box: sin IP quemada, sin box no se late.
String getMiniPcBaseUrl() {
    portENTER_CRITICAL(&muxBusqueda);
    String url = busqueda.tieneDestino()
        ? String("http://") + busqueda.ip() + ":" + busqueda.puerto() : String("");
    portEXIT_CRITICAL(&muxBusqueda);
    return url;
}

// Solo desde tareaRed: MDNS.queryService bloquea ~3 s.
void discoverMiniPc() {
    Serial.println("[mDNS] Buscando Mini-PC (_nexusminipc._tcp)...");
    int n = MDNS.queryService("_nexusminipc", "_tcp");
    String ip = n > 0 ? MDNS.IP(0).toString() : String("");
    int puerto = n > 0 ? MDNS.port(0) : 0;
    portENTER_CRITICAL(&muxBusqueda);
    busqueda.buscado(millis());
    if (n > 0) busqueda.encontrado(ip.c_str(), puerto);
    portEXIT_CRITICAL(&muxBusqueda);
    if (n > 0) {
        Serial.print("[mDNS] Mini-PC encontrado: ");
        Serial.print(ip);
        Serial.print(":");
        Serial.println(puerto);
    } else {
        Serial.println("[mDNS] No encontrado; se reintenta en 60 s");
    }
}

// --- Notificar al Mini-PC que se abrio por boton de salida ---

void sendExitEvent() {
    if (!ethHasIP) return;

    String base = getMiniPcBaseUrl();
    if (base.length() == 0) return;

    HTTPClient http;
    String url = base + "/api/esp32/exit-event";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
    http.setTimeout(HTTP_TIMEOUT_MS);

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
        Serial.println("[BTN] Boton de salida presionado — abriendo puerta");
        relayActivate();                 // también alarga si ya estaba abierta
        pulsacionesBoton++;
        exitEventPendiente = true;       // el aviso al box lo manda tareaRed
    }
    lastButtonState = currentState;
}

// --- Heartbeat al Mini-PC ---

void sendHeartbeat() {
    if (!ethHasIP) return;
    String base = getMiniPcBaseUrl();
    if (base.length() == 0) return;        // sin box conocido: tareaRed lo sigue buscando

    HTTPClient http;
    String url = base + "/api/esp32/status";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);
    http.setTimeout(HTTP_TIMEOUT_MS);

    JsonDocument doc;
    doc["esp32_id"] = deviceMAC;
    portENTER_CRITICAL(&muxBusqueda);
    bool fallbackMode = busqueda.enFallback();
    portEXIT_CRITICAL(&muxBusqueda);
    doc["status"] = fallbackMode ? "fallback" : "online";
    doc["door_state"] = digitalRead(DOOR_SENSOR_PIN) == LOW ? "closed" : "open";
    doc["uptime_seconds"] = millis() / 1000;
    doc["firmware_version"] = NEXUS_FIRMWARE_VERSION;

    String body;
    serializeJson(doc, body);

    int httpCode = http.POST(body);
    http.end();

    portENTER_CRITICAL(&muxBusqueda);
    if (httpCode == 200) busqueda.latidoOk();
    else busqueda.latidoFallido(millis());
    int fallos = busqueda.fallos();
    portEXIT_CRITICAL(&muxBusqueda);

    if (httpCode == 200) {
        if (fallbackMode) Serial.println("[HB] Recuperado: saliendo de FALLBACK");
    } else {
        Serial.print("[HB] Fallo #");
        Serial.print(fallos);
        Serial.print(" (HTTP ");
        Serial.print(httpCode);
        Serial.println(") — si sigue, se vuelve a buscar el box cada 60 s");
    }
}

// --- Tarea de red: todo lo que puede bloquear, fuera de loop() (fallos/07) ---

void tareaRed(void* /*param*/) {
    for (;;) {
        if (ethHasIP) {
            bool buscar;
            portENTER_CRITICAL(&muxBusqueda);
            buscar = busqueda.debeBuscar(millis());
            portEXIT_CRITICAL(&muxBusqueda);
            if (buscar) discoverMiniPc();

            if (exitEventPendiente) {
                exitEventPendiente = false;
                sendExitEvent();
            }
            if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
                lastHeartbeat = millis();
                sendHeartbeat();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
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

    // Latido, búsqueda del box y aviso del botón: en su propia tarea (fallos/07).
    // La primera búsqueda la hace tareaRed apenas arranca.
    xTaskCreatePinnedToCore(tareaRed, "red", 8192, NULL, 1, NULL, 0);

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
    medidorLoop.vuelta(millis());
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

    delay(10);
}
