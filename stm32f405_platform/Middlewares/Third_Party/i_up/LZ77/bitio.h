/**
 * @file bitio.h
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
#ifndef __BITIO_H
#define __BITIO_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#define READ_U8(address) (*((volatile uint8_t *)(address)))
/** @addtogroup Utilities 
  * @{
  */
/** @addtogroup bitio
  * @{
  */

/** @defgroup  bitio
  * @{
  */
typedef struct
{
    uint32_t file_addr;
    int bytepos;
    int bitpos;
    int filesize;//byte大小
    int seek;
    /**
   * @brief 
   * 
   * @param addr 内部flash，APP_SAVE 区域
   * @param size 字节长度
   * @param src 源地址
   * @return int 
   */
    int (*write)(void * addr, uint32_t size, void *src);

}bFile_t;



/** @defgroup  Exported Functions
  * @{
  */
bFile_t *bitIO_open(void *fileaddr, int32_t file_size);
int bitof(int n);
int bitIO_read(bFile_t *bf, void *dest, int size, int nbit);
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
