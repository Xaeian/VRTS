#include <stdint.h>
#include "system_stm32g0xx.h"

#define __weak __attribute__((weak))

// Constants for linker
extern uint32_t _sidata; // Start address of init data
extern uint32_t _sdata; // Start address of data
extern uint32_t _edata; // End address of data
extern uint32_t _sbss; // Start address of bss
extern uint32_t _ebss; // End address of bss
extern uint32_t _estack; // Top od stack

extern void __libc_init_array(); // Static constructor initializator form libc
extern int main(); // Main program point

void Reset_Handler(void) {
  uint32_t *dataInit = &_sidata;
  uint32_t *data = &_sdata;
  while(data < &_edata) {
    *data++ = *dataInit++;
  }
  uint32_t *bss = &_sbss;
  while(bss < &_ebss) {
    *bss++ = 0;
  }
  SystemInit();
  __libc_init_array();
  main();
  while(1);
}

void Default_Handler(void) {
  while(1);
}

void Void_Handler(void *object) {
}

void (*ADC_IRQFnc)(void *) = Void_Handler; void *ADC_IRQSrc;
void (*EXTI0_IRQFnc)(void *) = Void_Handler; void *EXTI0_IRQSrc;
void (*EXTI1_IRQFnc)(void *) = Void_Handler; void *EXTI1_IRQSrc;
void (*EXTI2_IRQFnc)(void *) = Void_Handler; void *EXTI2_IRQSrc;
void (*EXTI3_IRQFnc)(void *) = Void_Handler; void *EXTI3_IRQSrc;
void (*EXTI4_IRQFnc)(void *) = Void_Handler; void *EXTI4_IRQSrc;
void (*EXTI5_IRQFnc)(void *) = Void_Handler; void *EXTI5_IRQSrc;
void (*EXTI6_IRQFnc)(void *) = Void_Handler; void *EXTI6_IRQSrc;
void (*EXTI7_IRQFnc)(void *) = Void_Handler; void *EXTI7_IRQSrc;
void (*EXTI8_IRQFnc)(void *) = Void_Handler; void *EXTI8_IRQSrc;
void (*EXTI9_IRQFnc)(void *) = Void_Handler; void *EXTI9_IRQSrc;
void (*EXTI10_IRQFnc)(void *) = Void_Handler; void *EXTI10_IRQSrc;
void (*EXTI11_IRQFnc)(void *) = Void_Handler; void *EXTI11_IRQSrc;
void (*EXTI12_IRQFnc)(void *) = Void_Handler; void *EXTI12_IRQSrc;
void (*EXTI13_IRQFnc)(void *) = Void_Handler; void *EXTI13_IRQSrc;
void (*EXTI14_IRQFnc)(void *) = Void_Handler; void *EXTI14_IRQSrc;
void (*EXTI15_IRQFnc)(void *) = Void_Handler; void *EXTI15_IRQSrc;
void (*DMA1_IRQFnc)(void *) = Void_Handler; void *DMA1_IRQSrc;
void (*DMA2_IRQFnc)(void *) = Void_Handler; void *DMA2_IRQSrc;
void (*DMA3_IRQFnc)(void *) = Void_Handler; void *DMA3_IRQSrc;
void (*DMA4_IRQFnc)(void *) = Void_Handler; void *DMA4_IRQSrc;
void (*DMA5_IRQFnc)(void *) = Void_Handler; void *DMA5_IRQSrc;
void (*DMA6_IRQFnc)(void *) = Void_Handler; void *DMA6_IRQSrc;
void (*DMA7_IRQFnc)(void *) = Void_Handler; void *DMA7_IRQSrc;
void (*TIM1_IRQFnc)(void *) = Void_Handler; void *TIM1_IRQSrc;
void (*TIM2_IRQFnc)(void *) = Void_Handler; void *TIM2_IRQSrc;
void (*TIM3_IRQFnc)(void *) = Void_Handler; void *TIM3_IRQSrc;
void (*TIM6_IRQFnc)(void *) = Void_Handler; void *TIM6_IRQSrc;
void (*TIM7_IRQFnc)(void *) = Void_Handler; void *TIM7_IRQSrc;
void (*TIM14_IRQFnc)(void *) = Void_Handler; void *TIM14_IRQSrc;
void (*TIM15_IRQFnc)(void *) = Void_Handler; void *TIM15_IRQSrc;
void (*TIM16_IRQFnc)(void *) = Void_Handler; void *TIM16_IRQSrc;
void (*TIM17_IRQFnc)(void *) = Void_Handler; void *TIM17_IRQSrc;
void (*I2C1_IRQFnc)(void *) = Void_Handler; void *I2C1_IRQSrc;
void (*I2C2_IRQFnc)(void *) = Void_Handler; void *I2C2_IRQSrc;
void (*USART1_IRQFnc)(void *) = Void_Handler; void *USART1_IRQSrc;
void (*USART2_IRQFnc)(void *) = Void_Handler; void *USART2_IRQSrc;
void (*USART3_IRQFnc)(void *) = Void_Handler; void *USART3_IRQSrc;
void (*USART4_IRQFnc)(void *) = Void_Handler; void *USART4_IRQSrc;
void (*LPUART1_IRQFnc)(void *) = Void_Handler; void *LPUART1_IRQSrc;

