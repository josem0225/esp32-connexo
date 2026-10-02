# Connexo v1 — Pinout Completo y Opciones de Conexion

## Resumen del board

Carrier board para control de acceso. Un board por punto de acceso.
Controla UNA puerta/talanquera/motor. Si hay 3 puertas, se usan 3 boards.

- Board: 130x100mm, 2 capas
- Cerebro: WT32-ETH01 (ESP32 + Ethernet + WiFi + Bluetooth)
- Alimentacion: 12V DC (adaptador universal, minimo 3A)
- 2 reles de salida (chapa + talanquera/motor)
- 3 entradas de campo (sensor puerta, boton salida, no-touch)
- 4 puertos de distribucion 12V
- DIP switch + boton RESET para flasheo

---

## ENTRADAS DE ALIMENTACION

### J3 — Barrel Jack 12V (entrada principal)

| Dato | Valor |
|------|-------|
| Tipo | Barrel jack 5.5x2.1mm, centro positivo |
| Voltaje | 12V DC |
| Corriente minima | 3A |
| Que conectar | Adaptador de pared 12V 3A o fuente switching |

Este es el punto principal de alimentacion de toda la placa.
De aqui sale la cadena: 12V → buck → 5V → LDO → 3.3V.

### J4 — Bornera 12V (entrada alternativa)

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | **12V** | Positivo de la fuente |
| 2 | **GND** | Negativo / tierra |

Misma funcion que J3 pero con bornera de tornillo.
Usar cuando no se tiene barrel jack (por ejemplo, fuente centralizada del edificio).
**NO conectar J3 y J4 a fuentes distintas al mismo tiempo.**

### J1 — USB-C 5V (solo desarrollo)

| Dato | Valor |
|------|-------|
| Tipo | USB-C 6 pines |
| Voltaje | 5V (del laptop o cargador) |
| Uso | Flashear firmware sin tener fuente 12V |

**No alimenta los reles.** Solo sirve para desarrollo. En campo se usa J3 o J4.

---

## SALIDAS DE POTENCIA (RELES)

La placa tiene 2 reles SPDT (SRD-05VDC-SL-C) controlados por el ESP32
a traves del ULN2003A. Cada rele puede manejar hasta 10A @ 250VAC.

### J9 — Salida Rele K1 (Chapa Electrica / Electroiman)

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | **12V** | Salida 12V conmutada (contacto NC del rele) |
| 2 | **GND** | Tierra comun |

**Tipo de conector:** Bornera de tornillo 5mm (cable pelado, atornillar).

**Comportamiento (fail-safe):**
- Rele K1 usa contacto **NC** (normalmente cerrado)
- Sin energia / falla de corriente → rele desactivado → circuito CERRADO → electroiman ENERGIZADO → puerta CERRADA... NO.
- Correccion: sin energia → NO hay 12V → electroiman sin corriente → puerta **ABRE** (evacuacion)
- ESP32 activa rele → contacto NC se ABRE → electroiman sin corriente → puerta ABRE
- ESP32 desactiva rele → contacto NC CIERRA → 12V al electroiman → puerta CERRADA

**Que se puede conectar aqui:**

| Dispositivo | Tipo | Conexion |
|------------|------|----------|
| Electroiman 12V (chapa magnetica) | 300-800mA | Pin1=12V al electroiman, Pin2=GND al electroiman |
| Cerradura electrica 12V | 200-500mA | Igual |
| Pestillo electrico 12V | 100-300mA | Igual |

### J10 — Salida Rele K2 (Talanquera / Motor)

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | **COM** | Comun del rele (polo central) |
| 2 | **NO** | Normalmente abierto |

**Tipo de conector:** Bornera de tornillo 5mm.

**Comportamiento:**
- Contacto SECO (sin voltaje propio, aislado galvanicamente)
- ESP32 activa rele → COM y NO se unen → se cierra el circuito
- ESP32 desactiva rele → COM y NO se separan → circuito abierto

**Que se puede conectar aqui:**

| Dispositivo | Conexion | Notas |
|------------|----------|-------|
| Talanquera (ZKTeco, CAME, FAAC) | COM y NO a los bornes "OPEN" de la talanquera | Consultar manual de la talanquera para identificar bornes |
| Motor de puerta corrediza | COM y NO a los bornes "START" del controlador del motor | El motor tiene su propio controlador; nuestra placa solo da el pulso |
| Motor de puerta batiente | COM y NO a los bornes de activacion | Igual que corrediza |
| Cualquier dispositivo que necesite un "pulso seco" | COM y NO | Funciona como un boton: cierra circuito cuando el ESP32 lo ordena |

