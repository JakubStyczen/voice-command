#ifndef CLI_H
#define CLI_H

#include "stm32f4xx_hal.h"

void CLI_Init(UART_HandleTypeDef *huart);
void CLI_Process(void);

#endif
