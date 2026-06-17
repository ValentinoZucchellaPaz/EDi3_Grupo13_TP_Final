# 🎯 RADAR ULTRASÓNICO INTELIGENTE CON SONAR ACÚSTICO
## Descripción Completa del Proyecto

---

## 📌 OBJETIVO GENERAL

Diseñar e implementar un **sistema de radar ultrasónico inteligente** que utiliza un **sensor de distancia HC-SR04 montado sobre un servomotor** para realizar barridos de 180° y detectar obstáculos. El sistema permite **control dual (manual/automático)**, captura de datos en tiempo real, y **retroalimentación acústica tipo sonar** mediante un buzzer. Los datos se transmiten a una computadora vía UART+DMA para reconstruir la imagen del radar en una interfaz gráfica.

---

## 🔧 COMPONENTES HARDWARE

### **Sensor de Entrada**
- **HC-SR04** (Sensor ultrasónico)
  - Rango: 2 cm - 400 cm
  - Precisión: ±3 cm
  - Frecuencia: 40 kHz
  - Pines: TRIG (disparo), ECHO (lectura)

- **Joystick analógico**
  - Eje X: Control manual de ángulo (0-4095 cuentas ADC)
  - Zona muerta: ±400 cuentas (previene ruido)

### **Actuadores de Salida**
- **Servomotor SG90**
  - Rango: 0° a 180°
  - Voltaje: 5V
  - Control: PWM 50Hz (20ms período)
  - Resolución: 0.5ms/10µs por grado

- **Buzzer** (controlado por DAC)
  - Suena al detectar objetos muy proximos al radar

### **Indicadores Visuales**
- **LED en P0.22**: Indicador de proximidad (<20 cm = encendido)

### **Comunicación**
- **UART0**: 115200 baud, interfaz con PC
- **DMA Channel 0**: Transmisión eficiente de buffers sin intervención CPU

### **Microcontrolador**
- **LPC1768** (ARM Cortex-M3)
- **Timers**: Timer0 (servo PWM), Timer1 (TRIGGER), Timer2 (ECHO Capture)
- **ADC**: 12-bit a 200kHz (joystick)
- **DAC**: 10-bit (buzzer sonar)

---

## 🏗️ ARQUITECTURA DEL SISTEMA

```
┌─────────────────────────────────────────────────────────────────┐
│                    SISTEMA DE RADAR ULTRASÓNICO                 │
└─────────────────────────────────────────────────────────────────┘

┌──────────────┐          ┌──────────────┐          ┌──────────────┐
│   Sensor     │          │   Control    │          │   Feedback   │
│  Ultrasónico │          │  Manual/Auto │          │   Sonar      │
└──────┬───────┘          └──────┬───────┘          └──────┬───────┘
       │                         │                         │
       ├─ ECHO (P0.4)            ├─ ADC (P0.23)            └─ DAC (P0.26)
       │  TIMER2 CAPTURE         │  ADC_CH0                   (Buzzer)
       │  Mide pulso (µs)        │  Joystick X-axis
       │  ↓ Calcula distancia    │  ↓ Convierte a ángulo
       │  (high_time / 58)       │  (val * 180 / 4095)
       │
       ├─ TRIG (P0.18)
       │  TIMER1 PWM
       │  Genera pulso 60ms, 10µs
       │
       └──────────────────────────────────┐
                                          │
                                    ┌─────▼──────┐
                                    │  Datos     │
                                    │  Capturados│
                                    │ {ang,dist} │
                                    └─────┬──────┘
                                          │
                        ┌─────────────────┼─────────────────┐
                        │                 │                 │
                   ┌────▼─────┐    ┌─────▼──────┐    ┌─────▼──────┐
                   │ Buffer A  │    │  Buffer B  │    │   DAC      │
                   │ (25x4B)   │    │  (25x4B)   │    │  (Sonar)   │
                   └────┬──────┘    └────┬───────┘    └─────┬──────┘
                        │                │                  │
                   ┌────▼────────────────▼──┐          ┌────▼─────┐
                   │  Servo PWM              │          │  Buzzer  │
                   │  TIMER0 (P0.0)          │          │  Acústico│
                   │  20ms, 500-2500µs       │          └──────────┘
                   └────┬────────────────┬───┘
                        │                │
                   ┌────▼────┐      ┌───▼─────┐
                   │  Servo   │      │   PC    │
                   │  Motor   │      │  UART0  │
                   │  Gira    │      │  DMA    │
                   │  0-180°  │      │ 115200B │
                   └──────────┘      └─────────┘
                        │                  │
                        │                  │
                   ┌────▼──────────────────▼────┐
                   │   Barrido e imagen radar   │
                   │   en pantalla de PC        │
                   └───────────────────────────┘
```

