/**
 * @file kpcan.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 协议接口文件
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

#ifndef __KPCAN_PORT_H
#define __KPCAN_PORT_H
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
/** @addtogroup Utilities
  * @{
  */


/** @addtogroup KPCAN
  * @{
  */

/**
 * @brief 获取所有块的字节长度和
 * n块数据传输时，每一个块数据的一个字，高20字节为n块数据的总字节数，低12位为当前块的index
 * 
 */
#define KP_BLOCKS_BSIZE(blockaddr)                    ((*(uint32_t*)blockaddr) >> 12 )


/**
 * @brief 获取当前块在所有块的序列
 * 
 */
#define KP_BLOCKS_INDEX(blockaddr)                    ((*(uint32_t*)blockaddr) & 0x00000FFF )


/**
 * @brief 获取当前块下接收数据的Byte长度，不包含末尾4字节（CRC 和块长度）,包含开始4字节（所有块总长度信息）
 * @param blocksize 创建条目时的大小
 */
#define KP_BLOCK_BSIZE(blockaddr, blocksize)          (*(uint16_t *)((uint32_t)blockaddr+blocksize))

/**
 * @brief 获取当前块下有效数据的Byte长度, 不包含末尾4字节（CRC 和块长度），也不包含开始4字节（所有块总长度信息）
 * @param blocksize 创建条目时的大小
 */
#define KP_BLOCK_VALID_BSIZE(blockaddr, blocksize)    (KP_BLOCK_BSIZE(blockaddr, blocksize) - 4)




/**
 * @brief 状态
 * 
 */
typedef enum
{
  KP_ERR = 0,
  KP_OK  = 1,
  KP_BT_ING = 2,   //块传输ing
  KP_BT_Fail = 3,  //块传输失败
  KP_BT_Succ = 4,  //块传输成功
}KP_State_TypeDef;

/**
 * @brief 设备类型
 * 
 */
typedef enum
{
  KP_SLAVE = 1,     //关节节点
  KP_MASTER,          //主节点
}KP_Dev_TypeDef;


/**
 * @brief 条目数据类型
 * 
 */
typedef enum
{
  KP_DATA_TYPE_INT = 1,     //4字节int
  KP_DATA_TYPE_FLOAT,       //4字节float  
  KP_DATA_TYPE_FUC,         //4字节函数指针
  KP_DATA_TYPE_BLOCK,      //用于大数据的存储空间
}KP_Data_TypeDef;

/**
 * @brief can数据帧格式
 * 
 */
typedef struct
{
  uint16_t can_id;
  uint8_t dlc;
  uint8_t data[8];
}KP_CANFrame_TypeDef, *KP_canframe;

/**
 * @brief 回调函数
 * 
 */
typedef void(*callback)(void); 

/**
 * @brief 内存分配/释放函数
 * 
 */
typedef void*(*kp_mallocDef)(uint32_t size);

typedef void(*kp_freeDef)(void* addr); 

typedef void (*kp_delayDef)(uint32_t ms);

typedef void (*kp_sendFrameDef)(KP_canframe msg);


typedef void * item_t;






/**
 * @brief 控制句柄
 * 
 */
typedef struct
{
  char            name[30];
  uint16_t        dev_id;             //对应CAN id
  KP_Dev_TypeDef  dev_type;
  void *          MItemHead;          //主id链表 p_KPMitem
  kp_mallocDef    kp_malloc;
  kp_freeDef      kp_free;
  kp_delayDef     kp_delay;
  kp_sendFrameDef kp_send;
  void *          ACKItemHead;
  uint32_t        heartbeat;
}KP_HandleTypeDef, *KP_handle;



/** @defgroup KPCAN Exported Functions
  * @{
  */

KP_handle        KP_Init(uint16_t dev_id, const char *name, KP_Dev_TypeDef dev_type, kp_mallocDef malloc, kp_freeDef free, kp_delayDef delay, kp_sendFrameDef send);

KP_State_TypeDef KP_DeInit(KP_handle kph);

item_t           KP_CreateItem(KP_handle kph, uint8_t m_id, uint8_t s_id, KP_Data_TypeDef data_type, uint32_t buff, uint32_t buff_size);

KP_State_TypeDef KP_Despatch(KP_handle kph, KP_canframe rcv, KP_canframe reply);

KP_State_TypeDef KP_GetItem(KP_handle kph, item_t item, void *dest, uint16_t size);

KP_State_TypeDef KP_PutItem(KP_handle kph, item_t item, void *src, uint16_t size, KP_canframe reply);

void*            KP_GetItemAddr(KP_handle kph, item_t item, uint16_t *size);

int              KP_GetItemACK(KP_handle kph, item_t item);

KP_State_TypeDef KP_GetItemFrame(KP_handle kph, item_t item, KP_canframe reply);

KP_State_TypeDef KP_WriteItem(KP_handle kph, item_t item, uint32_t timeout, int asheartbit);

KP_State_TypeDef KP_ReadItem(KP_handle kph, item_t item, void *dest, uint16_t timeout);

uint32_t         KP_ISHeartBit(KP_handle kph, uint32_t timeout);

void             KP_SetHeartbeat(KP_handle kph, uint32_t count);

uint32_t         KP_GetHeartbeat(KP_handle kph);
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

