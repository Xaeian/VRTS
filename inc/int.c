#include "int.h"

//-------------------------------------------------------------------------------------------------

void INT_EnableEXTI(uint8_t no, uint8_t priority, void (*handler)(void *), void *object)
{
  switch(no)
  {
    case 0: case 1:
      EXTI0_IRQFnc = handler;
      EXTI0_IRQSrc = object;
      NVIC_SetPriority(EXTI0_1_IRQn, priority);
      NVIC_EnableIRQ(EXTI0_1_IRQn);
      break;
    case 2: case 3:
      EXTI2_IRQFnc = handler;
      EXTI2_IRQSrc = object;
      NVIC_SetPriority(EXTI2_3_IRQn, priority);
      NVIC_EnableIRQ(EXTI2_3_IRQn);
      break;
    case 4: case 5: case 6: case 7: case 8: case 9:
    case 10: case 11: case 12: case 13: case 14: case 15:
      EXTI4_IRQFnc = handler;
      EXTI4_IRQSrc = object;
      NVIC_SetPriority(EXTI4_15_IRQn, priority);
      NVIC_EnableIRQ(EXTI4_15_IRQn);
      break;
  }
}

//-------------------------------------------------------------------------------------------------

void INT_EnableTIM(TIM_TypeDef *tim_typedef, uint8_t priority, void (*handler)(void *), void *object)
{
  switch((uint32_t)tim_typedef) {
    case (uint32_t)TIM1:
      TIM1_IRQFnc = handler;
      TIM1_IRQSrc = object;
      NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, priority);
      NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
      break;
    case (uint32_t)TIM2:
      TIM2_IRQFnc = handler;
      TIM2_IRQSrc = object;
      NVIC_SetPriority(TIM2_IRQn, priority);
      NVIC_EnableIRQ(TIM2_IRQn);
      break;
    case (uint32_t)TIM3:
      TIM3_IRQFnc = handler;
      TIM3_IRQSrc = object;
      NVIC_SetPriority(TIM3_IRQn, priority);
      NVIC_EnableIRQ(TIM3_IRQn);
      break;
    case (uint32_t)TIM6:
      TIM6_IRQFnc = handler;
      TIM6_IRQSrc = object;
      NVIC_SetPriority(TIM6_DAC_LPTIM1_IRQn, priority);
      NVIC_EnableIRQ(TIM6_DAC_LPTIM1_IRQn);
      break;
    case (uint32_t)TIM7:
      TIM7_IRQFnc = handler;
      TIM7_IRQSrc = object;
      NVIC_SetPriority(TIM7_LPTIM2_IRQn, priority);
      NVIC_EnableIRQ(TIM7_LPTIM2_IRQn);
      break;
    case (uint32_t)TIM14:
      TIM14_IRQFnc = handler;
      TIM14_IRQSrc = object;
      NVIC_SetPriority(TIM14_IRQn, priority);
      NVIC_EnableIRQ(TIM14_IRQn);
      break;
    case (uint32_t)TIM15:
      TIM15_IRQFnc = handler;
      TIM15_IRQSrc = object;
      NVIC_SetPriority(TIM15_IRQn, priority);
      NVIC_EnableIRQ(TIM15_IRQn);
      break;
    case (uint32_t)TIM16:
      TIM16_IRQFnc = handler;
      TIM16_IRQSrc = object;
      NVIC_SetPriority(TIM16_IRQn, priority);
      NVIC_EnableIRQ(TIM16_IRQn);
      break;
    case (uint32_t)TIM17:
      TIM17_IRQFnc = handler;
      TIM17_IRQSrc = object;
      NVIC_SetPriority(TIM17_IRQn, priority);
      NVIC_EnableIRQ(TIM17_IRQn);
      break;
  }
}

//-------------------------------------------------------------------------------------------------