---

## ⚙️ MODOS DE FUNCIONAMIENTO

### **Modo AUTOMÁTICO**
- **Activación**: Enviar comando 'A' por UART desde PC
- **Comportamiento**: 
  - Servo realiza barrido continuo: 0° → 180° → 0°
  - Avanza 2° cada 60 ms (SERVO_PASO_AUTO = 2, SERVO_PERIODOS_POR_PASO = 3)
  - Recorre 180° en ~5.4 segundos
- **Captura de datos**: 
  - Se adquieren muestras continuamente: {ángulo, distancia}
  - Cada 25 muestras se envían al PC mediante DMA
- **Aplicación**: Reconstrucción de imagen radar en tiempo real en la PC

### **Modo MANUAL**
- **Activación**: Enviar comando 'M' por UART desde PC
- **Control**: Joystick analógico (eje X)
  - Rango ADC: 0-4095 cuentas
  - Mapeo: ángulo = (ADC_value × 180) / 4095
  - Centro: ~2048 (zona muerta ±400)
- **Libertad**: Usuario controla completamente la orientación del radar
- **Captura**: Continúa adquiriendo datos mientras controla
- **Aplicación**: Inspección manual de áreas de interés

---

## 🔄 FLUJO DE EJECUCIÓN

### **1. Inicialización (main)**

```
┌─ GPIO Init
│  ├─ P0.22 (LED proximidad)
│  └─ P0.1  (LED saturación)
│
├─ Ultrasonic_Init()
│  ├─ config_timer1_pwm()    → TRIGGER en P0.18 (60ms periodo)
│  └─ config_timer2_capture()→ ECHO en P0.4 (captura flancos)
│
├─ DAC_Config()              → Inicializa salida analógica (buzzer)
│
├─ ADC_Config()              → ADC a 200kHz, Canal 0 (joystick)
│
├─ UART0_Config()            → 115200 baud, DMA habilitado
│
└─ Servo_ConfigTimerPWM()    → PWM en P0.0, 20ms, 50Hz
```

### **2. Loop Principal (Ciclo infinito)**

**Cada iteración (~10-20 ms):**

```
1. distance = Ultrasonic_GetDistance()
   └─ Lee valor calculado en TIMER2_IRQHandler
   
2. angulo_servo = Servo_GetAngulo()
   └─ Obtiene ángulo actual (0-180°)
   
3. DAC_SetDistance(distance)
   ├─ Prende/apaga LED proximidad (si < 20cm)
   └─ Actualiza valor DAC para buzzer sonar
   
4. Si MODO_AUTO:
   │  └─ Servo_SetAnguloAutomatico()
   │     └─ Incrementa ángulo 2° cada 60ms
   │
   └─ Si MODO_MANUAL:
      ├─ adc = ADC_Read()
      ├─ Servo_Tick(adc)
      │  └─ Convierte ADC a ángulo
      └─ Imprime DEBUG (ADC vs ANGULO)
   
5. Guardar muestra en buffer actual
   ├─ radar_bufferA o radar_bufferB
   ├─ Estructura: {angulo, distancia}
   └─ idx++
   
6. Si idx == 25 (buffer completo):
   ├─ Cambia flag_buffer (A↔B)
   ├─ UART0_SendBuffer(buffer, sizeof)
   │  └─ Usa DMA para transmisión sin bloqueo
   ├─ idx = 0
   └─ Ciclo continúa con nuevo buffer
```

