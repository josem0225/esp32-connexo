# Valoracion del Activo: PCB Connexo Controller v1

Fecha: 2026-10-01
Empresa: NEXO GROUP SOLUTIONS S.A.S (NIT 902.101.794-5)
Marca: Connexo

## Descripcion del activo

Diseno completo de una PCB carrier board para control de acceso biometrico,
incluyendo esquematico, layout, toolchain de generacion, archivos de fabricacion
y documentacion. Listo para manufactura en JLCPCB.

### Especificaciones tecnicas
- Board: 130x100mm, 2 capas, FR-4
- Componentes: 52 (SMD + THT mixto)
- Nets: 32
- DRC: 0 errores, ERC: 0 errores
- Bloques funcionales: 6 (alimentacion, ESP32, RFID, reles, entradas, distribucion)

### Bloques de circuito incluidos
1. **Entrada de alimentacion dual** — Barrel jack 12V + USB-C 5V con diodo OR
2. **Buck converter** — LM2596S-5 (12V→5V, 3A) con circuito de aplicacion completo
3. **LDO** — AMS1117-3.3 (5V→3.3V) para PN532
4. **Driver de reles** — ULN2003A + 2x SRD-05VDC-SL-C (electroiman + talanquera)
5. **Entradas de campo** — sensor magnetico, boton, no-touch con optoacoplador PC817
6. **Distribucion** — regletas de 12V y GND para perifericos

### Entregables producidos
- Schematic (.kicad_sch) — 52 componentes, 32 nets, validado ERC=0
- PCB layout (.kicad_pcb) — 2 capas, ruteo completo, DRC=0
- Gerbers JLCPCB (.zip) — listos para fabricacion
- BOM con LCSC part numbers verificados (.csv) — 27 lineas, todos en stock
- CPL pick-and-place (.csv) — 52 posiciones
- Toolchain Python de generacion programatica (8 scripts)
- Documentacion tecnica (manual bilingue, package de fabricacion)

## Desglose de horas equivalentes

### 1. Diseno de esquematico
| Tarea | Horas |
|-------|-------|
| Arquitectura del circuito (bloques, rieles, protecciones) | 8 |
| Seleccion de componentes y verificacion de datasheets | 6 |
| Esquematico completo (52 componentes, 32 nets) | 10 |
| Verificacion ERC y correccion de errores | 4 |
| **Subtotal** | **28** |

### 2. Diseno de PCB layout
| Tarea | Horas |
|-------|-------|
| Placement optimizado por zonas funcionales | 8 |
| Ruteo 2 capas (31 nets, ground plane) | 12 |
| Verificacion DRC iterativa (61→15→3→0 errores) | 8 |
| Optimizacion de ground stitching y thermal relief | 4 |
| Verificacion de clearances con modulo ESP32 | 4 |
| **Subtotal** | **36** |

### 3. BOM engineering y sourcing
| Tarea | Horas |
|-------|-------|
| Busqueda y verificacion de 27 LCSC part numbers | 6 |
| Verificacion de stock y alternativas | 3 |
| Correccion de 11 errores de LCSC (encontrados en JLCPCB) | 4 |
| Optimizacion de costos (capacitores, alternativas) | 2 |
| **Subtotal** | **15** |

### 4. Toolchain de generacion programatica (IP unico)
| Script | Funcion | Horas |
|--------|---------|-------|
| design.py | Definicion completa del circuito (source of truth) | 10 |
| placement.py | Posiciones fisicas optimizadas | 4 |
| schematic.py | Generador de .kicad_sch | 8 |
| build.py | Orquestador con validaciones | 4 |
| build_pcb.py | Generador de .kicad_pcb via pcbnew API | 12 |
| route_pcb.py | Ruteo automatico + ground stitching | 8 |
| router.py | Maze router Dijkstra 2-layer con vias | 16 |
| check_pads.py | Validador de pines vs footprints | 4 |
| check_placement.py | Validador de solapes y margenes | 4 |
| symbols.py + sexpr.py | Parser de librerias KiCad + extends | 8 |
| **Subtotal** | **78** |

### 5. Documentacion y fabricacion
| Tarea | Horas |
|-------|-------|
| Manual bilingue (HTML) con BOM y checklist | 4 |
| Package de fabricacion para JLCPCB | 3 |
| Generacion de Gerbers, BOM JLCPCB, CPL | 2 |
| Proceso de ordering en JLCPCB (configuracion, revision) | 3 |
| **Subtotal** | **12** |

