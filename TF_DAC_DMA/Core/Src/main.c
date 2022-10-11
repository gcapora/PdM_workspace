/*******************************************************************************
  * @file		main.c
  * @brief      Generador de señales con entrada por UART
  * @author		Guillermo F. Caporaletti <gfcaporaletti@undav.edu.ar>
  * @detail		Recibe una entrada por UART y la saca por DAC.
  ******************************************************************************
  * @attention
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"

/* Private define ------------------------------------------------------------*/
#define N_MUESTRAS			105		// Muestras en un período de señal
#define LARGO_PAQUETE		5		// Paquete de datos recibidos por UART
#define LARGO_MAX_PAQUETE	16		// Lo admisible ante error de transmisión
#define TIEMPO_ENCENDIDO_1	100		// <-- tiempos para parpadeo de leds
#define TIEMPO_ENCENDIDO_2	500
#define TIEMPO_PARPADEO_ESPERA		1000
#define TIEMPO_PARPADEO_CARGADO		750
#define TIEMPO_PARPADEO_ENCENDIDO	75

/* Private typedef -----------------------------------------------------------*/

// Los estados por los que puede pasar cada generador (DAC1 o DAC2)
typedef enum {
	Espera,
	Vacio = Espera,
	Recibiendo,
	Cargado,
	Generando,
	Pausa
} estadosMEF;

// Cada generador tiene estado y senial almacenada
typedef struct {
	estadosMEF estado;
	bool encendido;			// Este bool es redundante
	bool cargado;			// Este bool es redundante
	uint32_t senial[N_MUESTRAS];
} generador_t;

/* Private variables HAL ------------------------------------------------------*/
DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac2;
TIM_HandleTypeDef htim2;

/* Variables privadas USUARIO -------------------------------------------------*/
generador_t GeneradorDAC2;
uint32_t Senial[N_MUESTRAS] = {
		0,6,19,39,66,100,142,190,244,306,373,
		447,526,611,700,795,894,997,1104,1215,1328,1444,1562,1682,1803,1925,
		2048,2170,2292,2413,2533,2651,2767,2880,2991,3098,3201,3300,3395,3484,3569,
		3648,3722,3789,3851,3905,3953,3995,4029,4056,4076,4089,4095,4093,4084,4067,
		4044,4013,3975,3930,3879,3821,3756,3686,3609,3527,3440,3348,3251,3150,3045,
		2936,2824,2709,2592,2473,2353,2231,2109,1986,1864,1742,1622,1503,1386,1271,
		1159,1050,945,844,747,655,568,486,409,339,274,216,165,120,82,
		51,28,11,2
};								// Senial inicial y donde se almacenarán nuevas señales
uint16_t MuestraNro = 0;		// Posición en que se va a almacenar próxima muestra

delay_t parpadeoLedAzul;		// Parpadeo en estados Cargado, Generando y Pausa
delay_t parpadeoLedVerde;		// Parpadeo en estados Espera y Recibiendo

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM2_Init(void);

