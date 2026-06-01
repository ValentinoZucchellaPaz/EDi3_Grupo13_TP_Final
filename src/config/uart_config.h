#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include <stdint.h>

// ── Parámetros de comunicación ──────────────────────────
#define UART_BAUD_RATE      115200

// ── Marcadores de trama ─────────────────────────────────
#define TRAMA_STX           0x02    // INICIO
#define TRAMA_ETX           0x03    // FIN
#define TRAMA_SIZE          8       // BYTES X TRAMA

// ── Byte de modo dentro de la trama ─────────────────────
#define TRAMA_MODO_AUTO     0x00
#define TRAMA_MODO_MANUAL   0x01

// ── Comandos que puede recibir desde la PC ───────────────
#define CMD_AUTO            'A'     // cambiar a modo automático
#define CMD_MANUAL          'M'     // cambiar a modo manual
#define CMD_PAUSA           'P'     // pausar sistema
#define CMD_START           'S'     // iniciar sistema

// ── Flags compartidos con main y FSM ────────────────────
extern volatile uint8_t cmd_recibido;
extern volatile uint8_t ultimo_cmd;

// ── API pública ──────────────────────────────────────────
void configUART(void);
void uartEnviarMedicion(uint16_t angulo, uint16_t distancia, uint8_t modo);

#endif