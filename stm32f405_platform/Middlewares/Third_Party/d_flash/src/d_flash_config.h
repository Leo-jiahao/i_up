/**
 * @file d_flash_config.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 配置文件，传入操作flash的接口，和参数配置
 * @version 0.1
 * @date 2023-06-17
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

#ifndef __D_FLASH_CONFIG_H__
#define __D_FLASH_CONFIG_H__
#include "stm32f405_demo1_flash.h"
#define ENABLE_D_FLASH                   1      //使能d flash

/* 获取第一块扇区的首地址*/
#define FLASH_START_ADDR_1B             BSP_GetFlashAddr(ADDR_FLASH_SECTOR_10)

/* 获取第二块扇区的首地址*/		        
#define FLASH_START_ADDR_2B		        BSP_GetFlashAddr(ADDR_FLASH_SECTOR_11)

/* 获取第一块扇区的大小*/
#define FLASH_SIZE_1B                   (128 * 1024)//128K

/* 获取第二块扇区的大小*/
#define FLASH_SIZE_2B                   (128 * 1024)//128K

/* 第一块扇区的擦除接口，返回值为ture or false*/
#define FLASH_ERASE_1B                  BSP_EraseSector(ADDR_FLASH_SECTOR_10)

/* 第二块扇区的擦除接口，返回值为ture or false*/
#define FLASH_ERASE_2B                  BSP_EraseSector(ADDR_FLASH_SECTOR_11)

/* 读数据接口，返回值为读的内容，32bits*/
#define FLASH_READ_U32(address)         (*((volatile uint32_t *)(address)))


/* 写数据接口，addr：目的地址，src：uint32 *源地址，len：字长，返回值为ture or false*/
#define FLASH_WRITE(addr,src,len)       BSP_WriteFlash(addr,src,len)

/* 最大写入的条目数，每条数据在表头固定占12字节*/
#define USER_ITEM_MAX_NUM               (100) 

/* 数据在1B 2B之间转储的空间限制条件，在每次init时检测，并执行*/
#define TRANSFER_THRESHOLD_PERCENT      (99)    //数据限制99%



#endif  
