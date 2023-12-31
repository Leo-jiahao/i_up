/**
 * @file utilities.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-01
 * 
 * @copyright 
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) <2023>  <Leo-jiahao> 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */
#ifndef __UTILITIES_H
#define __UTILITIES_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
/** @addtogroup Utilities
  * @{
  */


/** @addtogroup Utilities crc
  * @{
  */



/** @defgroup Utilities crc Exported Functions
  * @{
  */

uint16_t crc16_1(uint8_t const *data, int len);
uint16_t crc16_2(uint8_t const *p_data, int32_t data_len);
uint16_t crc16_3(uint8_t const *p_data, int32_t data_len, uint16_t crc);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#endif

