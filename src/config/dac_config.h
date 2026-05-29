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