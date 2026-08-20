# NexusControl — ESP32 Firmware

Firmware C++ (PlatformIO + Arduino) para los controladores ESP32 de NexusControl.

## Requisitos

- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) o extensión VS Code
- CP2102 (USB-to-UART) para flashear boards sin USB integrado

## Conexion CP2102 -> ESP32

| CP2102 | ESP32   |
|--------|---------|
| TX     | RX (GPIO3) |
| RX     | TX (GPIO1) |
| GND    | GND     |
| 3.3V   | 3.3V    |

Para entrar en modo flash: mantener **GPIO0** presionado mientras se hace reset.

## Compilar y flashear

```bash
# Compilar puerta peatonal
pio run -e puerta-peatonal

# Flashear (con CP2102 conectado)
pio run -e puerta-peatonal -t upload

# Monitor serial
pio device monitor
```

## Estructura

```
esp32-firmware/
├── lib/nexus-common/       -> Codigo compartido (Ethernet, heartbeat, HTTP server)
├── firmware/
│   └── puerta-peatonal/    -> Firmware especifico para puerta peatonal
├── test/                   -> Tests unitarios
└── docs/                   -> Pinout y documentacion hardware
```

## Agregar un nuevo tipo de dispositivo

1. Crear `firmware/nuevo-dispositivo/main.cpp`
2. Agregar en `platformio.ini`:
   ```ini
   [env:nuevo-dispositivo]
   board = esp32dev
   build_src_filter = +<nuevo-dispositivo/>
   ```
3. Compilar: `pio run -e nuevo-dispositivo`