void INT_EnableUART(USART_TypeDef *uart_typedef, uint8_t priority, void (*handler)(void *), void *object)
{
  switch((uint32_t)uart_typedef)
  {
    case (uint32_t)USART1:
      USART1_IRQFnc = handler;
      USART1_IRQSrc = object;
      NVIC_SetPriority(USART1_IRQn, priority);
      NVIC_EnableIRQ(USART1_IRQn);
      break;
    case (uint32_t)USART2:
      USART2_IRQFnc = handler;
      USART2_IRQSrc = object;
      NVIC_SetPriority(USART2_IRQn, priority);
      NVIC_EnableIRQ(USART2_IRQn);
      break;
    case (uint32_t)USART3:
      USART3_IRQFnc = handler;
      USART3_IRQSrc = object;
      NVIC_SetPriority(USART3_4_LPUART1_IRQn, priority);
      NVIC_EnableIRQ(USART3_4_LPUART1_IRQn);
      break;
    case (uint32_t)USART4:
      USART4_IRQFnc = handler;
      USART4_IRQSrc = object;
      NVIC_SetPriority(USART3_4_LPUART1_IRQn, priority);
      NVIC_EnableIRQ(USART3_4_LPUART1_IRQn);
      break;
    case (uint32_t)LPUART1:
      LPUART1_IRQFnc = handler;
      LPUART1_IRQSrc = object;
      NVIC_SetPriority(USART3_4_LPUART1_IRQn, priority);
      NVIC_EnableIRQ(USART3_4_LPUART1_IRQn);
      break;
  }
}

void INT_EnableI2C(I2C_TypeDef *i2c_typedef, uint8_t priority, void (*handler)(void *), void *object)
{
  switch((uint32_t)i2c_typedef)
  {
    case (uint32_t)I2C1:
      I2C1_IRQFnc = handler;
      I2C1_IRQSrc = object;
      NVIC_SetPriority(I2C1_IRQn, priority);
      NVIC_EnableIRQ(I2C1_IRQn);
      break;
    case (uint32_t)I2C2:
      I2C2_IRQFnc = handler;
      I2C2_IRQSrc = object;
      NVIC_SetPriority(I2C2_IRQn, priority);
      NVIC_EnableIRQ(I2C2_IRQn);
      break;
  }
}

void INT_EnableADC(uint8_t priority, void (*handler)(void *), void *object)
{
  ADC_IRQFnc = handler;
  ADC_IRQSrc = object;
  NVIC_SetPriority(ADC1_COMP_IRQn, priority);
  NVIC_EnableIRQ(ADC1_COMP_IRQn);
}

//-------------------------------------------------------------------------------------------------

void INT_EnableDMA(uint8_t dma_no, uint8_t priority, void (*handler)(void *), void *object)
{
  switch(dma_no) {
    case 1:
      DMA1_IRQFnc = handler;
      DMA1_IRQSrc = object;
      NVIC_SetPriority(DMA1_Channel1_IRQn, priority);
      NVIC_EnableIRQ(DMA1_Channel1_IRQn);
      break;
    case 2:
      DMA2_IRQFnc = handler;
      DMA2_IRQSrc = object;
      NVIC_SetPriority(DMA1_Channel2_3_IRQn, priority);
      NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
      break;
    case 3:
      DMA3_IRQFnc = handler;
      DMA3_IRQSrc = object;
      NVIC_SetPriority(DMA1_Channel2_3_IRQn, priority);
      NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
      break;
    case 4:
      DMA4_IRQFnc = handler;
      DMA4_IRQSrc = object;
      NVIC_SetPriority(DMA1_Ch4_7_DMAMUX1_OVR_IRQn, priority);
      NVIC_EnableIRQ(DMA1_Ch4_7_DMAMUX1_OVR_IRQn);
      break;
    case 5:
      DMA5_IRQFnc = handler;
      DMA5_IRQSrc = object;
      NVIC_SetPriority(DMA1_Ch4_7_DMAMUX1_OVR_IRQn, priority);
      NVIC_EnableIRQ(DMA1_Ch4_7_DMAMUX1_OVR_IRQn);
      break;
    case 6:
      DMA6_IRQFnc = handler;
      DMA6_IRQSrc = object;
      NVIC_SetPriority(DMA1_Ch4_7_DMAMUX1_OVR_IRQn, priority);
      NVIC_EnableIRQ(DMA1_Ch4_7_DMAMUX1_OVR_IRQn);
      break;
    case 7:
      DMA7_IRQFnc = handler;
      DMA7_IRQSrc = object;
      NVIC_SetPriority(DMA1_Ch4_7_DMAMUX1_OVR_IRQn, priority);
      NVIC_EnableIRQ(DMA1_Ch4_7_DMAMUX1_OVR_IRQn);
      break;
  }
}

//-------------------------------------------------------------------------------------------------
