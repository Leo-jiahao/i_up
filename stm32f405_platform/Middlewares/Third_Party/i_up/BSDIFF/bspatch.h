/**
 * @file bspatch.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-05
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
#ifndef __BSPATCH_H
#define __BSPATCH_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>


/** @addtogroup Utilities 
  * @{
  */
/** @addtogroup bspatch
  * @{
  */

/** @defgroup  bspatch
  * @{
  */
/**
 * @brief 
 * 
 * @param addr 内部flash，APP_SAVE 区域
 * @param size 字节长度
 * @param src 源地址
 * @return int 
 */
typedef int (*write)(void * addr, uint32_t size, void *src);



/** @defgroup  Exported Functions
  * @{
  */
int bspatch(
    const uint8_t *old  , int32_t old_size, 
    uint8_t       *new  , int32_t new_size, 
    const uint8_t *patch, int32_t patch_size, write __write_flash);
int32_t offtin(uint8_t *buff);
/**
  * @}
  */

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
