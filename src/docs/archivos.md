# Guía de Modularización en C — LPC1769
### Electrónica Digital 3 — FCEFyN, UNC

---

## ¿Por qué separar el código en archivos?

Cuando el proyecto crece, tener todo en `main.c` se vuelve un caos. La solución es separar cada periférico en su propio par de archivos `.c` y `.h`. Así el código es más fácil de leer, debuggear y reutilizar.

```
❌ Todo junto (mal)          ✅ Modularizado (bien)
─────────────────────        ──────────────────────────────
main.c (500 líneas)          main.c          (30 líneas)
                             config/dac_config.c
                             config/dac_config.h
                             config/timer_pwm.c
                             config/timer_pwm.h
                             app/fsm.c
                             app/fsm.h
```

---

## La regla fundamental

```
archivo.h  →  QUÉ existe  (la "vitrina" — visible para todos)
archivo.c  →  CÓMO funciona (la "cocina" — implementación)
main.c     →  usa todo incluyendo solo los .h
```

**Nunca incluís un `.c`. Siempre incluís el `.h`.**

---

## Anatomía de un archivo .h

El `.h` es la "interfaz pública" del módulo. Solo declara, nunca implementa.

```c
#ifndef DAC_CONFIG_H       // ← GUARDIA: evita que se incluya dos veces
#define DAC_CONFIG_H       //   el nombre es el archivo en MAYÚSCULAS con _

// 1. Includes necesarios para los tipos que usás
#include "lpc17xx_dac.h"
#include <stdint.h>

// 2. Constantes del módulo
#define DISTANCIA_MAX_CM   400
#define DAC_BITS           1023

// 3. Structs o enums que otros módulos necesitan ver
typedef enum {
    TONO_GRAVE,
    TONO_MEDIO,
    TONO_AGUDO
} TipoTono_t;

// 4. Variables globales que otros módulos pueden leer
//    (solo se DECLARAN acá con extern, se DEFINEN en el .c)
extern volatile uint16_t dac_valor_actual;

// 5. Prototipos de funciones (sin cuerpo, terminan en ;)
void configDAC(void);
void dacSetTono(uint16_t distancia_cm);

#endif                     // ← cierra el #ifndef
```

---

## Anatomía de un archivo .c

El `.c` es la implementación real. Acá van los cuerpos de las funciones.

```c
// 1. Siempre incluís tu propio header primero
#include "dac_config.h"

// 2. Variables globales del módulo
//    Si pusiste extern en el .h, acá va la definición real
volatile uint16_t dac_valor_actual = 0;

// 3. Implementación completa de cada función declarada en el .h
void configDAC(void){
    DAC_Init();
    DAC_SetBias(DAC_350uA);   // menor consumo
}

void dacSetTono(uint16_t distancia_cm){
    // objeto cerca → tono agudo (DAC alto)
    // objeto lejos → tono grave (DAC bajo)
    uint16_t valor = DAC_BITS - (distancia_cm * DAC_BITS / DISTANCIA_MAX_CM);
    DAC_UpdateValue(valor);
    dac_valor_actual = valor;
}
```

---

## Cómo se conecta todo en main.c

```c
// main.c solo incluye los .h, nunca los .c
#include "config/dac_config.h"
#include "config/timer_pwm.h"
#include "config/uart_config.h"
#include "app/fsm.h"

int main(void){
    // Llama a las funciones definidas en los otros módulos
    configDAC();
    configTimerPWM();
    configUART();
    fsm_init();

    while(1){
        fsm_update();
    }
    return 0;
}
```

---

## Ejemplo completo — módulo FSM

Este ejemplo muestra cómo compartir una variable de estado entre módulos.

**app/fsm.h**
```c
#ifndef FSM_H
#define FSM_H

// Enum visible para todo el proyecto
typedef enum {
    ESCANEO,
    TRACKING,
    PAUSADO
} Estado_t;

// extern: "esta variable existe, pero está definida en fsm.c"
// Cualquier archivo que incluya fsm.h puede leer estadoActual
extern volatile Estado_t estadoActual;

// Prototipos
void fsm_init(void);
void fsm_update(void);

#endif
```

**app/fsm.c**
```c
#include "fsm.h"

// Acá está la definición real de la variable
volatile Estado_t estadoActual = ESCANEO;

void fsm_init(void){
    estadoActual = ESCANEO;
}

void fsm_update(void){
    switch(estadoActual){
        case ESCANEO:
            // lógica de escaneo
            break;
        case TRACKING:
            // lógica de tracking
            break;
        case PAUSADO:
            // sistema en espera
            break;
        default:
            break;
    }
}
```