### **3. Interrupciones**

#### **TIMER1_IRQHandler** (Cada 60ms)
```
Match 0 @ 10µs:  GPIO P0.18 = 0  (Baja TRIG)
Match 1 @ 60ms:  GPIO P0.18 = 1  (Sube TRIG, inicia nuevo ciclo)
```

#### **TIMER2_IRQHandler** (Rising/Falling edge en ECHO)
```
Rising edge:  start_pulse = Timer2_Count
Falling edge: end_pulse = Timer2_Count
              high_time = end_pulse - start_pulse
              distance_cm = high_time / 58
              
              Maneja overflow (si end_pulse < start_pulse):
              high_time = (0xFFFFFFFF - start_pulse) + end_pulse
```

#### **TIMER0_IRQHandler** (Cada 20ms - Servo PWM)
```
Match 1 @ pulso_us:  GPIO P0.0 = 0  (Baja PWM)
Match 0 @ 20ms:      GPIO P0.0 = 1  (Sube PWM, nuevo ciclo)
```

#### **UART0_IRQHandler** (Comando recibido)
```
Si dato == 'A':  Servo_SetModo(SERVO_MODO_AUTO)
Si dato == 'M':  Servo_SetModo(SERVO_MODO_MANUAL)
```

#### **DMA_IRQHandler** (Transmisión completada)
```
Si TC (Transfer Complete):  dma_busy = 0
Si ERR (Error):            dma_busy = 0
```

---

## 📊 CAPTURA Y TRANSMISIÓN DE DATOS

### **Estructura de Datos**

```c
typedef struct {
    uint16_t angulo;     // 0-180 grados
    uint16_t distancia;  // cm
} radar_sample_t;        // 4 bytes

radar_bufferA[25]  // 100 bytes
radar_bufferB[25]  // 100 bytes
```

### **Double Buffering**

```
Tiempo T0:  ┌─────────────────────────┐
            │  Llenando Buffer A       │
            │  idx: 0 → 24             │
            │  (25 muestras)           │
            └─────────────┬─────────────┘
                          │
Tiempo T1:  ┌─────────────▼─────────────┐
            │  Buffer A lleno!          │
            ├─ flag_buffer = 0          │
            ├─ Envía Buffer A por DMA  │
            └─────────────┬─────────────┘
                          │
Tiempo T1:  ┌─────────────▼─────────────┐
            │  Llenando Buffer B        │
            │  idx: 0 → 24              │
            │  (Mientras A se transmite)│
            └─────────────┬─────────────┘
                          │
Tiempo T2:  ┌─────────────▼─────────────┐
            │  Buffer B lleno!          │
            ├─ flag_buffer = 1          │
            ├─ Envía Buffer B por DMA  │
            └─────────────┬─────────────┘
                          │
Tiempo T2:  ┌─────────────▼─────────────┐
            │  Llenando Buffer A        │
            │  (Mientras B se transmite)│
            └─────────────────────────┘

Ventaja: Nunca pierde datos mientras transmite
```

### **Transmisión UART+DMA**

