/*******************************************************************************
* @file    errorHandler.h
* @author  Guillermo Caporlaetti
* @brief   Función en caso de error.
********************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ERROR_HANDLER_H
#define __ERROR_HANDLER_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"  		/* <- HAL include */
#include "stm32f4xx_nucleo_144.h" 	/* <- BSP include */

/* Types ---------------------------------------------------------------------*/

/* Constants -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void Error_Handler(void);
void Error_Handler_Init(void);

/* Exported macro ------------------------------------------------------------*/

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
