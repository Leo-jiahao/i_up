/**
 * @file lz77.c
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
/* Includes ------------------------------------------------------------------*/
#include "lz77.h"
#include <stdlib.h>
#include <string.h>

/** @defgroup Utilities
  * @{
  */
/** @defgroup  lz77
  * @{
  */

/** @defgroup  lz77 Private TypesDefinitions 私有类型
  * @{
  */

/**
 * @brief 从压缩包读取后向偏移、长度，前向字符
 * 例如（4， 2， 'a'）
 * 
 */
typedef struct{
  int off;
  int len;
  char next;
}token;
/**
  * @}
  */
/** @defgroup  lz77 Private Defines 私有宏定义
  * @{
  */

#define MAX_BIT_BUFFER 16


#define N               3
/**
  * @}
  */

/** @defgroup lz77 Private Macros 私有宏指令
  * @{
  */

/**
  * @}
  */
 /** @defgroup lz77 Private Variables 私有变量
  * @{
  */
 /**
  * @}
  */

/** @defgroup  lz77 Private FunctionPrototypes 私有函数
  * @{
  */


/**
 * @brief 从压缩文件中获取一个位图
 * 
 * @param bf 
 * @param la_size 
 * @param sb_size 
 * @return token 
 */
static token readcode(bFile_t *bf, int la_size, int sb_size)
{
  token t;
  int ret = 0;
  ret += bitIO_read(bf, &t.off, sizeof(t.off), bitof(sb_size));
  ret += bitIO_read(bf, &t.len, sizeof(t.len), bitof(la_size));
  ret += bitIO_read(bf, &t.next, sizeof(t.next), 8);
  if (ret < ( bitof(sb_size) + bitof(la_size) + 8))
  {
    /* code */
    t.off = -1;

  }
  return t;
  
}


 /**
  * @}
  */
/** @defgroup  lz77  Public Functions 共有函数
  * @{
  */

/**
 * @brief 将bf中定位的压缩包进行解压，到out_addr指向的内部flash，
 * 
 * @param bf 压缩文件
 * @param out_addr 解压文件存放地址
 * @param out_size 解压文件存放最大空间，不能超
 * @return int 
 * @par -1 err
 * @par 0 ok
 */
int decode(bFile_t *bf, void *out_addr, int out_size)
{
  token t;
  int back = 0, off = 0;
  unsigned char *buffer;
  int SB_SIZE = 0;//历史缓冲区
  int LA_SIZE = 0;//滑动窗口区
  int WINDOW_SIZE = 0;
  int out_pos = 0;

  if(bf == NULL || 
    bf->bitpos != NULL || 
    bf->bytepos != NULL || 
    bf->file_addr == NULL || 
    bf->filesize == NULL ||
    bf->seek != NULL){
    
    return -1;
  
  }
  
  //read header
  bitIO_read(bf, &SB_SIZE, sizeof(SB_SIZE), MAX_BIT_BUFFER);
  bitIO_read(bf, &LA_SIZE, sizeof(LA_SIZE), MAX_BIT_BUFFER);

  WINDOW_SIZE = (SB_SIZE * N) + LA_SIZE;

  buffer = (unsigned char*)calloc(WINDOW_SIZE, sizeof(unsigned char));
    
  while (1)
  {
    /* code */
    t = readcode(bf, LA_SIZE, SB_SIZE);
    if(t.off == -1){
      break;
    }

    if(back + t.len > WINDOW_SIZE - 1){
      memcpy(buffer, &(buffer[back - SB_SIZE]), SB_SIZE);
      back = SB_SIZE;
    }

    while (t.len > 0)
    {
      off = back - t.off;
      buffer[back] = buffer[off];

      //将buffer[back]写入到MCU内部flash中
      while(bf->write(((uint32_t *)((uint32_t)out_addr + out_pos)), 1, &buffer[back])<0){
        
      }
      out_pos++;
        
      back++;
      t.len--;
      /* code */
    }
    buffer[back] = t.next;

    //将buffer[back]写入到MCU内部flash中
    
    while(bf->write(((uint32_t *)((uint32_t)out_addr + out_pos)), 1, &buffer[back])<0){
      
    }
    out_pos++;
    back++;

  }
  free(buffer);

  return out_pos;
}


 /**
  * @}
  */
/**
  * @}
  */


/**
  * @}
  */


