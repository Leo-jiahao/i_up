/**
 * @file usr_can.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-02
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
#ifndef __USR_CAN_H__
#define __USR_CAN_H__
#include "stm32f405_demo1_can.h"
#include "kpcan.h"

typedef enum
{
    r_boot_slave_get = 0,

    r_boot_slave_put,
	
    item_num,
}DEV_ITEMS_TypeDef;


typedef struct
{
    uint8_t m_id;
    uint8_t s_id;
    KP_Data_TypeDef type;
    //对于块传输成功后的第一个32bit，代表当前块号，块末尾有4字节:buff_size到buff_size+4，表示当前块有效字节长度（包含块号和数据）、和crc16校验数据
    uint16_t buff_size;
}ITEM_TypeDef;

int Dev_CAN_Init(uint8_t node_id);

void Dev_CAN_DeInit(void);

extern item_t items[item_num];


extern KP_handle kph;


#endif

