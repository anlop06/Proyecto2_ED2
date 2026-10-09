/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	ESTADO_MENU, // Esperando 1 o 2
	ESTADO_SPI_CMD // Esperando comando tipo: "1,500"
} Estado_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_Tamaño_Buffer 32
#define RX_TIMEOUT_MS 30
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//uint8_t TxData, RxData;

uint8_t rxByte;
char rxLinea[RX_Tamaño_Buffer]; // Línea que escribe el usuario en la terminal
volatile uint8_t rxIndex = 0; // Posición del cursor en línea de la terminal
volatile uint8_t lineaEnviada = 0; // Si es igual a uno, indica que el usuario presionó Enter
Estado_t estado = ESTADO_MENU; // Estado actual del sistema
volatile uint32_t ultimoRx = 0; // Instante en ms en que llegó el último byte

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
// Prototipos de funciones
void contenidoUart(const char *s);
void mostrarMenu(void);
void mostrarSubmenuSPI(void);
void procesarLinea(void);
uint8_t validarComandoSPI(const char *linea, uint8_t *led, uint16_t *tiempo);
void enviarComandoSPI(uint8_t led, uint16_t tiempo);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  // TxData = 25;

  // Activamos Slave
  // HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_RESET);
  // HAL_SPI_TransmitReceive(&hspi2, &TxData, &RxData, 1, HAL_MAX_DELAY);

  // Desactivamos Slave
  // HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);

  //*************************************************************************/
  // Inicio programación proyecto
  //*************************************************************************/
  // Recepción usando ISR
  HAL_UART_Receive_IT(&huart2, &rxByte, 1); // Arma la recepción de la interrupción
  mostrarMenu(); // Muestra una vez al inicio del programa las opciones del menú
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

//	  TxData = 1;
//
//	  // Activamos slave
//	  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_RESET);
//	  HAL_SPI_TransmitReceive(&hspi2, &TxData, &RxData, 1, HAL_MAX_DELAY);
//
//	  // Desactivamos Slave
//	  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);
//	  HAL_Delay(500);
//
//	  TxData = 0;
//
//	  // Activamos slave
//	  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_RESET);
//	  HAL_SPI_TransmitReceive(&hspi2, &TxData, &RxData, 1, HAL_MAX_DELAY);
//
//	  // Desactivamos Slave
//	  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);
//	  HAL_Delay(500);

	  //*************************************************************************/
	  // Inicio programación proyecto
	  //*************************************************************************/
	  if (!lineaEnviada && rxIndex > 0 && (HAL_GetTick() - ultimoRx) > RX_TIMEOUT_MS) {
	  		rxLinea[rxIndex] = '\0';
	  		lineaEnviada = 1;
	  	}


	  if (lineaEnviada) {
		procesarLinea();
		rxIndex = 0; // Se limpia el índice para que lea desde el inicio de la línea
		lineaEnviada = 0; // Se reinicia la bandera
	}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ESP32_CS_Pin */
  GPIO_InitStruct.Pin = ESP32_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ESP32_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//*************************************************************************/
// Definiciones de funciones
//*************************************************************************/

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	if (huart-> Instance == USART2) {

		ultimoRx = HAL_GetTick(); // marca de tiempo del último byte

		if (rxByte == '\r' || rxByte == '\n') {
			// Si el usuario presiona enter, se cierra la cadena (solo si hay algo escrito)
			if (rxIndex > 0 && !lineaEnviada) {
				rxLinea[rxIndex] = '\0'; // Convierte el arreglo en una cadena de C
				lineaEnviada = 1;
			}
		}

		else if (!lineaEnviada && (rxIndex < RX_Tamaño_Buffer - 1)) {

			rxLinea[rxIndex++] = rxByte; // Se guarda el carácter

		}

		HAL_UART_Receive_IT(&huart2, &rxByte, 1);
	}

}

void contenidoUart(const char *s) {

	HAL_UART_Transmit(&huart2, (uint8_t *)s, strlen(s), 100);

}

void mostrarMenu(void) {

	contenidoUart("\r\n========= MENU PRINCIPAL =========\r\n"
			"1. Controlar dispositivo SPI\r\n"
			"2. Obtener medicion de sensor I2C\r\n"
			"Seleccione una opcion: ");
}

void mostrarSubmenuSPI(void) {
	contenidoUart("\r\n--------- Control SPI ---------\r\n"
			"Formato: LED,tiempo_ms (ej. 1,500)\r\n"
			"LED: 1 a 3\r\n"
			"Tiempo: 1 a 65535 ms\r\n"
			"Ingrese la opcion deseada: ");
}

void procesarLinea(void) {

	switch (estado) {

		case ESTADO_MENU:
			if (strcmp(rxLinea, "1") == 0) {
				estado = ESTADO_SPI_CMD;
				mostrarSubmenuSPI();
			}

			else if (strcmp(rxLinea, "2") == 0) {
				contenidoUart("\r\nOpcion no implementada (por el momento)\r\n");
				mostrarMenu();

			}
			else {
				contenidoUart("\r\nOpcion no valida. Ingresa 1 o 2.\r\n");
				mostrarMenu();
			}

			break;

		case ESTADO_SPI_CMD: {

			uint8_t led;
			uint16_t tiempo;

			if (validarComandoSPI(rxLinea, &led, &tiempo)) {

				enviarComandoSPI(led, tiempo);

				char mensaje[64];
				snprintf(mensaje, sizeof(mensaje), "\r\nEnviado por SPI: LED %u, %u ms", (unsigned)led, (unsigned)tiempo);

				contenidoUart(mensaje);

				// TODO parte 5: aquí se actualizará el periodo del timer con 'tiempo'

				estado = ESTADO_MENU;
				mostrarMenu();


			} else {
				contenidoUart("\r\nComando invalido, ingrese una instruccion valida.\r\n");
				mostrarSubmenuSPI(); // Se vuelve a mostrar el submenú del SPI
			}

			break;
		}
	}

}

uint8_t validarComandoSPI(const char *linea, uint8_t *led, uint16_t *tiempo) {

    char *coma = strchr(linea, ',');
    if (coma == NULL) return 0; // No hay coma

    char *fin;
    long l = strtol(linea, &fin, 10); // Parte antes de la coma
    if (fin != coma) return 0; // Había basura antes de la coma

    long t = strtol(coma + 1, &fin, 10); // Parte después de la coma
    if (fin == coma + 1 || *fin != '\0') return 0; // Vacío o con basura

    if (l < 1 || l > 3) return 0; // LED fuera de rango
    if (t < 1 || t > 65535) return 0; // Tiempo fuera de rango

    *led = (uint8_t)l;
    *tiempo = (uint16_t)t;
    return 1;
}

void enviarComandoSPI(uint8_t led, uint16_t tiempo) {

	uint8_t dataTx[3];

	dataTx[0] = led; // Número de LED
	dataTx[1] = (tiempo >> 8) & 0xFF; // Byte alto del tiempo
	dataTx[2] = tiempo & 0xFF; // Byte bajo del tiempo

	HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_RESET); // Se activa la comunicación con ESP32
	HAL_SPI_Transmit(&hspi2, dataTx, 3, 100);
	HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET); // Se desactiva la comunicación con ESP32

}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