**IMPORTANTE:** El contacto seco no da voltaje. El dispositivo conectado
debe tener su propia alimentacion. Nuestra placa solo cierra/abre el circuito.

---

## ENTRADAS DE CAMPO

### J11 — Contacto Magnetico / Sensor de Estado

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | **CONT** | Cable del sensor (senal) |
| 2 | **GND** | Tierra |

**Tipo de conector:** JST XH 2P (polarizado, no se puede enchufar al reves).

**Proteccion:** Resistencia serie de 1k + pull-up 10k a 3.3V + cap 100nF antirrebote.
Si alguien mete 12V por error: 12V/1k = 12mA, lo aguantan los diodos de proteccion del ESP32.

**Que se puede conectar aqui:**

| Dispositivo | Tipo | Como funciona |
|------------|------|---------------|
| Sensor magnetico de puerta | Contacto seco NC | Pegado en el marco; detecta si la puerta esta abierta o cerrada. Circuito cerrado = puerta cerrada |
| Contacto auxiliar de talanquera | Contacto seco | Toda talanquera comercial trae bornes AUX/LIMIT. Se cierra cuando la barrera esta arriba |
| Contacto auxiliar de motor | Contacto seco | Final de carrera del motor. Se cierra cuando la puerta esta completamente abierta |
| Cualquier contacto seco | NC o NO | El firmware se configura para interpretar NC o NO |

**Caso de uso tipico — talanquera:**
1. Abrir caja de control de la talanquera
2. Buscar bornes marcados "AUX", "LIMIT SWITCH" o "FEEDBACK"
3. Sacar 2 cables de ahi
4. Enchufar en J11
5. En el firmware: saber si la barrera esta arriba o abajo

### J12 — Boton de Salida (REX)

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | **BTN** | Cable del boton (senal) |
| 2 | **GND** | Tierra |

**Tipo de conector:** JST XH 2P.

**Proteccion:** Igual que J11 (1k serie + 10k pull-up + 100nF).

**Que se puede conectar aqui:**

| Dispositivo | Tipo | Notas |
|------------|------|-------|
| Pulsador de salida (REX) | Contacto momentaneo NO | El tipico boton verde de "presione para salir" |
| Pulsador de emergencia | Contacto momentaneo | Para abrir la puerta desde adentro |
| Interruptor de llave | Contacto momentaneo o sostenido | Para personal autorizado |

### J13 — Sensor No-Touch (Infrarrojo)

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | **12V** | Alimentacion del sensor (12V) |
| 2 | **GND** | Tierra |
| 3 | **OUT** | Senal de salida del sensor |

**Tipo de conector:** JST XH 3P.

**Proteccion:** Optoacoplado con PC817. Si se cruzan cables, muere
el optoacoplador de $0.05, NO el ESP32. Aislamiento galvanico completo.

**Que se puede conectar aqui:**

| Dispositivo | Tipo | Notas |
|------------|------|-------|
| Sensor no-touch infrarrojo (Rosslare, ZKTeco) | Salida 12V activa | Pin1 alimenta el sensor, Pin3 recibe la senal cuando detecta mano |
| Sensor de movimiento PIR 12V | Salida 12V | Para abrir automaticamente al detectar persona |
| Cualquier sensor 12V con salida activa | NPN o PNP | Verificar polaridad segun tipo |

---

## DISTRIBUCION DE 12V (JST XH 2P)

4 puertos identicos para alimentar perifericos del sistema.
Cada uno lleva 12V + GND en un solo conector polarizado.

### J16, J17, J18, J19

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | **12V** | Positivo 12V |
| 2 | **GND** | Tierra |

**Tipo de conector:** JST XH 2P (polarizado).

**Que se puede conectar aqui:**

| Puerto | Uso sugerido | Corriente max |
|--------|-------------|---------------|
| J16 | Alimentar lector Hikvision DS-K1T344 | ~1A |
| J17 | Alimentar camara IP 12V | ~0.5A |
| J18 | Alimentar sensor no-touch (si no usa J13) | ~0.1A |
| J19 | Reserva | — |

**NOTA:** La corriente total de todos los puertos J16-J19 sale de la fuente
principal (J3/J4). Si se conectan muchos perifericos, usar fuente de mas amperaje.

---

## MODULO ESP32 (WT32-ETH01)

### J5 — Fila A (izquierda) y J6 — Fila B (derecha)

**Tipo:** Pin socket hembra 1x13, paso 2.54mm. Viene soldado por JLCPCB.