```
DMA Channel 0:
├─ Source: buffer en RAM
│  └─ Incrementa después de cada byte
├─ Destination: UART0 FIFO
│  └─ NO incrementa (es un puerto)
├─ Transferencia: 100 bytes (25 × 4 bytes)
├─ Baudrate: 115200 bps
│  └─ Tiempo: 100 bytes × 8 bits / 115200 = ~7ms
├─ Interrupciones: TC (Transfer Complete), ERR
└─ Ventaja: CPU libre, sin bloqueos

Secuencia:
1. Buffer A lleno (25 muestras = 100 bytes)
2. DMA_SendBuffer(bufferA, 100)
3. GPDMA_ChannelStart() inicia transmisión
4. Mientras DMA trabaja → CPU llena Buffer B
5. DMA_IRQHandler → dma_busy = 0
6. Siguiente buffer puede enviarse
```

---

## 🎛️ CONFIGURACIÓN DETALLADA

### **Timers y Presiciones**

| Timer | Propósito | Presición | Período | Función |
|-------|-----------|-----------|---------|---------|
| **Timer1** | TRIGGER PWM | 1 µs | 60 ms | Dispara sensor c/60ms |
| **Timer2** | ECHO Capture | 1 µs | N/A | Mide tiempo de pulso |
| **Timer0** | Servo PWM | 1 µs | 20 ms | Genera 50Hz para servo |

### **ADC (Joystick)**

```
Resolución: 12-bit (0-4095)
Clock: 200 kHz
Modo: Manual (ADC_START_NOW)
Canal 0 (P0.23): Joystick X-axis

Mapeo:
Izquierda:    ADC ≈ 0      → Ángulo ≈ 0°
Centro:       ADC ≈ 2048   → Ángulo ≈ 90°
Derecha:      ADC ≈ 4095   → Ángulo ≈ 180°

Zona muerta:  2048 ± 400   → No se mueve
```

### **DAC (Buzzer Sonar)**

```
Resolución: 10-bit (0-1023)
Bias: 700 µA
Mapeo de distancia:

if (distance ≤ 5 cm):    dac_value = 1023  (máximo)
if (distance ≥ 100 cm):  dac_value = 0     (mínimo/silencio)
else:                    dac_value = ((100-dist) × 1023) / 95

Ejemplo:
- 5 cm:   → 1023/1023 = 1.0V  (frecuencia máxima)
- 52 cm:  → 512/1023  ≈ 0.5V (intermedio)
- 100 cm: → 0/1023    = 0V    (sin sonido)
```

### **Servo Motor**

```
Voltaje: 5V DC
Torque: 4.3 kg/cm @ 5V
Velocidad: 60°/60ms @ 5V
Rango: 0° a 180°

Control PWM:
- Período: 20 ms (50 Hz)
- 0.5 ms high → 0°
- 1.5 ms high → 90°
- 2.5 ms high → 180°

Resolución práctica: 1° = 11.1 µs

Conversión (grados a microsegundos):
us = 500 + (grados × 2000) / 180
   = 500 + grados × 11.11
```

### **UART**

```
Baudrate: 115200 bps (11.52 Mbps)
Databits: 8
Parity: None
Stopbits: 1
Flowcontrol: None

Comandos desde PC:
'A' → Modo automático
'M' → Modo manual

Datos hacia PC:
Buffer de 100 bytes cada 25 muestras
Envíos frecuencia: ~1 buffer cada 0.5-1 segundo aprox
```

---

## 📈 PROCESO SENSOR ULTRASÓNICO

```
Ciclo de medición (cada 60 ms):

1. TIMER1 genera pulso TRIGGER (10 µs)
   └─ Envía pulso al sensor

2. Sensor HC-SR04 responde
   └─ Emite pulso ultrasónico (40 kHz)

3. Sonido viaja 
   ├─ Distancia desconocida en ambas direcciones
   └─ Tiempo = 2 × Distancia / Velocidad_Sonido

4. Sensor detecta eco
   └─ Genera pulso ECHO en P0.4

5. TIMER2 captura flancos
   ├─ Rising (inicio): Guarda start_pulse
   └─ Falling (fin):   Guarda end_pulse

6. Cálculo de distancia
   ├─ high_time = end_pulse - start_pulse (en µs)
   ├─ distance = high_time / 58 (datasheet)
   │  └─ Derivado de: 340 m/s × 2 = 58 µs/cm
   └─ Rango práctico: 2-400 cm

Ejemplo real:
Objeto a 50 cm:
├─ Tiempo ida y vuelta: 50cm × 2 / 340m/s = 294 µs
└─ distance_cm = 294 / 58 = 50 cm ✓
```

