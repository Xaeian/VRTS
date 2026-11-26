// example.c

// Three LEDs blink apart, each in its own thread: PA12, PA11 and PC7 of a G0 board

#include <stm32g0xx.h>
#include "vrts.h"

static void Thread_1(void);
static void Thread_2(void);
static void Thread_3(void);

stack(stack_1, 128);
stack(stack_2, 128);
stack(stack_3, 128);

int main(void)
{
  // PA12, PA11 and PC7 as outputs
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOCEN;
  GPIOA->MODER &= ~((0x03 << (2 * 12)) | (0x03 << (2 * 11)));
  GPIOC->MODER &= ~(0x03 << (2 * 7));
  GPIOA->MODER |= (0x01 << (2 * 12)) | (0x01 << (2 * 11));
  GPIOC->MODER |= (0x01 << (2 * 7));
  systick_init(10); // 10ms base tick
  thread(Thread_1, stack_1);
  thread(Thread_2, stack_2);
  thread(Thread_3, stack_3);
  vrts_init(); // does not return
  while(1);
}

static void Thread_1(void)
{
  while(1) {
    for(int i = 0; i < 8; i++) {
      GPIOA->ODR ^= (1 << 12); // PA12 toggle LED
      delay(250); // 4 x 250ms
    }
    let();
  }
}

static void Thread_2(void)
{
  while(1) {
    for(int i = 0; i < 14; i++) {
      GPIOA->ODR ^= (1 << 11); // PA11 toggle LED
      delay(100); // 7 x 100ms
    }
    let();
  }
}

static void Thread_3(void)
{
  while(1) {
    for(int i = 0; i < 4; i++) {
      GPIOC->ODR ^= (1 << 7); // PC7 toggle LED
      delay(500); // 2 x 500ms
    }
    let();
  }
}
