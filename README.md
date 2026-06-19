# Radar Ultrasónico 180° con Control Dual

> **Asignatura:** Electrónica Digital III - Universidad Nacional de Córdoba
> **Integrantes:**
>
> - Garay, Alexis Tomas
> - Guzman, Pedro
> - Zucchella Paz, Valentino
>
> **Profesor:** Blasco, Marcos Javier

---

## 🚀 1. Descripción General del Proyecto

El sistema implementa un radar ultrasónico de barrido 180° sobre un microcontrolador LPC1769 (ARM Cortex-M3). Un sensor HC-SR04 montado sobre un servomotor SG90 realiza mediciones de distancia mientras barre el espacio frente a él. Los datos capturados se transmiten por UART a una PC, donde un script Python los grafica en tiempo real en formato polar, recreando visualmente la imagen de un radar.

El sistema resuelve la necesidad de visualizar la distribución espacial de objetos en un entorno de forma no invasiva y a bajo costo. Está orientado a aplicaciones educativas y de prototipado, sirviendo como base para sistemas de detección de presencia o asistencia a la navegación en robótica.

---

<p align="center">
  <img src="./docs/video_cantina.gif" />
</p>

<p align="center">
  <img src="./docs/deteccion_de_personas.gif" />
</p>

<p align="center">
  <a href="#evidencia-fotográfica-y-gráficos">Ver mas imagenes</a>
</p>
   
---

### 🎯 Alcances del Proyecto

- **El sistema SÍ es capaz de:** Realizar barridos automáticos continuos de 0° a 180° y viceversa, permitir control manual de la posición angular mediante potenciómetro, medir distancias con el HC-SR04 (rango 2–450 cm, ±3 cm), transmitir paquetes de 90 muestras `{ángulo, distancia}` por UART con DMA, detectar y descartar paquetes con error de sincronización en el lado Python, y proveer retroalimentación acústica (buzzer) y visual (LEDs) en tiempo real.

- **El sistema NO incluye (Fuera de alcance):** Almacenamiento local de datos (data logging), conectividad inalámbrica, detección de múltiples objetos simultáneos, compensación de temperatura en la medición ultrasónica ni uso de sistema operativo en tiempo real (RTOS), tampoco incluye un filtro a los datos obtenidos por el sensor.

### ⏩ Posibles Etapas Siguientes

- Migrar el circuito de protoboard a un PCB, mejorando la integridad de señal y reduciendo interferencias en los pulsos del HC-SR04 y el PWM del servo.
- Agregar conectividad inalámbrica (ESP8266/ESP32 por UART) para visualización remota desde navegador web.
- Incorporar un segundo sensor para barrido en 3D o detección de altura.
- Implementar modos de bajo consumo (sleep del Cortex-M3) para uso con batería en aplicaciones portátiles.
- Reemplazar el script Python por una aplicación embebida con pantalla TFT conectada directamente al LPC.

---

## 📐 2. Arquitectura del Sistema: Hardware y Software

### 🔌 Hardware & Interconexión

- **Diagrama de Bloques:**
  ![diagrama en bloques](./docs/diagrama.png)

- **Esquemático del Circuito:**
  ![diagrama circuital](./docs/diagrama%20Circuital.jpeg)

- **Esquemático de Flujo:**
  ![diagrama de flujo](./docs/Diagrama%20de%20Flujo.png)

- **Descripción del Circuito y Consideraciones de Diseño:**

  El sistema tiene dos dominios de alimentación: la LPC1769 opera a 3.3V provista por USB, y los periféricos (HC-SR04 y SG90) operan a 5V desde una fuente externa dedicada. Las señales de control del servo y del sensor (TRIG/ECHO) son manejadas directamente por los pines del LPC; debido a que el pin ECHO del sensor devuelve 5V se implemento un divisor resistivo para que entren 3.3V a la placa.

  Una consideración crítica de diseño fue la incorporación de un capacitor electrolítico de **1000 µF** entre VCC y GND de la alimentación del servo. El SG90 demanda picos de corriente elevados durante cambios bruscos de posición (torque máximo), lo que generaba caídas de tensión que interferían con el resto del circuito. El capacitor actúa como reservorio local de energía, absorbiendo estos transitorios y estabilizando la alimentación.

  El DAC del LPC (P0.26) se conecta al osciloscopio para visualizar la distancia medida como señal analógica, útil para depuración.