---

## 🎯 CASOS DE USO

### **Caso 1: Inspección de sala (Modo Automático)**
```
1. PC envía 'A'
2. Servo barre 0° → 180° → 0° continuamente
3. Captura 25 muestras por barrido
4. PC reconstruye imagen radar en tiempo real
5. Usuario ve obstáculos en forma de imagen radial
6. Buzzer genera sonido acústico para objetos cercanos
```

### **Caso 2: Inspección puntual (Modo Manual)**
```
1. PC envía 'M'
2. Usuario mueve joystick
3. Servo apunta a zona de interés
4. Se adquieren datos continuamente
5. PC registra datos en ángulos específicos
6. Permite enfoque en áreas problemáticas
```

### **Caso 3: Detección de proximidad**
```
1. Objeto se acerca a radar
2. Distancia disminuye (ej: 100cm → 50cm → 10cm)
3. DAC aumenta voltaje (0 → 512 → 1023)
4. Buzzer emite sonido 
5. LED P0.22 se enciende (<20cm)
6. Usuario recibe feedback acústico-visual
```

---

## ⚡ OPTIMIZACIONES Y CARACTERÍSTICAS

✅ **Double Buffering**: Captura continua sin pérdida de datos
✅ **DMA para UART**: Transmisión sin bloqueo de CPU
✅ **Zona muerta del joystick**: Previene drift del ADC
✅ **Overflow handling**: Maneja desbordamiento de timer en capture
✅ **Rango útil DAC**: 5-100 cm mapea a rango lineal
✅ **Velocidad variable servo**: 60ms por paso (2°) en automático
✅ **Indicadores LED**: Feedback visual de proximidad
✅ **Debug UART**: Imprime ADC/ángulo en tiempo real
✅ **Precisión ±3cm**: Limitado por sensor HC-SR04
✅ **Frecuencia alta**: 50 Hz servo + 16.7 Hz ciclos de medición

---

## 🔌 PINOUT RESUMIDO

```
LPC1768 Pins utilizados:

P0.0  → SERVO PWM (TIMER0 MAT1)
P0.1  → LED saturación DAC (GPIO OUT)
P0.2  → UART TX (UART0)
P0.3  → UART RX (UART0)
P0.4  → Sensor ECHO (TIMER2 CAP0)
P0.18 → Sensor TRIG (TIMER1 MAT0)
P0.22 → LED proximidad (GPIO OUT)
P0.23 → Joystick (ADC CH0)
P0.26 → Buzzer DAC (DAC OUT)

Timers:
TIMER0 → Servo PWM
TIMER1 → Sensor TRIGGER
TIMER2 → Sensor ECHO Capture

DMA:
CH0 → UART0_TX (Memory to Peripheral)
```

---

## 📋 RESUMEN

Este proyecto implementa un **sistema embebido de radar ultrasónico** que integra:

- **Captura de datos**: Sensor ultrasónico + ADC joystick
- **Control activo**: Servo motor con PWM de precisión
- **Procesamiento**: Cálculos en tiempo real
- **Transmisión**: UART con DMA para eficiencia
- **Retroalimentación**: Sonar acústico (DAC) + LEDs indicadores
- **Flexibilidad**: Dos modos operacionales (manual/automático)

El resultado es un dispositivo IoT completo que combina **adquisición de sensores, control de actuadores, procesamiento digital y comunicación** en una plataforma embebida eficiente.

---

**Grupo 13 - Electrónica Digital III - TP Final**
**Año: 2026**
