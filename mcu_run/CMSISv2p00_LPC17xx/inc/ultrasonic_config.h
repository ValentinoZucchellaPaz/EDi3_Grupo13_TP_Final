#ifndef ULTRASONIC_CONFIG_H
#define ULTRASONIC_CONFIG_H

static void config_timer2_pwm(void);
static void config_timer1_capture(void);
void Ultrasonic_Init(void);
int Ultrasonic_GetDistance(void);

#endif