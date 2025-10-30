#include "app/app_main.h"
#include <stdio.h>


int app_main(){

//	CLI_Init(&huart2);

	while(1){
		  HAL_GPIO_TogglePin(GPIOD, LD4_Pin);  // Zmień stan diody PD12
		  printf("Zmieniono stan\r\n");
		  HAL_Delay(1000); // Odczekaj 500 ms
	}


}