__weak void HardFault_Handler(void) {
  while(1);
}

__weak void NMI_Handler(void) { Default_Handler(); }
__weak void	SVC_Handler(void) { Default_Handler(); }
__weak void	PendSV_Handler(void) { Default_Handler(); }
__weak void	SysTick_Handler(void) { Default_Handler(); }
__weak void	WWDG_IRQHandler(void) { Default_Handler(); }
__weak void	PVD_IRQHandler(void) { Default_Handler(); }
__weak void	RTC_STAMP_IRQHandler(void) { Default_Handler(); }
__weak void	FLASH_IRQHandler(void) { Default_Handler(); }
__weak void	RCC_IRQHandler(void) { Default_Handler(); }
__weak void UCPD1_UCPD2_IRQHandler(void) { Default_Handler(); }
__weak void CEC_IRQHandler(void) { Default_Handler(); }
__weak void ADC_COMP_IRQHandler(void) { ADC_IRQFnc(ADC_IRQSrc); }
__weak void	EXTI0_1_IRQHandler(void) { EXTI0_IRQFnc(EXTI0_IRQSrc); EXTI1_IRQFnc(EXTI1_IRQSrc); }
__weak void	EXTI2_3_IRQHandler(void) { EXTI2_IRQFnc(EXTI2_IRQSrc); EXTI3_IRQFnc(EXTI3_IRQSrc); }
__weak void	EXTI4_15_IRQHandler(void) { EXTI4_IRQFnc(EXTI4_IRQSrc); EXTI5_IRQFnc(EXTI5_IRQSrc); EXTI6_IRQFnc(EXTI6_IRQSrc); EXTI7_IRQFnc(EXTI7_IRQSrc); EXTI8_IRQFnc(EXTI8_IRQSrc); EXTI9_IRQFnc(EXTI9_IRQSrc); EXTI10_IRQFnc(EXTI10_IRQSrc); EXTI11_IRQFnc(EXTI11_IRQSrc); EXTI12_IRQFnc(EXTI12_IRQSrc); EXTI13_IRQFnc(EXTI13_IRQSrc); EXTI14_IRQFnc(EXTI14_IRQSrc); EXTI15_IRQFnc(EXTI15_IRQSrc); }
__weak void	DMA_Channel1_IRQHandler(void) { DMA1_IRQFnc(DMA1_IRQSrc); }
__weak void	DMA_Channel2_3_IRQHandler(void) { DMA2_IRQFnc(DMA2_IRQSrc); DMA3_IRQFnc(DMA3_IRQSrc); }
__weak void	DMA_Channel4_5_6_7_IRQHandler(void) { DMA4_IRQFnc(DMA4_IRQSrc); DMA5_IRQFnc(DMA5_IRQSrc); DMA6_IRQFnc(DMA6_IRQSrc); DMA7_IRQFnc(DMA7_IRQSrc); }
__weak void	TIM1_BRK_UP_TRG_COMP_IRQHandler(void) { TIM1_IRQFnc(TIM1_IRQSrc); }
__weak void	TIM1_CC_IRQHandler(void) { Default_Handler(); }
__weak void	TIM2_IRQHandler(void) { TIM2_IRQFnc(TIM2_IRQSrc); }
__weak void	TIM3_IRQHandler(void) { TIM3_IRQFnc(TIM3_IRQSrc); }
__weak void	TIM6_DAC_LPTIM1_IRQHandler(void) { TIM6_IRQFnc(TIM6_IRQSrc); }
__weak void	TIM7_LPTIM2_IRQHandler(void) { TIM7_IRQFnc(TIM7_IRQSrc); }
__weak void	TIM14_IRQHandler(void) { TIM14_IRQFnc(TIM14_IRQSrc); }
__weak void	TIM15_IRQHandler(void) { TIM15_IRQFnc(TIM15_IRQSrc); }
__weak void	TIM16_IRQHandler(void) { TIM16_IRQFnc(TIM16_IRQSrc); }
__weak void	TIM17_IRQHandler(void) { TIM17_IRQFnc(TIM17_IRQSrc); }
__weak void	I2C1_IRQHandler(void) { I2C1_IRQFnc(I2C1_IRQSrc); }
__weak void	I2C2_IRQHandler(void) { I2C2_IRQFnc(I2C2_IRQSrc); }
__weak void	SPI1_IRQHandler(void) { Default_Handler(); }
__weak void	SPI2_IRQHandler(void) { Default_Handler(); }
__weak void	USART1_IRQHandler(void) { USART1_IRQFnc(USART1_IRQSrc); }
__weak void	USART2_IRQHandler(void) { USART2_IRQFnc(USART2_IRQSrc); }
__weak void	USART3_USART4_LPUART1_IRQHandler(void) { USART3_IRQFnc(USART3_IRQSrc); USART4_IRQFnc(USART4_IRQSrc); LPUART1_IRQFnc(LPUART1_IRQSrc); }

