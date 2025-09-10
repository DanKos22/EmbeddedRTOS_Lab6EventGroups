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

#define buttonBit (1<<3)
#define uartBit (1<<2)
#define syncBits (buttonBit | uartBit)

static void buttonTask(void * pvParameters);
static void waitTask(void * pvParameters);
static void UARTTask(void * pvParameters);
EventGroupHandle_t syncEventGroup = NULL;
TaskHandle_t waitTaskHandle = NULL;

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}


void userApp() {
	printf("Starting application\r\n\n");

	//HAL_UART_Receive_IT(&huart1, &ch, 1);
	//HAL_TIM_Base_Start_IT(&htim6);

	xTaskCreate(buttonTask, "Button Task", 200, NULL, 3, NULL);
	xTaskCreate(waitTask, "Wait Task", 200, NULL, 1, &waitTaskHandle);
	xTaskCreate(UARTTask, "UART Task", 200, NULL, 2, NULL);
	syncEventGroup = xEventGroupCreate();
	vTaskStartScheduler();

	while(1) {
	}
}



void buttonTask(void * pvParameters) {
	printf("Starting button task \r\n\n");
	while(1) {
		//wait for switch press
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin)== 0){
			printf("Switch press detected, setting event group bit...\r\n\n");

			//Set event group bit
			xEventGroupSync(syncEventGroup, buttonBit, syncBits, portMAX_DELAY);
			printf("Button task synchronized...\r\n\n");

			//wait for switch release
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin)== 0);
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	//Sync complete
	while(1){
		if(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0){
			printf("Switch press detected at %lums\r\n", xTaskGetTickCount()*1000/configTICK_RATE_HZ);
			//Wait for switch release
			while(HAL_GPIO_ReadPin(BUTTON_EXTI13_GPIO_Port, BUTTON_EXTI13_Pin) == 0);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void UARTTask(void * pvParameters) {
	uint8_t ch;
	printf("Starting UART task...\r\n\n");
	while(1) {

		if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != 0){
			HAL_UART_Receive(&huart1, &ch,  1,  100);
			if(ch == 's'){
				printf("Character 's' entered, setting event group bit...\r\n");
				xEventGroupSync(syncEventGroup, uartBit, syncBits, portMAX_DELAY);
				printf("UART task synchronized...\r\n\n");
				break;
			}
			else {
				printf("Invalid entry, enter 's' to pause/resume timer\r\n\n");
			}
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	//After syncing
	printf("Synchronization complete, deleting wait task\r\n\n");
	vTaskDelete(waitTaskHandle);
	while(1){
		if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != 0){
			HAL_UART_Receive(&huart1, &ch,  1,  100);
			printf("Character received: %c\r\n", ch);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}


void waitTask(void * pvParameter){
	printf("Starting wait Task\r\n\n");
	printf("Wait task will run until button task and UART task are synchronized\r\n\n");
	printf("LED will flash slowly while waiting for synchronization\r\n\n");
	while(1){
		printf("Synchronizing ...\r\n\n");
		HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
