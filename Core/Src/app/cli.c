#include "app/cli.h"
#include <string.h>
#include <stdio.h>

#define CLI_BUFFER_SIZE 64

static UART_HandleTypeDef *cli_huart;
static char cli_buffer[CLI_BUFFER_SIZE];
static uint8_t cli_index = 0;
static uint8_t rx_char;

void CLI_Init(UART_HandleTypeDef *huart)
{
    cli_huart = huart;
    HAL_UART_Receive_IT(cli_huart, &rx_char, 1); // start first RX interrupt
}

void CLI_Process(void)
{
    // nothing to do here in polling mode
}

// ISR callback (called by HAL when RX interrupt fires)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == cli_huart)
    {
        if (rx_char == '\r' || rx_char == '\n')
        {
            cli_buffer[cli_index] = '\0'; // end string

            if (cli_index > 0)
            {
                if (strncmp(cli_buffer, "cd ", 3) == 0)
                {
                    char *folder = cli_buffer + 3;
                    printf("Zmieniono folder na %s\r\n", folder);
                }
                else
                {
                	printf("Command unknown!\r\n");
                }
            }

            cli_index = 0; // reset buffer
        }
        else if (cli_index < CLI_BUFFER_SIZE - 1)
        {
            cli_buffer[cli_index++] = rx_char;
        }

        // re-enable interrupt for next char
        HAL_UART_Receive_IT(cli_huart, &rx_char, 1);
    }
}
