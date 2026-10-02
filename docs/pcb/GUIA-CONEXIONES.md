# Connexo - Controlador de Puerta: Guia de Conexiones

## Lo que viene soldado (JLCPCB ensambla)

Todo lo siguiente llega ya soldado en la placa. NO tocar.

| Ref | Que es | Tipo |
|-----|--------|------|
| R1-R12 | Resistencias SMD | No tocar |
| C1-C12 | Capacitores SMD | No tocar |
| D1, D4-D6 | Diodos Schottky | No tocar |
| D3 | LED de encendido (verde) | No tocar |
| F1, F2 | Fusibles PTC | No tocar |
| U1 | Regulador 3.3V | No tocar |
| U2 | Driver de reles (ULN2003A) | No tocar |
| U3 | Optoacoplador (PC817) | No tocar |
| U4 | Buck converter 12V a 5V | No tocar |
| L1 | Inductor del buck | No tocar |
| K1, K2 | Reles (cubos azules) | No tocar |
| J1 | USB-C (desarrollo) | No tocar |
| J3 | Barrel jack 12V | No tocar |
| J4 | Bornera 12V entrada | No tocar |
| J5, J6 | Sockets hembra 1x13 (para ESP32) | No tocar |
| J7 | JST XH 6P (programacion) | No tocar |
| J8 | Socket hembra 1x4 (para PN532) | No tocar |
| J9, J10 | Borneras salida (electroiman, talanquera) | No tocar |
| J11, J12 | JST XH 2P (sensor, boton) | No tocar |
| J13 | JST XH 3P (no-touch) | No tocar |
| J16-J19 | JST XH 2P (distribucion 12V+GND) | No tocar |

## Lo que TU enchufas a mano

### 1. Modulo WT32-ETH01 (ESP32 + Ethernet)

```
           ETHERNET
           ┌──────┐
           │ RJ45 │  <-- Cable de red entra por aqui
           └──┬───┘
     Pin 1 ──>│ EN          TX0 │<── Pin 1
              │ GND         RX0 │
              │ 3V3(NC)     IO0 │
              │ EN          GND │
              │ CFG         IO39│
              │ 485_EN      IO36│
              │ RXD         IO15│
              │ TXD         IO14│
              │ GND         IO12│
              │ 3V3(NC)     IO35│
              │ GND         IO4 │
              │ 5V          IO2 │
    Pin 13 ──>│ LINK        GND │<── Pin 13
              └─────────────────┘
                  ANTENA WiFi
              (no tapar con metal)
```

**Como enchufar:**
1. El modulo viene con pines macho — si no, soldarle pines macho 2.54mm
2. Enchufar en J5 (fila izquierda) y J6 (fila derecha)
3. Pin 1 = lado del Ethernet (borde superior del PCB)
4. Pin 13 = lado de la antena (hacia abajo)
5. El cable Ethernet entra desde arriba, entre el modulo y el borde del PCB

### 2. Lector PN532 (RFID/NFC)

Enchufar en J8 (socket 1x4, lado derecho del PCB).

| Pin J8 | Senal | Cable PN532 |
|--------|-------|-------------|
| 1 | 3.3V | VCC |
| 2 | GND | GND |
| 3 | SDA | SDA |
| 4 | SCL | SCL |

**IMPORTANTE:** El PN532 debe estar en modo I2C (switches/jumpers del modulo).

---

## Conectores de campo (lo que va al edificio)

### J3 — Entrada 12V principal (barrel jack)

Adaptador 12V DC, minimo 3A, plug 5.5x2.1mm centro positivo.
Esta es la alimentacion principal de todo el sistema.

### J4 — Entrada 12V alternativa (bornera 5mm)

```
  J4
┌─────┐
│ + - │  12V DC y GND
└─────┘
Pin 1 = 12V (RAW_12V)
Pin 2 = GND
```

Misma alimentacion que J3, para cuando no se tiene barrel jack.
NO conectar J3 y J4 a fuentes distintas al mismo tiempo.

### J9 — Electroiman (bornera 5mm)

```
  J9
┌─────┐
│ + - │  Electroiman 12V
└─────┘
Pin 1 = Salida 12V conmutada (se apaga cuando el rele K1 activa)
Pin 2 = GND
```

**Comportamiento:** Rele K1 usa contacto NC (normalmente cerrado).
- Sin energia / falla = electroiman APAGADO = puerta ABRE (evacuacion)
- ESP32 activa rele = electroiman ENCENDIDO = puerta CERRADA

