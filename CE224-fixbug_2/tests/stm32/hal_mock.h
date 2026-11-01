#ifndef CE224_HAL_MOCK_H
#define CE224_HAL_MOCK_H
/* Only the HAL surface used by the host regression harness. */
#define __MAIN_H
#define __I2C_H__
#include <stdint.h>
#include <stdbool.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
typedef struct { unsigned unused; } I2C_HandleTypeDef;
typedef struct { uint32_t counter; } TIM_HandleTypeDef;
typedef struct { uint32_t DEMCR, CYCCNT, CTRL; } MockCore;
extern MockCore mock_core;
extern uint32_t SystemCoreClock;
#define DWT (&mock_core)
#define CoreDebug (&mock_core)
#define CoreDebug_DEMCR_TRCENA_Msk 1U
#define DWT_CTRL_CYCCNTENA_Msk 1U
#define SysTick_IRQn 0
#define TIM4_IRQn 1
#define __DMB() ((void)0)
#define __HAL_TIM_SET_COUNTER(h,v) ((h)->counter=(v))
#define __HAL_TIM_GET_COUNTER(h) ((h)->counter)
#define TIM_CHANNEL_ALL 0
extern I2C_HandleTypeDef hi2c1;
extern int mock_busy;
#define I2C_FLAG_BUSY 1
#define __HAL_I2C_GET_FLAG(h,f) (mock_busy)
#define I2C_MEMADD_SIZE_8BIT 1
#define __HAL_RCC_I2C1_FORCE_RESET() ((void)0)
#define __HAL_RCC_I2C1_RELEASE_RESET() ((void)0)
#define GPIOC 0
#define GPIO_PIN_13 13
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *,uint16_t,uint16_t,uint16_t,uint8_t *,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *,uint16_t,uint16_t,uint16_t,uint8_t *,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *,uint16_t,uint32_t,uint32_t);
void HAL_GPIO_TogglePin(int,int);
void HAL_NVIC_SetPriority(int,int,int);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *);
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *,uint32_t);
#endif
