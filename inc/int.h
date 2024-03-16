#ifndef INT_H_
#define INT_H_

#include <stdint.h>
#include "stm32g0xx.h"
#include "startup.h"

//-------------------------------------------------------------------------------------------------

void INT_EnableEXTI(uint8_t no, uint8_t priority, void (*handler)(void *), void *object);
void INT_EnableTIM(TIM_TypeDef *tim_typedef, uint8_t priority, void (*handler)(void *), void *object);
void INT_EnableUART(USART_TypeDef *uart_typedef, uint8_t priority, void (*handler)(void *), void *object);
void INT_EnableI2C(I2C_TypeDef *i2c_typedef, uint8_t priority, void (*handler)(void *), void *object);
void INT_EnableADC(uint8_t priority, void (*handler)(void *), void *object);
void INT_EnableDMA(uint8_t dma_no, uint8_t priority, void (*handler)(void *), void *object);

//-------------------------------------------------------------------------------------------------

#endif
