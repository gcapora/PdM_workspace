/*******************************************************************************
* @file    errorHandler.c
* @author  Guillermo Caporaletti
* @brief   Functions to call in case of error.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/error_Handler.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define y const ----------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/

/*******************************************************************************
* @brief  This function is executed in case of error occurrence.
* @param  None
* @retval None
*/
void Error_Handler_Init(void)
{
  /* Inicializa el led de alarma
   * para garantizar que esté operativo */
  BSP_LED_Init(LED_RED);
}

/*******************************************************************************
* @brief  This function is executed in case of error occurrence.
* @param  None
* @retval None
*/
void Error_Handler(void)
{
  /* Inicializo, por si no se había hecho */
  Error_Handler_Init();

  /* Turn LED2 on */
  BSP_LED_Off(LED_RED);
  BSP_LED_Off(LED_BLUE);
  BSP_LED_Off(LED_GREEN);
  while (1)
  {
	  BSP_LED_Toggle(LED_RED);
	  HAL_Delay(250);
  }
}

//******************************************************************************
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/***************************************************************END OF FILE****/