static void Gen_Init(generador_t *GEN);
static void Gen_Espera(generador_t *GEN);
static void Gen_Recibir(generador_t *GEN);
static void Gen_Cargar(generador_t *GEN, uint32_t Senial[]);
static void Gen_Encender(generador_t *GEN);
static void Gen_Pausar(generador_t *GEN);
static void Leer_UART(generador_t *GEN);	// <-- Esta rutina lee de a un caracter
static void Cargar_Paquete(generador_t *GEN, uint8_t PaqueteRecibido[]);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  Error_Handler_Init();		// Errores según <errorHandler.h>
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DAC_Init();
  MX_TIM2_Init();

  /* Inicializacion de periféricos y APIs -------------------------------------*/
  if (!uartInit()) Error_Handler();			// Conexión con terminal
  debounceFSM_init();						// MEF antirrebote
  BSP_LED_Init(LED_BLUE);					// LED AZUL que será usado como indicador
  BSP_LED_Init(LED_GREEN);					// LED VERDE
  delayInit( &parpadeoLedAzul, TIEMPO_PARPADEO_CARGADO);
  delayInit( &parpadeoLedVerde, TIEMPO_PARPADEO_ESPERA);
  Gen_Init( &GeneradorDAC2 );				// Inicialización del generador de señal

  /* Inicio... ----------------------------------------------------------------*/
  HAL_TIM_Base_Start(&htim2);
  uartSendString((uint8_t *) "\nGENERADOR DE SENIAL V1.0\nTiempo entre muestras: 10,5 Msps\nTension: 0V - 3,3V");
  uartSendString((uint8_t *) "\n\n");
  Gen_Espera( &GeneradorDAC2 );	// Estado inicial del generador

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // Reviso botón de usuario:
	  debounceFSM_update();

	  // Analizo parpadeo LED VERDE:
	  if ( (GeneradorDAC2.estado == Espera) && delayRead( &parpadeoLedVerde )) BSP_LED_Toggle(LED1);

	  // Analizo parpadeo LED AZUL:
	  if ( (GeneradorDAC2.estado >= Cargado) && delayRead( &parpadeoLedAzul )) BSP_LED_Toggle(LED2);

	  // Si estoy en estado = Recibiendo o Cargado, espero recibir una señal
	  if (GeneradorDAC2.estado == Recibiendo || GeneradorDAC2.estado == Cargado) {
          Leer_UART(&GeneradorDAC2);
	  }

	  // Si presionamos el botón de usuario cambiamos de estadoMEF
	  if (readKeyPush()) {
		  switch (GeneradorDAC2.estado) {

		  case Espera:
			  Gen_Recibir( &GeneradorDAC2 );
			  break;

		  case Recibiendo:
			  // Gen_Cargar( &GeneradorDAC2, Senial);
			  break;

		  case Cargado:
			  Gen_Encender( &GeneradorDAC2 );
			  break;

		  case Generando:
			  Gen_Pausar( &GeneradorDAC2 );
			  break;

		  case Pausa:
			  Gen_Encender( &GeneradorDAC2);
			  break;

		  default:
			  // nada...
			  break;
		  }
	  }

	  // Si hay flanco descendente (señal lógica de 1 a 0), evalúo si fue un pulso largo:
	  if (readKeyRelease()) {
		  if ((readPresionadoLargo() == true) &&  GeneradorDAC2.estado >= Generando)
			  Gen_Espera( &GeneradorDAC2 );
	  }
   }
}

/*******************************************************************************
  * @brief  Inicializa Generador
  * @param  Estructura de datos del generador
  * @retval None
  */
static void Gen_Init(generador_t *GEN) {
	GEN->cargado = false;
	GEN->encendido = false;
	GEN->estado = Espera;
}

/*******************************************************************************
  * @brief  Establece al generador en espera
  * @param  Estructura de datos del generador
  * @retval None
  */
static void Gen_Espera(generador_t *GEN) {
	// Apago led azul     = señal cargada
	// Parpadeo led verde = en espera
	BSP_LED_Off(LED_BLUE);
	delayWrite( &parpadeoLedVerde, TIEMPO_PARPADEO_ESPERA );

	// Actualizo estructura
	GEN->cargado = false;
	GEN->estado = Espera;

	// Finalmente corto señal del generador
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_2);

	// Y envío informe a UART
    uartSendString((uint8_t *) "Generador vacio y en espera...\n");
    uartClearBuffer();
}

/*******************************************************************************
  * @brief  Pasa a estado de receción de señal
  * @param  Estructura de datos del generador
  * @retval None
  */
static void Gen_Recibir(generador_t *GEN) {
	GEN->estado = Recibiendo;
	BSP_LED_On(LED1);
    uartSendString((uint8_t *) "Listo para recibir...\n");
    uartClearBuffer();
}

/*******************************************************************************
  * @brief  Carga señal en el generador
  * @param  Estructura de datos del generador
  * @retval None
  */