El modulo WT32-ETH01 se enchufa aqui a mano. **No lo ensambla JLCPCB.**

| J5 Pin | Nombre | Funcion en nuestro circuito |
|--------|--------|---------------------------|
| 1 | EN | Reset (conectado a SW2) |
| 2 | GND | Tierra |
| 3 | 3V3 | **NO CONECTAR** (LDO propio del modulo) |
| 4 | EN | Reset (duplicado) |
| 5 | CFG/IO32 | Control rele K2 (talanquera/motor) |
| 6 | 485_EN/IO33 | I2C SDA (lector PN532) |
| 7 | RXD/IO5 | **LIBRE** — reserva para futuro |
| 8 | TXD/IO17 | I2C SCL (lector PN532) |
| 9 | GND | Tierra |
| 10 | 3V3 | **NO CONECTAR** |
| 11 | GND | Tierra |
| 12 | 5V | Alimentacion 5V desde la placa |
| 13 | LINK | **NO CONECTAR** (indicador LED Ethernet) |

| J6 Pin | Nombre | Funcion en nuestro circuito |
|--------|--------|---------------------------|
| 1 | TX0/IO1 | UART TX (programacion, J7) |
| 2 | RX0/IO3 | UART RX (programacion, J7) |
| 3 | IO0 | Boot mode (conectado a SW1) |
| 4 | GND | Tierra |
| 5 | IO39 | **LIBRE** — ADC, input only. Futuro: sensor corriente |
| 6 | IO36 | **LIBRE** — ADC, input only. Futuro: sensor corriente |
| 7 | IO15 | **LIBRE** — GPIO reserva |
| 8 | IO14 | Entrada boton salida (J12) |
| 9 | IO12 | **NUNCA USAR** (strapping pin, causa boot loop) |
| 10 | IO35 | Entrada no-touch (J13) |
| 11 | IO4 | Entrada sensor magnetico (J11) |
| 12 | IO2 | Control rele K1 (chapa electrica) |
| 13 | GND | Tierra |

### Pines libres para expansion futura

| Pin | GPIO | Tipo | Uso potencial |
|-----|------|------|---------------|
| J5-7 | IO5 | I/O | RS-485, tercer rele, o sensor |
| J6-5 | IO39 | Input only, ADC | Sensor de corriente (SCT-013) para monitoreo de motor |
| J6-6 | IO36 | Input only, ADC | Segundo sensor de corriente o sensor de temperatura |
| J6-7 | IO15 | I/O | Tercer rele, LED de estado, o buzzer |

---

## PROGRAMACION Y DEPURACION

### J7 — Conector de Programacion (CP2102)

| Pin | Nombre | Conectar a CP2102 |
|-----|--------|-------------------|
| 1 | GND | GND |
| 2 | TX | RX del CP2102 (cruzado) |
| 3 | RX | TX del CP2102 (cruzado) |
| 4 | IO0 | No conectar (lo maneja SW1) |
| 5 | EN | No conectar (lo maneja SW2) |
| 6 | 5V | 5V (alimenta la placa desde USB del laptop) |

**Tipo de conector:** JST XH 6P.

**Solo se necesitan 4 cables:** GND, TX, RX, 5V.
Los pines IO0 y EN ya se controlan con SW1 y SW2.

### SW1 — DIP Switch (FLASH / NORMAL)

| Posicion | Significado |
|----------|-------------|
| Switch 1 ARRIBA | **NORMAL** — operacion normal |
| Switch 1 ABAJO | **FLASH** — modo bootloader para cargar firmware |
| Switch 2 | Reserva (no hace nada) |

### SW2 — Boton RESET

Presionar para reiniciar el ESP32. Se usa durante flasheo y para
resolver problemas sin desconectar la corriente.

### Procedimiento de flasheo

1. Conectar cable JST XH 6P de J7 al CP2102
2. Conectar CP2102 al laptop por USB
3. Bajar SW1 posicion 1 a FLASH
4. Presionar SW2 (RESET)
5. En el laptop: `esptool.py` o PlatformIO flash
6. Subir SW1 posicion 1 a NORMAL
7. Presionar SW2 (RESET)
8. Listo — firmware corriendo

Despues del primer flash, se puede actualizar por OTA (WiFi/Ethernet)
sin tocar la placa.

---

## LECTOR RFID/NFC

### J8 — Modulo PN532 (I2C)

| Pin | Nombre | Funcion |
|-----|--------|---------|
| 1 | 3.3V | Alimentacion del PN532 |
| 2 | GND | Tierra |
| 3 | SDA | Datos I2C |
| 4 | SCL | Reloj I2C |