__attribute__((section(".isr_vector")))
const void (*IRQ[])(void) = {
  (const void (*)(void)) &_estack,
  Reset_Handler,
  NMI_Handler,
  HardFault_Handler,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  SVC_Handler,
  0,
  0,
  PendSV_Handler,
  SysTick_Handler,
  WWDG_IRQHandler,
  PVD_IRQHandler,
  RTC_STAMP_IRQHandler,
  FLASH_IRQHandler,
  RCC_IRQHandler,
  EXTI0_1_IRQHandler,
  EXTI2_3_IRQHandler,
  EXTI4_15_IRQHandler,
  UCPD1_UCPD2_IRQHandler,
  DMA_Channel1_IRQHandler,
  DMA_Channel2_3_IRQHandler,
  DMA_Channel4_5_6_7_IRQHandler,
  ADC_COMP_IRQHandler,
  TIM1_BRK_UP_TRG_COMP_IRQHandler,
  TIM1_CC_IRQHandler,
  TIM2_IRQHandler,
  TIM3_IRQHandler,
  TIM6_DAC_LPTIM1_IRQHandler,
  TIM7_LPTIM2_IRQHandler,
  TIM14_IRQHandler,
  TIM15_IRQHandler,
  TIM16_IRQHandler,
  TIM17_IRQHandler,
  I2C1_IRQHandler,
  I2C2_IRQHandler,
  SPI1_IRQHandler,
  SPI2_IRQHandler,
  USART1_IRQHandler,
  USART2_IRQHandler,
  USART3_USART4_LPUART1_IRQHandler,
  CEC_IRQHandler
};
