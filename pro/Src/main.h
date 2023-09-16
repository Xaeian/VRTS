//-------------------------------------------------------------------------------------------------

#define INCLUDE_DBG 1
#define INCLUDE_RTC 1

#define GPIO_INCLUDE_GPIF 1

#define I2C_DMA_TX 0
#define I2C_DMA_RX 0
#define ISM330_INCLUDE_SPI 1

#define STREAM_ADDRESS 1
#define SHAKE_DBG 0

//-------------------------------------------------------------------------------------------------

#ifndef MAIN_H_
#define MAIN_H_

typedef enum {
  HASH_Config = 4142165115,
  HASH_Reset = 273105544,
  HASH_Values = 574111733,
  HASH_Series = 461651792,
  HASH_Print = 271190290,
  HASH_Settings = 319085590,
  HASH_Acc = 193485996,
  HASH_Gyro = 2090310758,
  HASH_Status = 478842185
} HASH_e;

#endif

//-------------------------------------------------------------------------------------------------