**Tipo de conector:** Pin socket hembra 1x4.

**IMPORTANTE:** El PN532 debe estar configurado en modo I2C
(configurar switches/jumpers del modulo PN532 segun su manual).

Pull-ups I2C de 4.7k ya estan en la placa (R4, R5). No agregar externos.

---

## INDICADORES

| Componente | Color | Significado |
|-----------|-------|-------------|
| D3 (LED) | Verde | Encendido = hay 3.3V = sistema alimentado correctamente |
| WT32-ETH01 LED | Azul | Parpadea = Ethernet link activo |

---

## EXPANSION FUTURA (Rev B)

### Sensor de corriente para monitoreo predictivo

El ESP32 tiene 2 pines ADC libres (IO39, IO36) que pueden leer un sensor
de corriente tipo SCT-013 (pinza no invasiva).

**Como funciona:**
- La pinza SCT-013 se abraza al cable de alimentacion del motor/talanquera
- No se corta ni desconecta nada
- La pinza genera una senal proporcional a la corriente
- El ESP32 lee esa senal y detecta anomalias (corriente excesiva = desgaste)

**Que se necesita para rev B:**
- 1x conector JST XH 2P adicional
- 1x resistencia de burden (100-200 ohm)
- 1x capacitor de filtro (100nF)
- Calibracion en firmware

### Tercer rele

IO5 e IO15 estan libres y el ULN2003A tiene 5 canales sin usar.
Se puede agregar un tercer rele para controlar otro dispositivo
(segunda chapa, luz de semaforo, sirena, etc.).

---

## DIAGRAMA DE CONEXION EN CAMPO

```
INTERIOR DEL EDIFICIO                    PLACA CONNEXO                     EXTERIOR
                                   ┌─────────────────────┐
                                   │                     │
 Adaptador 12V 3A ────────────────┤ J3 (barrel jack)    │
                                   │                     │
 Cable de red (switch) ───────────┤ WT32-ETH01 (J5+J6)  │
                                   │                     │
 Chapa electrica ─────────────────┤ J9  (12V / GND)     │
   o electroiman                   │                     │
                                   │                     │
 Talanquera (bornes OPEN) ────────┤ J10 (COM / NO)      │
   o motor (bornes START)          │                     │
                                   │                     │
 Sensor puerta (contacto mag.) ───┤ J11 (CONT / GND)    │
   o auxiliar talanquera           │                     │
                                   │                     │
 Boton de salida (REX) ───────────┤ J12 (BTN / GND)     │
                                   │                     │
 Sensor no-touch IR ──────────────┤ J13 (12V/GND/OUT)   │
                                   │                     │
 Lector Hikvision 12V ────────────┤ J16 (12V / GND)     │
                                   │                     │
 Camara IP 12V ───────────────────┤ J17 (12V / GND)     │
                                   │                     │
 Reserva ─────────────────────────┤ J18 (12V / GND)     │
                                   │                     │
 Reserva ─────────────────────────┤ J19 (12V / GND)     │
                                   │                     │
 Lector PN532 RFID ───────────────┤ J8  (3V3/GND/I2C)   │
                                   │                     │
 CP2102 (solo desarrollo) ────────┤ J7  (UART 6P)       │
                                   └─────────────────────┘
```

## Tipos de punto de acceso soportados

### Puerta peatonal con chapa electrica
- J9 → chapa electrica 12V
- J11 → sensor magnetico en el marco
- J12 → boton de salida REX
- J13 → sensor no-touch (opcional)

### Puerta peatonal con pestillo electrico
- J9 → pestillo electrico 12V
- J11 → sensor magnetico
- J12 → boton de salida

### Talanquera vehicular
- J10 → bornes OPEN de la talanquera
- J11 → contacto auxiliar de la talanquera (saber si esta arriba)
- J16 → alimentar lector Hikvision del vehicular

### Puerta corrediza con motor
- J10 → bornes START del controlador del motor
- J11 → final de carrera del motor (saber si esta abierta)
- J12 → boton de apertura manual

### Talanquera + motor en el mismo punto
- J9 → chapa o motor secundario (contacto NC, 12V)
- J10 → talanquera (contacto seco COM/NO)
- J11 → auxiliar de la talanquera

### Integrado con Hikvision DS-K1T344
- J16 → alimentar el Hikvision (12V)
- J9 → chapa electrica
- J11 → sensor magnetico
- ESP32 se comunica con Hikvision via ISAPI por Ethernet
- Hikvision maneja biometria, ESP32 maneja logica de puerta
