# Connexo v1 — Monitoreo y Diagnostico (Firmware)

Todo lo descrito aqui se implementa en firmware. NO requiere cambios
en el hardware de la placa v1. Los sensores y reles ya estan conectados.

---

## 1. Tiempo de apertura de puerta

### Que mide
Cuanto tarda la puerta en abrirse despues de dar la orden.

### Como funciona
```
t0: ESP32 activa rele K1 (J9)        → registra timestamp_orden
t1: J11 cambia de estado              → registra timestamp_abrio
    (contacto magnetico detecta que la puerta se movio)

tiempo_apertura = t1 - t0
```

### Valores de referencia

| Dispositivo | Tiempo normal | Alerta amarilla | Alerta roja |
|------------|---------------|-----------------|-------------|
| Electroiman / chapa | 0.1 - 0.5 seg | > 1 seg | > 3 seg o no abrio |
| Pestillo electrico | 0.2 - 0.8 seg | > 1.5 seg | > 3 seg |
| Talanquera | 2 - 5 seg | > 8 seg | > 15 seg |
| Motor corrediza | 5 - 15 seg | > 20 seg | > 30 seg |

### Que hacer con los datos
- Guardar historico en la base de datos (timestamp, tiempo_apertura, punto_acceso)
- Graficar tendencia: si el tiempo sube gradualmente → desgaste mecanico
- Alertar al administrador cuando se pasa del umbral amarillo
- Bloquear y alertar en rojo (posible falla mecanica o electrica)

### Implementacion en firmware (pseudocodigo)
```python
async def abrir_puerta():
    t_orden = time.ticks_ms()
    relay_k1.on()
    
    # Esperar maximo 5 segundos a que J11 cambie
    timeout = 5000
    while time.ticks_diff(time.ticks_ms(), t_orden) < timeout:
        if sensor_j11.changed():
            t_abrio = time.ticks_ms()
            tiempo = time.ticks_diff(t_abrio, t_orden)
            log_apertura(tiempo_ms=tiempo, estado="OK")
            
            if tiempo > UMBRAL_AMARILLO:
                alerta("Puerta lenta", tiempo)
            return
        
        await asyncio.sleep_ms(10)
    
    # Timeout: no abrio
    relay_k1.off()
    log_apertura(tiempo_ms=timeout, estado="FALLO")
    alerta_critica("Puerta no respondio")
```

---

## 2. Puerta no abrio (fallo de apertura)

### Que detecta
Se dio la orden de abrir pero la puerta no se movio.

### Como funciona
```
ESP32 activa rele K1 ──→ inicia timer de 5 segundos
         ...espera...
J11 NO cambia de estado ──→ ALARMA: "puerta no respondio"
```

### Causas posibles (para el tecnico)
- Electroiman sin alimentacion 12V (revisar fuente)
- Cable desconectado en J9
- Electroiman danado (bobina abierta)
- Puerta trabada mecanicamente
- Sensor J11 desconectado o danado (falso negativo)

### Accion del firmware
1. Reintentar UNA vez despues de 2 segundos
2. Si falla de nuevo: desactivar rele, registrar evento
3. Enviar alerta al panel de monitoreo central
4. Enviar notificacion push al administrador
5. El evento queda en el log con timestamp para analisis

---

## 3. Puerta quedo abierta

### Que detecta
La puerta abrio pero no cerro en el tiempo esperado.

### Como funciona
```
J11 detecta puerta ABIERTA ──→ inicia timer configurable
         ...espera...
Timer expira y J11 sigue ABIERTA ──→ ALARMA: "puerta abierta"
```

### Tiempos configurables

| Tipo de puerta | Tiempo maximo abierta |
|---------------|----------------------|
| Puerta peatonal | 30 segundos |
| Puerta de emergencia | 60 segundos |
| Talanquera vehicular | 45 segundos |
| Puerta de carga | 5 minutos |

### Escalamiento de alertas
1. **t + 0 seg**: Puerta abre (normal)
2. **t + 30 seg**: Alerta nivel 1 → notificacion al panel
3. **t + 60 seg**: Alerta nivel 2 → notificacion push al admin
4. **t + 120 seg**: Alerta nivel 3 → sirena/buzzer (si hay)

---

## 4. Talanquera no respondio

