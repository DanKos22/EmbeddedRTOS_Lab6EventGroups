/*
 * userApp.c
 *
 *  Created on: Dec 8, 2023
 *      Author: Niall.OKeeffe@atu.ie
 */

#include "userApp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>

//--------------------------------------------------------------
//used for real time stats, do not delete code from this section
extern TIM_HandleTypeDef htim7;
extern volatile unsigned long ulHighFrequencyTimerTicks;
void configureTimerForRunTimeStats(void)
{
    ulHighFrequencyTimerTicks = 0;
    HAL_TIM_Base_Start_IT(&htim7);
}
unsigned long getRunTimeCounterValue(void)
{
	return ulHighFrequencyTimerTicks;
}
//end of real time stats code
//----------------------------------------------------------------

extern UART_HandleTypeDef huart1;

#define onBit (1<<3)
#define offBit (1<<2)

static void buttonTask(void * pvParameters);
static void LEDTask(void * pvParameters);
static void UARTTask(void * pvParameters);
EventGroupHandle_t LEDEventGroup = NULL;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	//HAL_UART_Receive_IT(&huart1, &ch, 1);
	//HAL_TIM_Base_Start_IT(&htim6);

	xTaskCreate(buttonTask, "Button Task", 200, NULL, 1, NULL);
	xTaskCreate(LEDTask, "LED Task", 200, NULL, 3, NULL);
	xTaskCreate(UARTTask, "UART Task", 200, NULL, 2, NULL);
	LEDEventGroup = xEventGroupCreate();
	vTaskStartScheduler();

	while(1) {
	}
}



void buttonTask(void * pvParameters) {
	EventBits_t buttonBit;
	printf("Starting button task \r\n\n");
	while(1) {
		//wait for switch press
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin)== 0){
			printf("Switch press detected, \r\n\n");

			//Set event group bit
			xEventGroupSetBits(LEDEventGroup, onBit);

			//wait for switch release
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin)== 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void LEDTask(void * pvParameter){
	EventBits_t LEDBits;
	printf("Starting LED Task\r\n\n");
	while(1){
		printf("LED task blocked, waiting for event bit to be set\r\n\n");
		LEDBits = xEventGroupWaitBits(LEDEventGroup, onBit|offBit, pdTRUE, pdFALSE, portMAX_DELAY);
		if(LEDBits & offBit){
			printf("Turning LED off...\r\n\n");
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 0);
		}
		else if(LEDBits & onBit){
			printf("Turning LED on...\r\n\n");
			HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, 1);
		}
	}
}

void UARTTask(void * pvParameters) {
	uint8_t ch;
	printf("Starting UART task...\r\n\n");
	while(1) {

		if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != 0){
			HAL_UART_Receive(&huart1, &ch,  1,  100);
			if(ch == 'o'){
				printf("Character 'o' entered, setting event group bit...\r\n");
				xEventGroupSetBits(LEDEventGroup, offBit);
			}
			else {
				printf("Invalid entry, enter 'o' to pause/resume timer\r\n\n");
			}

		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}