**Desde cualquier otro módulo:**
```c
#include "app/fsm.h"

void EINT0_IRQHandler(void){
    // Puedo leer y escribir estadoActual porque incluí fsm.h
    estadoActual = TRACKING;
}
```

---

## El error más común — incluir el .c

```c
// ❌ MAL — nunca hagas esto
#include "dac_config.c"

// ✅ BIEN — siempre el .h
#include "dac_config.h"
```

Si incluís el `.c`, el compilador define las funciones dos veces y da error de "multiple definition".

---

## La guardia del .h — ¿por qué es obligatoria?

```c
#ifndef DAC_CONFIG_H   // "si DAC_CONFIG_H NO está definido..."
#define DAC_CONFIG_H   // "...definilo ahora"

// contenido del header

#endif                 // fin de la condición
```

Sin esta guardia, si dos archivos distintos incluyen el mismo `.h`, el compilador ve las declaraciones dos veces y falla. La guardia garantiza que el contenido se procesa **una sola vez** sin importar cuántas veces se incluya el archivo.

El nombre de la guardia es por convención el nombre del archivo en mayúsculas con guiones bajos:
```
dac_config.h   →   DAC_CONFIG_H
timer_pwm.h    →   TIMER_PWM_H
fsm.h          →   FSM_H
```

---

## VSCode vs LPCXpresso — ¿cómo conviven?

**VSCode no compila ni flashea.** Es solo un editor de texto (muy bueno). LPCXpresso tiene el compilador ARM y el flasher. La solución es usarlos juntos:

```
VSCode                    LPCXpresso
──────────────────────    ──────────────────────────
Escribís el código   →    Compila con arm-none-eabi-gcc
Navegás los archivos →    Flashea la placa
Git, terminal, etc   →    Debug con breakpoints
```

### Flujo de trabajo recomendado

```
① Crear el proyecto UNA VEZ en LPCXpresso
      File → New → LPC17xx C Project
      Elegir LPC1769, agregar drivers de la cátedra

② La carpeta del proyecto queda en tu PC
      Ejemplo: C:/Users/vos/lpc1769-radar/

③ Abrir ESA MISMA carpeta en VSCode
      File → Open Folder → seleccionás la carpeta del proyecto

④ Editás en VSCode con todas sus ventajas
      (autocompletado, Git integrado, terminal, etc.)

⑤ Para compilar y flashear: volvés a LPCXpresso
      LPCXpresso detecta los cambios automáticamente
      Build → Debug → carga en la placa
```

Los archivos son los mismos en disco. Solo cambia qué programa los abre.

---

## Agregar archivos nuevos a LPCXpresso

Cuando creás un `.c` nuevo en VSCode, LPCXpresso no lo ve automáticamente en el build. Hay que avisarle:

```
① Click derecho en el proyecto → Refresh
   (LPCXpresso escanea la carpeta y detecta archivos nuevos)

② Si el archivo no entra en el build:
   Project → Properties
   → C/C++ Build → Settings
   → Verificar que src/config y src/app estén incluidas

③ Para que los #include funcionen correctamente:
   Project → Properties
   → C/C++ General → Paths and Symbols → Includes → Add
   → Agregar: ${workspace_loc:/tu-proyecto/src/config}
   → Agregar: ${workspace_loc:/tu-proyecto/src/app}
```

---

## Resumen visual del flujo completo

```
         VSCode (escribís)
         ├── src/main.c
         ├── src/config/dac_config.h  ←─────────────┐
         ├── src/config/dac_config.c                 │
         ├── src/app/fsm.h                           │
         └── src/app/fsm.c                           │
                  │                                  │
                  │ main.c hace:                     │
                  │ #include "config/dac_config.h" ──┘
                  │
                  ▼
         LPCXpresso (compilás)
         arm-none-eabi-gcc junta todos los .c
                  │
                  ▼
         proyecto.axf  (binario ARM)
                  │
                  ▼
         Flash de la LPC1769 ✓
```

---

## Checklist antes de agregar un módulo nuevo

```
□ Creé el archivo modulo.h con la guardia #ifndef
□ Declaré los prototipos de todas las funciones públicas
□ Usé extern para las variables compartidas
□ Creé el archivo modulo.c
□ Incluí el propio header (#include "modulo.h") primero
□ Implementé todas las funciones declaradas en el .h
□ En main.c incluí el .h (no el .c)
□ En LPCXpresso hice Refresh y verifiqué los include paths
□ Compilé y no hay errores de "undefined reference" ni
  "multiple definition"
```

---

*Electrónica Digital 3 — FCEFyN, UNC — 2025*