### Que detecta
Se dio pulso de apertura pero la barrera no subio.

### Como funciona
```
ESP32 activa rele K2 (J10) con pulso de 500ms
         ...espera...
J11 (conectado al auxiliar de la talanquera) no cambia
         ...timeout 15 seg...
ALARMA: "talanquera no respondio"
```

### Causas posibles
- Talanquera sin alimentacion (revisar 220V)
- Motor trabado
- Fusible quemado en la talanquera
- Cable COM/NO desconectado en J10
- Cable auxiliar desconectado en J11

---

## 5. Boton presionado pero no abrio

### Que detecta
Alguien presiono el boton de salida (J12) pero la puerta no abrio.

### Como funciona
```
J12 detecta pulsacion ──→ ESP32 activa rele K1
         ...espera...
J11 no cambia ──→ ALARMA: "boton presionado, puerta no abrio"
```

### Importancia
Esto es un tema de seguridad y evacuacion. Si alguien quiere salir
y la puerta no abre, hay que actuar inmediatamente.

---

## 6. Estadisticas para mantenimiento predictivo

### Datos que se acumulan

| Metrica | Fuente | Para que sirve |
|---------|--------|---------------|
| Ciclos de apertura/cierre | Contador por K1/K2 | Vida util del electroiman (~100K ciclos) |
| Tiempo promedio de apertura | J11 + K1 | Detectar desgaste gradual |
| Tendencia de tiempo de apertura | Historico | Predecir falla antes de que ocurra |
| Intentos fallidos | J11 + K1 | Detectar problemas recurrentes |
| Tiempo total abierta por dia | J11 | Uso real vs esperado |
| Horas de operacion | Uptime ESP32 | Programar mantenimiento preventivo |

### Ejemplo de alerta predictiva
```
Dia 1:   tiempo_apertura promedio = 0.3 seg
Dia 30:  tiempo_apertura promedio = 0.4 seg
Dia 60:  tiempo_apertura promedio = 0.6 seg  ← tendencia al alza
Dia 75:  tiempo_apertura promedio = 0.9 seg  ← ALERTA AMARILLA
         "Puerta Lobby: tiempo de apertura subiendo. 
          Programar revision mecanica."
Dia 90:  sin mantenimiento → falla
```

Con mantenimiento preventivo basado en datos, se evita la falla.

---

## 7. Dashboard de monitoreo (panel central)

Todos estos eventos se envian al backend via Ethernet/WiFi.
El panel de monitoreo muestra:

```
┌─────────────────────────────────────────────────────┐
│  CONNEXO — Panel de Monitoreo                       │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Puerta Lobby Principal          ● ONLINE  ✓ CERRADA│
│  Ultimo acceso: hace 3 min       Tiempo: 0.3s       │
│  Ciclos hoy: 47                  Estado: Normal      │
│                                                     │
│  Talanquera Parqueadero          ● ONLINE  ✓ ABAJO  │
│  Ultimo acceso: hace 12 min      Tiempo: 3.2s       │
│  Ciclos hoy: 23                  Estado: Normal      │
│                                                     │
│  Puerta Bodega                   ● OFFLINE ⚠        │
│  Ultima conexion: hace 2h        ALERTA: sin respuesta│
│                                                     │
│  ALERTAS ACTIVAS                                    │
│  ⚠ Puerta Bodega: offline hace 2 horas              │
│  ⚠ Talanquera: tiempo subiendo (3.2s, normal <3s)   │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## Expansion Rev B — Sensor de corriente

Para la version 2 de la placa se puede agregar un sensor de corriente
no invasivo (SCT-013) que se abraza al cable de alimentacion del motor.

**Pines disponibles:** IO39 (J6 pin 5), IO36 (J6 pin 6) — ADC input only.

**Que mide:**
- Corriente instantanea del motor
- Corriente promedio por ciclo
- Picos de corriente (motor trabado)

**Que se necesita agregar al board:**
- 1x conector JST XH 2P (para el cable del SCT-013)
- 1x resistencia de burden 100-220 ohm
- 1x capacitor 100nF
- 1x resistencia divisora para bias a 1.65V (mitad del ADC)

**Total: 4 componentes SMD + 1 conector.** Costo: ~$0.50.