**Pinout:**

| Pin LPC | Función                   | Periférico    |
| ------- | ------------------------- | ------------- |
| P0.0    | Servo PWM (TIMER0)        | SG90          |
| P0.1    | Buzzer (DAC)              | Buzzer pasivo |
| P0.2    | UART TX                   | USB-Serial PC |
| P0.3    | UART RX                   | USB-Serial PC |
| P0.4    | ECHO capture (TIMER2 CAP) | HC-SR04       |
| P0.18   | TRIG (TIMER1)             | HC-SR04       |
| P0.22   | LED proximidad (<20cm)    | LED rojo      |
| P0.23   | Potenciómetro (ADC CH0)   | 10kΩ pot      |
| P0.26   | DAC (osciloscopio)        | Osciloscopio  |
| P3.25   | LED modo manual           | LED amarillo  |

### 💻 Arquitectura de Software (Firmware)

La arquitectura es **bare-metal con lazo principal cooperativo e interrupciones**. No se utiliza RTOS; la concurrencia se logra enteramente mediante el NVIC del Cortex-M3 con 5 fuentes de interrupción independientes.

**Lazo principal (cada ~10–20 ms):**

```
1. Lee distancia medida por TIMER2
2. Lee ángulo actual del servo
3. Actualiza DAC (buzzer + osciloscopio)
4. Modo AUTO: incrementa ángulo 2°  |  Modo MANUAL: lee ADC → calcula ángulo
5. Guarda muestra {ángulo, distancia} en buffer activo
6. Si buffer lleno (25 muestras) → dispara DMA TX por UART
```

**Interrupciones y responsabilidades:**

| Interrupción | Período    | Función                                           |
| ------------ | ---------- | ------------------------------------------------- |
| TIMER1_IRQ   | 60 ms      | Genera pulso TRIG de 10 µs para HC-SR04           |
| TIMER2_IRQ   | Por flanco | Captura tiempo de ECHO → calcula distancia (÷58)  |
| TIMER0_IRQ   | 20 ms      | Genera PWM del servo (500–2500 µs según ángulo)   |
| UART0_IRQ    | Por byte   | Recibe comandos: `'A'`→AUTO, `'M'`→MANUAL         |
| DMA_IRQ      | Por TX     | Libera flag `dma_busy` al completar transferencia |

**Double Buffering UART:** Mientras el Buffer A (360 bytes) se transmite por DMA, el Buffer B se llena con nuevas muestras. Al completarse la TX, se intercambian roles. Esto garantiza cero pérdida de datos y no bloquea el lazo principal durante la transmisión (~7 ms a 115200 baud).

**Protocolo de trama UART:**

```
[0xAA][0x55][ángulo_L][ángulo_H][dist_L][dist_H] × 90 muestras
Header de sincronización 2 bytes + 90 structs de 4 bytes = 362 bytes/paquete
```

**Script Python (interfaz gráfica):**
El script recibe los paquetes UART, valida la cantidad de bytes recibidos contra los esperados (362 bytes), y descarta el paquete marcándolo como error si no coinciden. Los paquetes válidos se grafican en coordenadas polares actualizando el barrido del radar en tiempo real. El script también envía los comandos `'A'` o `'M'` para controlar el modo de operación del firmware.

---

## ⚡ 3. Especificaciones Eléctricas, Alimentación y Entorno

### 🔌 Parámetros de Alimentación y Consumo

- **Tensión de operación:**
  - LPC1769 y lógica: 3.3V (USB)
  - Periféricos (HC-SR04, SG90, buzzer, LEDs): 5V (fuente externa)

- **Método de alimentación:**
  - LPC1769: USB desde PC (programación y alimentación simultánea)
  - Periféricos: Fuente externa 5V con capacitor de desacople de 1000 µF en la línea del servo

### 📌 Especificaciones Electrónica Digital III (Cortex-M / ARM)

