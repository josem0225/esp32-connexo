# Pinout — Puerta Peatonal (WT32-ETH01)

Board: **WT32-ETH01** (ESP32 + LAN8720A Ethernet, sin USB)
Programador: **CP2102** (USB-to-UART TTL)

## GPIOs disponibles en WT32-ETH01

Pines usados internamente por Ethernet (NO usar):
- GPIO0 (CLK), GPIO16 (PHY power), GPIO18 (MDIO), GPIO23 (MDC)
- GPIO1 (TX0), GPIO3 (RX0) — UART0 (serial/debug)
- GPIO5 (usado internamente por algunos revisions del board)

Pines libres para uso general:
- **GPIO2, GPIO4, GPIO14, GPIO15, GPIO17, GPIO32, GPIO33, GPIO35, GPIO36, GPIO39**
- **NO usar GPIO12** (controla voltaje flash en boot)

## Conexiones GPIO — Puerta Peatonal

| Funcion              | Pin GPIO | Direccion | Notas                                    |
|----------------------|----------|-----------|------------------------------------------|
| Rele electroiman     | GPIO 2   | OUTPUT    | Via optoacoplador PC817. HIGH = abre      |
| Sensor magnetico     | GPIO 4   | INPUT     | Reed switch / efecto Hall. LOW = cerrada  |
| Boton salida         | GPIO 14  | INPUT     | Pull-up interno. LOW = presionado         |

> **NO usar GPIO12** — controla voltaje del flash en boot. HIGH al arrancar
> causa loop de reset.

## Ethernet LAN8720A (pines fijos, NO cambiar)

| Funcion    | Pin GPIO | Nota                    |
|------------|----------|-------------------------|
| MDC        | GPIO 23  | Fijo en WT32-ETH01      |
| MDIO       | GPIO 18  | Fijo en WT32-ETH01      |
| CLK        | GPIO 0   | ETH_CLOCK_GPIO0_IN      |
| PHY Power  | GPIO 16  | Enciende/apaga LAN8720  |

## Conexion CP2102 (para flashear)

| CP2102 | WT32-ETH01 | Header |
|--------|------------|--------|
| TX     | RX0 (IO3)  | Pin RX |
| RX     | TX0 (IO1)  | Pin TX |
| GND    | GND        | GND    |
| 3.3V   | 3V3        | 3V3    |

Para entrar en modo flash:
1. Conectar IO0 a GND (o mantener presionado boton BOOT si tiene)
2. Hacer reset (desconectar/reconectar 3.3V)
3. Soltar IO0
4. `pio run -e puerta-peatonal -t upload`

## Alimentacion

- WT32-ETH01: 3.3V por pin 3V3 (via CP2102 para dev, fuente externa en produccion)
- Rele: 12V DC (fuente independiente)
- Electroiman: 12V DC (alimentado por el rele)
- **Tierra comun** obligatoria entre ESP32 y circuito del rele
