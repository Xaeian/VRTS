#include "stm32g0xx.h"
#include "vrts.h"

static void Thread_1(void);
static void Thread_2(void);
static void Thread_3(void);

static uint32_t stack_1[128];
static uint32_t stack_2[128];
static uint32_t stack_3[128];

int main(void)
{
  RCC->IOPENR |= RCC_IOPSMENR_GPIOASMEN;
  GPIOA->MODER = (GPIOA->MODER & ~(0x03 << (2 * 5))) | (0x01 << (2 * 5));
  // set PA5 as output (LED)
  SYSTICK_Init(10); // basetime 10ms
  thread(&Thread_1, stack_1, sizeof(stack_1));
  thread(&Thread_2, stack_2, sizeof(stack_2));
  thread(&Thread_3, stack_3, sizeof(stack_3));
  VRTS_Init();
  while(1);
}

static void Thread_1(void)
{
  while(1) {
    for(int i = 0; i < 8; i++) {
      GPIOA->ODR ^= (1 << 5); // PA5 toggle LED (blinking)
      sleep(250); // 4 x 250ms
    }
    let();
  }
}

static void Thread_2(void)
{
  while(1) {
    for(int i = 0; i < 14; i++) {
      GPIOA->ODR ^= (1 << 5); // PA5 toggle LED (blinking)
      sleep(100); // 7 x 100ms
    }
    let();
  }
}

static void Thread_3(void)
{
  while(1) {
    for(int i = 0; i < 4; i++) {
      GPIOA->ODR ^= (1 << 5); // PA5 toggle LED (blinking)
      sleep(500); // 2 x 500ms
    }
    let();
  }
}
