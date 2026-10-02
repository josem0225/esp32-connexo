# Estado del PCB Connexo — Pedido JLCPCB

## Resumen
PCB carrier board para control de acceso. Generado programaticamente con KiCad 10 + Python.
Board: 130x100mm, 2 capas, 52 componentes, 32 nets, DRC=0, ERC=0.

El usuario esta en JLCPCB, paso "Component Placements", y detecta problemas visuales
que necesitan revision con imagenes.

## Archivos clave

### Generacion (source of truth)
- `hardware/pcb-puerta/gen/design.py` — todos los componentes, footprints, nets
- `hardware/pcb-puerta/gen/placement.py` — posiciones fisicas (mm) y rotaciones
- `hardware/pcb-puerta/gen/build_pcb.py` — genera .kicad_pcb via pcbnew API
- `hardware/pcb-puerta/gen/route_pcb.py` — enruta con maze router
- `hardware/pcb-puerta/gen/check_placement.py` — valida solapes y margenes

### Fabricacion (para JLCPCB)
- `~/Documents/Nexo-ESP32/Nexo-ESP32-gerbers-jlcpcb.zip` — Gerbers (ya subidos)
- `~/Documents/Nexo-ESP32/fab/Nexo-ESP32-bom-jlcpcb.csv` — BOM con LCSC (corregido)
- `~/Documents/Nexo-ESP32/fab/Nexo-ESP32-cpl-jlcpcb.csv` — Pick-and-place/centroids
- `~/Documents/Nexo-ESP32/Nexo-ESP32.kicad_pcb` — PCB generado
- `~/Documents/Nexo-ESP32/Nexo-ESP32.kicad_sch` — Schematic generado

### Regeneracion
```bash
cd hardware/pcb-puerta/gen
PY=/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/3.9/bin/python3
$PY check_pads.py && $PY check_placement.py
python3 build.py
$PY build_pcb.py && $PY route_pcb.py
```

## Configuracion JLCPCB (ya aplicada)
- FR-4, 2 capas, 1.6mm, Green, HASL, 1oz copper
- PCB Qty: 5, PCBA Qty: 5
- PCBA Type: Standard (SMD + THT)
- Assembly Side: Top
- Confirm Parts Placement: Yes
- Stencil Storage: Yes, Fixture Storage: Yes

## BOM — LCSC Part Numbers (verificados 2026-10-01)

| Ref | Valor | Footprint | LCSC | Estado |
|-----|-------|-----------|------|--------|
| R1,R2 | 5.1kΩ | 0805 | C84375 | OK |
| R3,R7,R9 | 1kΩ | 0805 | C17513 | OK Basic |
| R4,R5 | 4.7kΩ | 0805 | C17673 | OK Basic |
| R6,R8,R10,R12 | 10kΩ | 0805 | C17414 | OK Basic |
| R11 | 2.2kΩ | 0805 | C17520 | OK Basic |
| C2,C5-C10 | 100nF | 0805 | C49678 | OK Basic |
| C3,C4 | 10uF | 0805 | C15850 | OK Basic |
| C1,C11,C12 | 100uF 16V | 1210 | C394395 | OK (Taiyo Yuden) |
| D1,D4-D6 | SS34 | SMA | C8678 | OK Basic |
| D3 | LED verde | 0805 | C2297 | OK Basic |
| F1 | 1.5A PTC | 1812 | C207070 | OK |
| F2 | 3A PTC | 1812 | C2760295 | OK |
| U1 | AMS1117-3.3 | SOT-223 | C6186 | OK Basic |
| U2 | ULN2003A | SOP-16 | C181730 | OK |
| U4 | LM2596S-5.0 | TO-263-5 | C347421 | OK |
| L1 | 33uH 3A | 12x12mm | C2929502 | OK |
| U3 | PC817C | DIP-4 | C3008368 | OK |
| K1,K2 | SRD-05VDC-SL-C | Relay_SPDT | C35449 | OK |
| J1 | USB-C 6P | USB-C | C3151650 | ⚠️ Qty:0 pre-order |
| J3 | Barrel Jack | BarrelJack | C381116 | OK |
| J4,J9,J10 | 2P Terminal 5mm | 5.0mm | C474881 | OK (potencia, campo) |
| J11,J12,J16-J19 | JST XH 2P | 2.50mm | C158012 | OK (325K stock) |
| J13 | JST XH 3P | 2.50mm | C144394 | OK (129K stock) |
| J5,J6 | 1x13 Pin Socket | 2.54mm | C2897376 | OK |
| J8 | 1x4 Pin Socket | 2.54mm | C124413 | OK |
| J7 | 1x6 Pin Header | 2.54mm | C37208 | OK |