### 6. Debug e iteracion
| Tarea | Horas |
|-------|-------|
| Resolucion de DRC clearance violations (61 errores) | 6 |
| Fix de footprint mismatches (relay, USB-C, TO-263) | 4 |
| Fix de symbol extends (ULN2003A, AMS1117, relay) | 4 |
| Resolucion de ground plane islands y thermal relief | 3 |
| Redimensionamiento del board (100x100 → 130x100) | 2 |
| Verificacion de pin spacing WT32-ETH01 (22.86mm) | 2 |
| **Subtotal** | **21** |

---

### Total horas equivalentes: 190 horas

## Valoracion economica

### Por tarifa de mercado

| Perfil | Tarifa/hora | Total USD |
|--------|------------|-----------|
| Firma de diseno USA | $180-250 | $34,200 - $47,500 |
| Ingeniero senior freelance (Upwork) | $75-120 | $14,250 - $22,800 |
| Ingeniero electronico Colombia senior | $36-60 | $6,840 - $11,400 |
| Freelancer offshore (India/China) | $25-45 | $4,750 - $8,550 |

### Valoracion recomendada del activo

| Componente | Valor conservador | Valor de mercado |
|------------|-------------------|------------------|
| Diseno de circuito (schematic + layout) | $4,000 | $8,000 |
| Toolchain de generacion (IP reutilizable) | $5,000 | $12,000 |
| BOM engineering + sourcing | $1,000 | $2,500 |
| Documentacion + fab package | $500 | $1,500 |
| Debug/iteracion (know-how) | $1,000 | $2,500 |
| **TOTAL** | **$11,500 USD** | **$26,500 USD** |

### En pesos colombianos (TRM ~4,200 COP/USD)
| | Conservador | Mercado |
|---|---|---|
| **Total COP** | **$48,300,000** | **$111,300,000** |

## Valor diferencial del toolchain

El toolchain de generacion programatica (78 horas, ~41% del esfuerzo) es IP unico
que NO existe en el mercado. Permite:

1. **Regenerar el PCB completo en segundos** — un cambio en design.py regenera
   schematic, PCB, ruteo, Gerbers automaticamente
2. **Iterar sin GUI** — no depende de un ingeniero moviendo componentes a mano
   en KiCad/Altium/Eagle
3. **Reproducible y versionable** — todo en git, diff legible, CI posible
4. **Reutilizable** — adaptar a nuevos boards (vehicular, ascensor, etc.) cambiando
   solo design.py y placement.py

Comparable en el mercado: herramientas como Atopile (~$50K+ de inversion VC) o
SKiDL ($0 open source pero sin layout). Nuestro toolchain hace schematic + layout
+ routing, lo que ninguna herramienta open source ofrece completo.

## Contexto competitivo

| Solucion | Costo por puerta | Nuestro PCB |
|----------|-----------------|-------------|
| Hikvision DS-K2604T | ~$430 USD / puerta | — |
| ZKTeco C3-200 | ~$200 USD / puerta | — |
| **Connexo PCB + ESP32** | — | **~$17 USD / puerta** |

El costo de fabricacion por unidad (PCB + componentes + ensamblaje) es ~$17 USD,
lo que da un margen bruto de >95% sobre el precio de mercado de controladores
comparables.

## Notas

- Valoracion basada en horas equivalentes de desarrollo, no en costo de fabricacion
- El toolchain tiene valor creciente: cada nuevo producto que se genere con el
  amortiza la inversion inicial
- No incluye el firmware del ESP32 (activo separado)
- No incluye el costo de fabricacion de las 5 unidades (~$87 USD)
- Fuentes de precios: Upwork, Fiverr, Cad Crowd, ERI Economic Research Institute

## Fuentes
- [Fiverr — Electronics Design Engineer Costs](https://www.fiverr.com/resources/guides/costs/electronics-design-engineer)
- [Upwork — PCB Designers for Hire](https://www.upwork.com/hire/pcb-designers/)
- [Cad Crowd — PCB Design Outsourcing Costs](https://www.cadcrowd.com/blog/how-much-does-it-cost-to-outsource-pcb-design-services-electronics-engineering-complete-prices-for-companies/)
- [ERI — Electronics Design Engineer Salary Colombia](https://www.erieri.com/salary/job/electronics-design-engineer/colombia)
- [Circuit Board Design — PCB Design Cost 2026](https://www.circuit-board-design.com/blog/how-much-does-pcb-design-cost-in-2026-a-real-pricing-guide-rctf)