- **IDE y SDK:** MCUXpresso IDE
- **Microcontrolador Principal:** NXP LPC1769 (ARM Cortex-M3, 100 MHz)
- **Bibliotecas de Terceros:** Drivers del Profesor David Trujillo (abstracciones de los drivers de CMSIS) y funciones y abstracciones propias.
- **Periféricos Avanzados Utilizados:** NVIC (5 fuentes de interrupción), DMA (canal UART TX), DAC 10-bit, ADC 12-bit a 200 kHz, 3× Timers (T0, T1, T2), UART0 a 115200 baud.
- **Estrategia de Concurrencia:** Toda la temporización crítica (PWM servo, trigger ultrasónico, captura de eco) se delega a hardware (timers + captura por flanco), liberando al CPU. La transmisión de datos se realiza enteramente por DMA sin intervención del CPU, disparada desde el lazo principal cuando el buffer está lleno. Cuando se reciben datos se hace con interrupcion de UART (cambio de modo).

---

## 🔄 4. Proceso de Integración y Desarrollo

- **Etapa 1 – Validación de módulos individuales:** Se desarrollaron y validaron por separado el driver del servomotor (PWM por TIMER0) y el driver del sensor HC-SR04 (trigger por TIMER1, captura de eco por TIMER2). Se verificó el rango angular completo del servo (0°–180°) y las distancias medidas contra valores conocidos.

- **Etapa 2 – Integración firmware básico:** Se integró el control del servo con la lectura del sensor en el lazo principal. Se incorporó el ADC para el modo manual y el DAC para retroalimentación visual en osciloscopio. Se validó el barrido automático completo con mediciones en tiempo real.

- **Etapa 3 – Comunicación UART simple:** Se implementó la transmisión UART básica (sin DMA) enviando los datos por consola serie. Esto permitió detectar un **problema de desincronización**: los datos llegaban corridos respecto al frame esperado. Se resolvió incorporando el header de sincronización `0xAA 0x55` al inicio de cada paquete y validando la longitud en el receptor Python.

- **Etapa 4 – Sistema completo con DMA e interfaz Python:** Se reemplazó la UART bloqueante por DMA con double buffering. Se desarrolló la interfaz gráfica Python con gráfico polar en tiempo real, validación de paquetes por conteo de bytes, control de modo (AUTO/MANUAL) desde la PC y descarte de paquetes erróneos. Se realizaron pruebas de estrés de la comunicación verificando estabilidad en barridos continuos prolongados.

---

## 📊 5. Ensayos, Pruebas y Resultados

### Pruebas Funcionales Realizadas

- **Precisión del HC-SR04:** Se midieron objetos a distancias conocidas (10, 30, 50, 100 cm) comparando con regla. Error medido dentro de ±3 cm en todo el rango.

- **Resolución angular del servo:** Se verificó el posicionamiento en 0°, 45°, 90°, 135° y 180° midiendo el ángulo físico. El error fue inferior a ±5°.

- **Validación del protocolo UART con DMA:** Se usó un script de prueba en consola que imprimía los paquetes crudos recibidos, permitiendo detectar y corregir el problema de desincronización. Tras incorporar el header `0xAA 0x55`, la tasa de paquetes erróneos cayó a 0 en condiciones normales.

- **Estabilidad del capacitor de desacople:** Se observó en osciloscopio la línea de 5V del servo con y sin el capacitor de 1000 µF, confirmando la reducción de los picos de caída de tensión durante arranque del motor.

- **Validación del modo manual:** Se barrió el potenciómetro de extremo a extremo verificando que el ángulo reportado seguía linealmente el valor ADC (0–4095 → 0°–180°).

### Evidencia Fotográfica y Gráficos

- **Captura de osciloscopio – señal DAC distancia / línea 5V servo:**
<p align="center">
  <img src="./docs/dac_output.gif" />
</p>

- **Screenshot interfaz Python – radar en funcionamiento:**
<p align="center">
  <img src="./docs/interfaz_python.jpeg" />
</p>

- **Foto del prototipo real:**
<p align="center">
  <img src="./docs/prototipo_real.jpeg" />
</p>

---

## 📂 6. Estructura del Repositorio

```text
├── TP_FINAL_EDIII/
│   └── src/
│       ├── Config/              # Archivos .c y .h de los módulos del proyecto
│       └── TP_FINAL_EDIII.c     # Programa principal (main)
├── docs/
│   ├── diagrama.png             # Diagrama circuital
│   └── datasheets/              # HC-SR04, SG90, LPC1769
├── uart_monitor/
│   ├── radar_debug.py           # Debug de UART por consola
│   └── radar_app.py             # Interfaz gráfica polar + control de modo
└── README.md
```