## Layout del board (130x100mm)

```
Board origin: (100, 100) en KiCad. Board: X=100..230, Y=100..200

Enfoque mixto: bornera 5mm para potencia (J4,J9,J10), JST XH para señal y distribucion.

         X=100                                              X=230
    Y=100 ┌─────────────────────────────────────────────────┐
          │  J1    J3  J4   F2  C11  U4  D5  L1  C12  D6   │ ← alimentacion 12V/USB
          │  R1 R2 D1  F1 C1 C2 C3 U1 C4 C5 R3 D3         │ ← riel 5V y 3.3V
     J11──│  C8 R7,R8  J7  J5─────J6      J8 R4,R5,C7      │  J11,J12,J13 = JST XH
     J12──│  C9 R9,R10     │  ESP32  │                K1 D4 │──J9  (bornera)
          │                │  area   │     U2  R6           │
     J13──│  R11 U3 R12,C10└────────┘           K2          │──J10 (bornera)
          │  J16 J17 J18 J19                                │  J16-J19 = JST XH 2P
    Y=200 └─────────────────────────────────────────────────┘  (V12+GND pareado)
```

## Posiciones exactas (de placement.py)

### ESP32 area
- J5 (fila A): (140.0, 130.0) — pin socket 1x13, pins de Y=130 a Y=160.48
- J6 (fila B): (162.86, 130.0) — pin socket 1x13, 22.86mm a la derecha de J5
- J7 (PROG): (128.0, 132.0) — header 1x6, a la IZQUIERDA de J5

### Modulo WT32-ETH01 (NO ensamblado, el usuario lo enchufa)
- Dimension: ~55mm x 25mm
- Se enchufa en J5 y J6 (elevado 8.5mm por sockets)
- Ethernet jack: sale por el extremo pin-1 (Y=130, hacia arriba)
- Antena: extremo pin-13 (Y=160, hacia abajo)
- Area cubierta estimada: X=139..164, Y=~118..~180

### Componentes BAJO el modulo ESP32 (clearance vertical ~6.7mm)
- U1 (AMS1117) en (151, 120) — 1.8mm alto, en zona del Ethernet overhang
- C3 en (144, 124) — 1mm alto
- C4 en (158, 124) — 1mm alto
- U2 (ULN2003A) en (148, 165) — 1.75mm alto, en zona de antena overhang
- R6 en (142, 173) — 0.5mm alto

