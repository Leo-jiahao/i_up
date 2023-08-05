/**
 * @file can_protocol.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 一种CAN协议
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

#ifndef __KPCAN_PROTOCOL_H
#define __KPCAN_PROTOCOL_H
/* Includes ------------------------------------------------------------------*/
#include "kpcan.h"

/** @addtogroup Utilities
  * @{
  */


/** @addtogroup KPCAN
  * @{
  */


#define MAIN_ID_MAX       (0xff)
#define SUB_ID_MAX        (0xff)

#define KP_ITEM_MAX_NUM	    	255

typedef struct __SITEM *SHeadTypeDef;

/**
 * @brief main id管理节点
 * 
 */
typedef struct __MITEM
{
  uint8_t           m_id;
  uint8_t           item_num;
  struct __MITEM    *next;
  KP_handle         head;
  SHeadTypeDef      SItemHead;    //子id链表
}KPMitem, *p_KPMitem;


/**
 * @brief 数据条目节点
 * 
 */
typedef struct __SITEM
{
  uint8_t           mutex;          //为1，清零，改写，置1。如果为0，退出
  uint8_t           ack_flag;       //用于不等待write read，主发送时置 0xFF,后续有任何回复OK，重新置为1，错误置为0.
  uint8_t           s_id;           //子id
  KP_Data_TypeDef   data_type;      //数据类型
  void*             buff;           //数据存储地址，由于指针是变量仍然可以用于存储一些4Byte变量
  uint16_t          buff_size;      //数据存储空间大小
  struct __SITEM    *next;
  p_KPMitem         head;
}KPSitem,*p_KPSitem;


#define ACK_STATE_WAIT              1
#define ACK_STATE_WAIT_SUCCESS      2
#define ACK_STATE_WAIT_BLOCK_F      3
#define ACK_STATE_WAIT_BLOCK_FS     4
#define ACK_STATE_WAIT_BLOCK_END    5
#define ACK_STATE_WAIT_BLOCK_ENDS ACK_STATE_WAIT_SUCCESS
#define ACK_STATE_READ_WAIT         6
#define ACK_STATE_READ_SUCCESS      7
/**
 * @brief 定义等待的响应节点
 * 
 */
typedef struct __AITEM
{
  uint8_t ACK_STATE;
  p_KPSitem item;
  struct __AITEM *next;
}KPAitem, *p_KPAckitem;

/** @defgroup KPCAN Exported Functions
  * @{
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

