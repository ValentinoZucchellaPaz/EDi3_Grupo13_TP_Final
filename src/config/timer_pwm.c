#include "timer_pwm.h"

// ── Variables públicas ───────────────────────────────────
volatile uint8_t     servo_angulo = 0;
volatile ServoModo_t servo_modo   = SERVO_MODO_AUTO;

// ── Variable privada — dirección del barrido automático ─
static int8_t direccion = 1;   // +1 subiendo, -1 bajando

// ── Función interna: convierte grados a microsegundos ───
static uint32_t anguloAPulso(uint8_t angulo){
    // mapea 0°-180° → 1000µs-2000µs linealmente
    return SERVO_MIN_US + ((uint32_t)angulo * 1000 / 180);
}

// ────────────────────────────────────────────────────────
void configTimerPWM(void){
    //  P1.29 → PINSEL3 bits[27:26] = 00
    PINSEL_CFG_T pinCfg = {
        .port      = PORT_1,
        .pin       = PIN_29,
        .func      = PINSEL_FUNC_00,   
        .mode      = PINSEL_PULLUP,
        .openDrain = DISABLE
    };
    GPIO_ClearPins(PORT_1,(1<<29));
    GPIO_SetDir(PORT_1,(1<<29),GPIO_OUTPUT);
    PINSEL_ConfigPin(&pinCfg);

    // 1 tick = 1µs
    TIM_TIMERCFG_T timCfg = {
        .prescaleOpt   = TIM_US,
        .prescaleValue = 1
    };

    // MR0: período 20ms — resetea el contador
    TIM_MATCHCFG_T mr0 = {
        .channel    = TIM_MATCH_0,
        .matchValue = SERVO_PERIOD_US,
        .resetEn    = ENABLE,
        .stopEn     = DISABLE,
        .intEn      = ENABLE,
        .extOpt     = TIM_NOTHING
    };

    // MR1: duty cycle 
    TIM_MATCHCFG_T mr1 = {
        .channel    = TIM_MATCH_1,
        .matchValue = anguloAPulso(0),  // arranca en 0°
        .resetEn    = DISABLE,
        .stopEn     = DISABLE,
        .intEn      = ENABLE,
        .extOpt     = TIM_NOTHING        
    };

    TIM_InitTimer(LPC_TIM0, &timCfg);
    TIM_ConfigMatch(LPC_TIM0, &mr0);
    TIM_ConfigMatch(LPC_TIM0, &mr1);
    TIM_Enable(LPC_TIM0);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

// ────────────────────────────────────────────────────────
void servoSetAngulo(uint8_t angulo){
    if(angulo > SERVO_ANGULO_MAX) angulo = SERVO_ANGULO_MAX;
    if(angulo < SERVO_ANGULO_MIN) angulo = SERVO_ANGULO_MIN;
    servo_angulo = angulo;
    TIM_UpdateMatchValue(LPC_TIM0, TIM_MATCH_1, anguloAPulso(angulo));
}

// ────────────────────────────────────────────────────────
void servoSetModo(ServoModo_t modo){
    servo_modo = modo;
    // al cambiar a automático reseteamos la dirección
    if(modo == SERVO_MODO_AUTO){
        direccion = 1;
    }
}

// ────────────────────────────────────────────────────────
// Llamar desde SysTick cada 50ms
void servoTick(uint16_t joystick_raw){

    if(servo_modo == SERVO_MODO_AUTO){

        // Usamos int16_t para poder detectar negativos antes de castear
        int16_t nuevo = (int16_t)servo_angulo + (int16_t)(direccion * SERVO_PASO_AUTO);

        // Verificamos límites ANTES de castear a uint8_t
        if(nuevo >= (int16_t)SERVO_ANGULO_MAX){
            nuevo     = (int16_t)SERVO_ANGULO_MAX;
            direccion = -1;    // llegó a 180° → empieza a bajar
        }
        else if(nuevo <= (int16_t)SERVO_ANGULO_MIN){
            nuevo     = (int16_t)SERVO_ANGULO_MIN;
            direccion = 1;     // llegó a 0° → empieza a subir
        }

        // Recién acá casteamos, sabemos que nuevo está entre 0 y 180
        servoSetAngulo((uint8_t)nuevo);
        // servoSetAngulo hace: servo_angulo = angulo ← acá se actualiza

    } else {

        int16_t desviacion = (int16_t)joystick_raw - (int16_t)JOYSTICK_CENTRO;

        if(desviacion > (int16_t)JOYSTICK_ZONA_MUERTA){
            int16_t paso = desviacion / 400 + 1;
            int16_t nuevo = (int16_t)servo_angulo + paso;
            if(nuevo > (int16_t)SERVO_ANGULO_MAX) nuevo = SERVO_ANGULO_MAX;
            servoSetAngulo((uint8_t)nuevo);

        } else if(desviacion < -(int16_t)JOYSTICK_ZONA_MUERTA){
            int16_t paso = (-desviacion) / 400 + 1;
            int16_t nuevo = (int16_t)servo_angulo - paso;
            if(nuevo < (int16_t)SERVO_ANGULO_MIN) nuevo = SERVO_ANGULO_MIN;
            servoSetAngulo((uint8_t)nuevo);
        }
        // zona muerta → no se llama servoSetAngulo → servo quieto
    }
}
void TIMER0_IRQHandler(void)
{
      /*
        MR0 - Comienza nuevo período - HIGH
    */
    if(TIM_GetIntStatus(LPC_TIM0,TIM_MR0_INT) == SET){
        TIM_ClearIntPending(LPC_TIM0,TIM_MR0_INT);
        GPIO_SetPins(PORT_1,(1 << 29));
    }
    
    /*
        MR1 - Fin del pulso - LOW
    */
    if(TIM_GetIntStatus(LPC_TIM0,TIM_MR1_INT) == SET){
        TIM_ClearIntPending(LPC_TIM0,TIM_MR1_INT);
        GPIO_ClearPins(PORT_1,(1 << 29));
    }
  
}