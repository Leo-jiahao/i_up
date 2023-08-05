/**
 * @file d_flash.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
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
#ifndef __D_FLASH_H__
#define __D_FLASH_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "d_flash_config.h"
/** @defgroup Utilities
  * @{
  */
/** @defgroup  d flash
  * @{
  */


#define ITEM_MAX_NUM                    (USER_ITEM_MAX_NUM)
#define FErr                            (-1)
#define FSuc                            (0)


/**
 * @brief 定义状态码
 * 
 */
typedef enum
{
    DF_F_OK         = 0x0000,
    G_NODE_ERR      = 0x0001,
    D_NODE_ERR      = 0x0002,
    H_NODE_ERR      = 0x0003,
    Write_ERR       = 0x0004,
    Storage_Full    = 0x0005,
    Read_Err        = 0x0006,
    Read_SizeErr    = 0x0007,
    DataKey_SizeErr = 0x0008,
    Transfer_Err    = 0x0009,
    DF_S_ERR        = 0x000A,
}DF_Code;



/**
 * @brief 
 * 
 * @param key 传入的key，字符串的长度，必须为4的整数倍，且不大于配置值：如"LH_MAXOF"
 * @param dest 
 * @param size 最大1024字节
 * @return DF_Code 
 */
DF_Code df_read(const char *key, void *dest, uint32_t size);

/**
 * @brief 
 * 
 * @param key 传入的key，字符串的长度，必须为4的整数倍，且不大于配置值：如"LH_MAXOF"
 * @param src 
 * @param size 最大1024字节
 * @param state 
 * @return DF_Code 
 */
DF_Code df_write(const char *key, void *src, uint32_t size, uint32_t *state);
/**
 * @brief 初始化
 * 
 * @param state 
 * @return DF_Code 
 */
DF_Code df_init(void *state);
/**
 * @brief 强制擦除
 * 
 * @return DF_Code 
 */
DF_Code df_erase(void);

#endif   /* __D_FLASH_H__ */

