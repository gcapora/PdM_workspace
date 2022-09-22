/*******************************************************************************
* @file    API_uart.c
* @author  Guillermo Caporaletti
* @brief   Módulo para manejo de puerto UART.
********************************************************************************
*
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "API_uart.h"
#include "error_Handler.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* UART handler declaration */
UART_HandleTypeDef UartHandle;

/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Inicializa la conexión por UART
  * @param  None
  * @retval None
  */
bool_t uartInit() {
  /*##-1- Configure the UART peripheral ######################################*/
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  /* UART configured as follows:
      - Word Length = 8 Bits (7 data bit + 1 parity bit) : 
	                  BE CAREFUL : Program 7 data bits + 1 parity bit in PC HyperTerminal
      - Stop Bit    = One Stop bit
      - Parity      = ODD parity
      - BaudRate    = 9600 baud
      - Hardware flow control disabled (RTS and CTS signals) */
  UartHandle.Instance          = USARTx;
  UartHandle.Init.BaudRate     = 9600;
  UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits     = UART_STOPBITS_1;
  UartHandle.Init.Parity       = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode         = UART_MODE_TX_RX;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
  /*##########################################################################*/

  return (HAL_UART_Init(&UartHandle) == HAL_OK);
}

/**
  * @brief  Transmite por UART
  * @param  None
  * @retval None
  */
void uartSendString(uint8_t * pstring) {
	// hola!!!
}

/**
  * @brief  Transmite por UART una cantidad definida de caracteres
  * @param  None
  * @retval None
  */
void uartSendStringSize(uint8_t * pstring, uint16_t size) {
	// hola2!!
}

/**
  * @brief  Recibe por UART una cantidad definida de caracteres
  * @param  None
  * @retval None
  */
void uartReceiveStringSize(uint8_t * pstring, uint16_t size) {
	// chau!
}

/***************************************************************END OF FILE****/