### J10 — Talanquera (bornera 5mm)

```
  J10
┌─────┐
│ C N │  Contacto seco
└─────┘
Pin 1 = COM (comun del rele K2)
Pin 2 = NO (normalmente abierto)
```

**Contacto seco:** No tiene voltaje. Conectar al controlador de la talanquera
segun su manual. Cuando el ESP32 activa K2, COM y NO se unen.

### J11 — Sensor magnetico (JST XH 2P)

```
Sensor de puerta (contacto seco, detecta si la puerta esta abierta/cerrada)

Pin 1 = SENSOR_IN (senal)
Pin 2 = GND

Cable JST XH macho 2P del sensor → enchufar en J11
```

### J12 — Boton de salida (JST XH 2P)

```
Boton de presionar para abrir desde adentro (tipo REX)

Pin 1 = BTN_IN (senal)
Pin 2 = GND

Cable JST XH macho 2P del boton → enchufar en J12
```

### J13 — Sensor no-touch (JST XH 3P)

```
Sensor infrarrojo de proximidad (salida 12V, tipo Rosslare)

Pin 1 = 12V (alimentacion del sensor)
Pin 2 = GND
Pin 3 = Senal del sensor (NT_IN)

Cable JST XH macho 3P → enchufar en J13
```

**Proteccion:** Optoacoplado (PC817). Si se cruzan cables, muere el
optoacoplador de $0.05, NO el ESP32.

### J16, J17, J18, J19 — Distribucion 12V+GND (JST XH 2P)

```
4 puertos de 12V+GND pareados para alimentar perifericos.

Pin 1 = 12V
Pin 2 = GND

Cada uno lleva un cable JST XH macho 2P al periferico.
```

Usos tipicos:
- J16 → Alimentar lector Hikvision (12V)
- J17 → Alimentar camara (12V)
- J18 → Reserva
- J19 → Reserva

### J7 — Programacion (JST XH 6P)

```
Para flashear firmware via CP2102 USB-Serial

Pin 1 = GND
Pin 2 = TX (del ESP32, conectar a RX del CP2102)
Pin 3 = RX (del ESP32, conectar a TX del CP2102)
Pin 4 = IO0 (poner a GND para modo bootloader)
Pin 5 = EN (reset)
Pin 6 = 5V
```

Cable JST XH macho 6P → hacer arnes al CP2102.
Solo se usa para desarrollo, no en produccion.

### J1 — USB-C (desarrollo)

Alternativa de alimentacion 5V para desarrollo (laptop/cargador).
NO alimenta los reles (necesitan 12V). Solo sirve para correr firmware.

---

## LED indicador

| LED | Ubicacion | Significado |
|-----|-----------|-------------|
| D3 (verde) | Centro-derecha, cerca de J6 | Encendido = hay 3.3V = sistema alimentado |

---

## Diagrama general de conexion en campo

```
                    ┌─────────────────────────┐
   Adaptador 12V ──┤ J3 (barrel) o J4 (born) │
                    │                         │
   Cable red ───────┤ WT32-ETH01 (en J5+J6)  │
                    │                         │
   Sensor puerta ──┤ J11 (JST 2P)            │
   Boton salida ───┤ J12 (JST 2P)            │
   No-touch ───────┤ J13 (JST 3P)            │
                    │                         │
   Electroiman ────┤ J9  (bornera)            │
   Talanquera ─────┤ J10 (bornera)            │
                    │                         │
   Hikvision 12V ──┤ J16 (JST 2P)            │
   Camara 12V ─────┤ J17 (JST 2P)            │
   Reserva ────────┤ J18 (JST 2P)            │
   Reserva ────────┤ J19 (JST 2P)            │
                    │                         │
   PN532 RFID ─────┤ J8  (socket 4P)         │
   CP2102 prog ────┤ J7  (JST 6P)            │
                    └─────────────────────────┘
```

## Orden de montaje

1. Recibir placas de JLCPCB (todo soldado excepto WT32-ETH01 y PN532)
2. Soldar pines macho al WT32-ETH01 (si no los trae)
3. Enchufar WT32-ETH01 en J5+J6 (pin 1 = lado Ethernet = arriba)
4. Enchufar PN532 en J8
5. Conectar adaptador 12V a J3
6. Verificar LED D3 enciende (verde = sistema OK)
7. Conectar cable Ethernet al WT32-ETH01
8. Conectar perifericos de campo (sensor, boton, electroiman, etc.)
