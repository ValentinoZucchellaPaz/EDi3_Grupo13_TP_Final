## Radar Ultrasónico con Modo Manual, Automático y Sonar Acústico

El trabajo consta de un **sensor de distancia acoplado encima de un servo motor**, de esta manera se realiza un barrido 180° y se **mandan las mediciones de distancia a la computadora mediante UART**.
En la computadora se **reconstruye la imagen con los datos y se controla el modo** de funcionamiento: automatico o manual.
El modo manual va a usar un modulo joystick (analogico y convierto con ADC) para manejar el movimiento del servo (barrido)
Ademas de todo lo anterior, las mediciones del sensor se enviarán al DAC, el cual va a transformar eso en analogico y alimentar un buzzer, de manera que cuando se detecte un objeto se tendrá un sonido similar al de un sonar.

### Sensor

- Sensor ultrasónico HC-SR04 montado sobre un servomotor.
- Módulo joystick analógico para control manual.

### Actuador

- Servomotor encargado de orientar el radar.
- Buzzer controlado mediante DAC para generar una respuesta acústica tipo sonar.

### Funcionamiento

- El sistema realiza un barrido angular de 180° detectando obstáculos mediante el sensor ultrasónico.
- Un timer genera la señal PWM utilizada para posicionar el servomotor.
- La medición de distancia se realiza utilizando entradas Capture e interrupciones.
- Los datos obtenidos:
  - ángulo
  - distancia
- se almacenan en memoria y son enviados a una computadora mediante UART utilizando DMA

El sistema posee dos modos de funcionamiento seleccionables desde la PC mediante comandos UART:

#### Modo Automático

- El servomotor realiza continuamente un barrido automático entre los límites angulares definidos.
- El sistema transmite en tiempo real las mediciones adquiridas para reconstruir el radar en una interfaz gráfica desarrollada en la PC.

#### Modo Manual

- El movimiento del radar se controla mediante un joystick analógico.
- El ADC adquiere continuamente la posición horizontal del joystick.
- El DMA almacena las muestras del ADC en memoria para minimizar la intervención de CPU.
- Según la posición detectada:
  - izquierda → el servo gira hacia la izquierda
  - derecha → el servo gira hacia la derecha
- Mientras el usuario controla manualmente el radar, el sistema continúa midiendo distancias y transmitiendo datos hacia la computadora.

#### Sonar Acústico

- La distancia detectada por el sensor ultrasónico se utiliza para controlar una señal generada mediante el DAC.
- La salida analógica del DAC se conecta a un buzzer, generando un efecto acústico similar al de un sonar real.
- A menor distancia detectada mayor intensidad o frecuencia sonora.
- A mayor distancia detectada menor intensidad o frecuencia sonora
- La generación de la señal sonora se realiza utilizando timers y DMA para alimentar continuamente el DAC con una forma de onda almacenada en memoria.

### Periféricos usados

✅ ADC  
✅ DAC  
✅ DMA  
✅ Timers  
✅ PWM (servo)  
✅ Capture (sensor ultrasónico)  
✅ UART Full Duplex  
✅ Interrupciones (ADC, Capture, DMA y UART)
