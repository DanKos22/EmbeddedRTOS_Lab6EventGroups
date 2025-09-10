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

#define toggleBit (1<<3)
static void buttonTask(void * pvParameters);
static void LEDTask(void * pvParameters);
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
	xTaskCreate(LEDTask, "LED Task", 200, NULL, 2, NULL);
	LEDEventGroup = xEventGroupCreate();
	vTaskStartScheduler();

	while(1) {
	}
}



void buttonTask(void * pvParameters) {
	printf("Starting button task \r\n\n");
	while(1) {
		//wait for switch press
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin)== 0){
			printf("Switch press detected, giving semaphore\r\n\n");

			//Set event group bit
			xEventGroupSetBits(LEDEventGroup, toggleBit);

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
		LEDBits = xEventGroupWaitBits(LEDEventGroup, toggleBit, pdTRUE, pdFALSE, portMAX_DELAY);
		if(LEDBits & toggleBit){
			printf("Toggling LED...\r\n\n");
			HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		}
	}
}