### Componentes de borde (entradas/salidas)
- J4 (12V bornera 5mm): (153, 106) — borde superior
- J9 (electroiman bornera 5mm): (206, 165) — borde derecho
- J10 (talanquera bornera 5mm): (206, 185) — borde derecho
- J11 (sensor JST XH 2P): (104.6, 132) — borde izquierdo
- J12 (boton JST XH 2P): (104.6, 150) — borde izquierdo
- J13 (no-touch JST XH 3P): (104.6, 172) — borde izquierdo
- J16 (DIST 12V+GND #1, JST XH 2P): (113, 192) — borde inferior
- J17 (DIST 12V+GND #2, JST XH 2P): (124, 192) — borde inferior
- J18 (DIST 12V+GND #3, JST XH 2P): (135, 192) — borde inferior
- J19 (DIST 12V+GND #4, JST XH 2P): (146, 192) — borde inferior

## Enfoque mixto de conectores (decision 2026-10-01)

Bornera 5mm para potencia y cable pelado de campo (J4, J9, J10).
JST XH 2.50mm para señales y distribucion (J11, J12, J13, J16-J19).

Razones:
- Potencia: el técnico trae cable pelado 18-22 AWG y destornillador.
  JST XH solo acepta 22-28 AWG y requiere crimpar. Bornera es mejor.
- Señales: μA, polarizado evita errores, cables pre-armados disponibles.
- Distribucion: JST XH pareados (V12+GND en cada conector) eliminan
  el problema de cablear rieles separados. Un cable por periferico.

## REVISION VISUAL (2026-10-01, capturas JLCPCB)

### 1. Componentes sobre J5/J6 — OK
Perspectiva del viewer 3D. Sin solape real: C3/C4/U1 a 6-10mm del pin-1,
U2/R6 a 4.5-12.5mm del pin-13. Validado por check_placement.py con
envelope ESP32 (X=139-164, Y=118-180).

### 2. "Regleta grande" — Eran J5/J6
Los pin sockets 1x13 del WT32-ETH01 (8.5mm de alto). Correcto.

### 3. Ethernet clearance — OK
30mm de J5 pin-1 al borde superior. Componentes intermedios quedan
bajo el modulo elevado (6.7mm de clearance vertical).

### 4. Borneras fuera del borde — ELIMINADO
J11/J12/J13 ahora son JST XH (mas pequeños, no sobresalen).
J4/J9/J10 siguen como bornera, overhang normal.

### 5. J1 USB-C sin stock — PENDIENTE
C3151650 con Qty:0 (pre-order). Placa funciona sin el.

## Uso alternativo de J8: perifoneo / audio (DFPlayer Mini)

J8 (1x4 pin socket, 2.54mm) esta diseñado para el PN532 (RFID por I2C),
pero puede reutilizarse para un modulo DFPlayer Mini (audio/perifoneo)
cuando esa puerta no necesita lector RFID.

### Conexion DFPlayer Mini en J8

| Pin J8 | Señal original | Uso DFPlayer |
|--------|---------------|--------------|
| 1 | 3.3V | VCC |
| 2 | GND | GND |
| 3 | IO33 (I2C SDA) | RX del DFPlayer (SoftwareSerial TX) |
| 4 | IO17 (I2C SCL) | TX del DFPlayer (SoftwareSerial RX) |

### Que hace falta
- DFPlayer Mini (~$2 USD) — reproduce MP3 desde microSD
- Parlante 8 ohm 2W (~$1-3 USD) — conectado directo al DFPlayer
- MicroSD con archivos de audio (alarmas, "acceso concedido", perifoneo)
- CERO modificaciones al PCB — mismo conector J8

### Logica en firmware
Cada puerta se configura segun necesidad:
- **Puerta con RFID:** firmware inicializa I2C + PN532 en IO33/IO17 via J8
- **Puerta con perifoneo:** firmware inicializa SoftwareSerial + DFPlayer en IO33/IO17 via J8
- **Puerta con ambos (v2):** se agrega conector dedicado en la proxima revision del PCB

### Casos de uso
- Alarma sonora por apertura forzada o puerta abierta mucho tiempo
- Perifoneo desde el panel de despacho (audio pre-grabado o streaming)
- Confirmacion audible de acceso concedido/denegado
- Timbre de visitante

## QA antes de ordenar

1. LCSC verificados: cada numero buscado en lcsc.com, NO inventado
2. Envelope ESP32 validado en check_placement.py (zona X=139-164, Y=118-180)
3. CPL usa Mid X, Mid Y (centro geometrico) — verificar al regenerar
4. **PENDIENTE: imprimir Gerber 1:1 y poner WT32-ETH01 encima**
5. Regenerar TODO tras cambios: schematic, PCB, Gerbers, BOM, CPL

## Precio estimado (solo componentes, 5 boards)
- Capacitores 100uF: $9.39
- Reles: $3.43
- LM2596S: $3.02
- Conectores JST XH + borneras: ~$8
- Resto (R, C, D, ICs): ~$5
- Total componentes: ~$30
- PCB: ~$11
- Envio DHL: ~$42
- **Total estimado: ~$83 USD** (sin ensamblaje fee)
