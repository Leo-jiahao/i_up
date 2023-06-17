/**
 * @file bitio.c
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-18
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
#include "bitio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


/** @defgroup  bitio Private Defines 私有宏定义
  * @{
  */
#define BIT_IO_BUFFER 4096
/**
  * @}
  */

/** @defgroup  bitio  Public Functions 共有函数
  * @{
  */

int bitof(int n)
{
    return (int)(ceil( log(n) / log(2) ));
}

/**
 * @brief 初始化位图文件描述符
 * 
 * @param fileaddr 压缩文件存在内部flash的大小
 * @param file_size 压缩文件大小
 * @return bFile_t* 
 * NULL Err
 */
bFile_t *bitIO_open(void *fileaddr, int32_t file_size)
{
    bFile_t *bf = NULL;

    bf = (bFile_t *)malloc(sizeof(bFile_t));
    if(bf == NULL){
        return NULL;
    }
    bf->filesize = file_size;
    bf->bytepos = NULL;
    bf->bitpos = NULL;
    bf->file_addr = (uint32_t)fileaddr;
    bf->seek = 0;

    return bf;
}

/**
 * @brief 释放内存
 * 
 * @param bf 
 * @return bFile_t* 
 */
bFile_t *bitIO_close(bFile_t *bf)
{
    if(bf){
        free(bf);
        bf = NULL;
    }
    return bf;
}



/**
 * @brief 从bit文件中读取数据到目标位置
 * 
 * @param bf bit文件描述符
 * @param dest 目的地址
 * @param size 字节长度
 * @param nbit 位长度 number of bit
 * @return int 
 * @par -1 err
 * @par 0 ok
 */
int bitIO_read(bFile_t *bf, void *dest, int size, int nbit)
{
    int i = 0;
    int byte_pos = 0, bit_pos = 0;
    uint8_t mask;

    if(bf == NULL || dest == NULL || size <= NULL || nbit <= 0){
        return -1;
    }

    memset(dest, 0, size);
    if(bf->seek >= bf->filesize){
            return -1;
    }
    for ( i = 0; i < nbit && bf->seek < bf->filesize; i++)
    {
        /* code */
        mask = 1 << bf->bitpos;
        
        //bit数据整合到字节数据
        if(((READ_U8(bf->file_addr)) & mask) != 0){
            READ_U8((uint32_t)dest + byte_pos) |= (0X01 << bit_pos);
        }

        byte_pos = (bit_pos < 7) ? byte_pos : (byte_pos + 1);
        bit_pos = (bit_pos < 7) ? (bit_pos + 1) : 0;

        if(bf->bitpos >= 7){
          bf->bytepos += 1;
          bf->file_addr++;
          bf->seek++;
//          if(bf->seek>=bf->filesize){
//            break;
//          }
        }
        bf->bitpos = (bf->bitpos < 7) ? (bf->bitpos + 1) : 0;
    }
    
    return i;

}
