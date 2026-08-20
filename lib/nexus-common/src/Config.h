#pragma once

// ============================================================
// NexusControl ESP32 — Configuracion de hardware y red
// ============================================================

// --- Identificacion ---
#define NEXUS_DEVICE_TYPE     "puerta-peatonal"
#define NEXUS_FIRMWARE_VERSION "1.0.0"

// --- Red ---
#define NEXUS_USE_DHCP        true     // true = DHCP, false = IP estatica
#define NEXUS_MINI_PC_IP      "192.168.0.162"
#define NEXUS_MINI_PC_PORT    8000
#define NEXUS_ESP32_PORT      80

// IP estatica (solo si NEXUS_USE_DHCP = false)
#define NEXUS_ESP32_IP        "192.168.1.50"
#define NEXUS_ESP32_GATEWAY   "192.168.1.1"
#define NEXUS_ESP32_SUBNET    "255.255.255.0"
#define NEXUS_ESP32_DNS       "8.8.8.8"

// --- mDNS ---
#define NEXUS_MDNS_SERVICE    "_nexuscontrol"
#define NEXUS_MDNS_PROTOCOL   "_tcp"

// --- Heartbeat ---
#define HEARTBEAT_INTERVAL_MS 5000   // 5 segundos
#define HEARTBEAT_TIMEOUT_COUNT 3    // 3 fallos = modo fallback (15s)

// --- Rele electroiman ---
#define RELAY_PIN             2      // WT32-ETH01: GPIO2
#define RELAY_ACTIVE_HIGH     true   // true = HIGH abre, false = LOW abre
#define RELAY_OPEN_DURATION_MS 3000  // 3 segundos de apertura

// --- Sensor magnetico (estado puerta) ---
#define DOOR_SENSOR_PIN       4      // WT32-ETH01: GPIO4
#define DOOR_OPEN_TIMEOUT_MS  60000  // Alerta si abierta > 60s

// --- Boton de salida ---
#define EXIT_BUTTON_PIN       14     // WT32-ETH01: GPIO14 (NO usar GPIO12)

// --- Ethernet WT32-ETH01 (LAN8720A) — pines fijos, NO cambiar ---
#define ETH_PHY_TYPE          ETH_PHY_LAN8720
#define ETH_PHY_ADDR          1
#define ETH_PHY_MDC           23
#define ETH_PHY_MDIO          18
#define ETH_PHY_POWER         16
#define ETH_CLK_MODE          ETH_CLOCK_GPIO0_IN