static void Gen_Cargar(generador_t *GEN, uint32_t Senial[]) {
	for (uint16_t i=0; i<N_MUESTRAS; i++) GEN->senial[i] = Senial[i];
	GEN->cargado = true;
	BSP_LED_Off(LED1);
	GEN->estado = Cargado;
	delayWrite( &parpadeoLedAzul, TIEMPO_PARPADEO_CARGADO );
    uartSendString((uint8_t *) "Senial cargada en generador.\n");
    uartClearBuffer();
}

static void Gen_Encender(generador_t *GEN) {
	// Acelero el parpadeo del led azul
	delayWrite( &parpadeoLedAzul, TIEMPO_PARPADEO_ENCENDIDO );

	// Actualizo estructura
	GEN->estado = Generando;
	GEN->encendido = true;

	// Enciendo generador
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, GeneradorDAC2.senial, N_MUESTRAS, DAC_ALIGN_12B_R);

	// Reinicializo índice de carga
	MuestraNro = 0;

	// Muestro mensaje de esto.
	uartSendString((uint8_t *) "Generador 2 encendido.\n\r");

}

static void Gen_Pausar(generador_t *GEN) {
	GEN->estado = Pausa;
	GEN->encendido = false;

	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_2);

	uartSendString((uint8_t *) "Generador 2 en pausa.\n\r");
	delayWrite( &parpadeoLedAzul, TIEMPO_PARPADEO_CARGADO );
}


static void Leer_UART(generador_t *GEN){
	static uint8_t Buffer[LARGO_MAX_PAQUETE];	//Aquí guardo los caracteres antes de mandar el paquete
	static uint8_t Buffer_pos=0;
	uint8_t Leo;

	while (uartReceiveStringSize( &Leo, 1) == true) {
		// Mientras esté recibiendo por UART me quedo en este loop

		if ( Leo>=48 && Leo<=57 ) {
			// Es un dígito numérico: Debo almacenarlo en el Buffer
			Buffer[Buffer_pos] = Leo;
			Buffer_pos++;
			Buffer_pos = (Buffer_pos >= LARGO_MAX_PAQUETE) ? (LARGO_MAX_PAQUETE - 1) : Buffer_pos;

		} else if (Leo == ',') {
			// Es el fin de un paquete: Debo cargarlo en la Senial
			// 1) Le damos formato de string (debe terminar en \0)
			for (uint16_t i=Buffer_pos; i<LARGO_MAX_PAQUETE; i++) Buffer[i] = '\0';
			// 2) Enviamos a cargarlo...
			Cargar_Paquete(GEN, Buffer);
			Buffer_pos=0;

		}   // Si no es número ni ',', no lo considero.
	}
}

static void Cargar_Paquete(generador_t *GEN, uint8_t PaqueteRecibido[]) {
	// PaqueteRecibido es un string?
	uint32_t Largo = strlen((char *) PaqueteRecibido);
	if ( Largo > LARGO_MAX_PAQUETE) Error_Handler();

	// Transformamos string recibido en número y lo asignamos a Senial[]
	uint32_t Numero = 0;
	Numero = atoi((char *) PaqueteRecibido );
	Senial[MuestraNro] = Numero;

	// Informamos en UART
	char muestra_str[16];
	sprintf(muestra_str, "%u", MuestraNro);
	uartSendString((uint8_t *) "Muestra #");
	uartSendString((uint8_t *) muestra_str);
	uartSendString((uint8_t *) ": ");
	uartSendString((uint8_t *) PaqueteRecibido);
	uartSendString((uint8_t *) "\n");

	// Incrementamos para próxima carga y verificamos si ya completamos
	MuestraNro++;
	if (MuestraNro >= N_MUESTRAS) {
		MuestraNro = 0;
		Gen_Cargar( GEN, Senial);
	}
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{
  DAC_ChannelConfTypeDef sConfig = {0};

  /** DAC Initialization   */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT2 config   */
  sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 7 ;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

}

#ifdef  USE_FULL_ASSERT
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
