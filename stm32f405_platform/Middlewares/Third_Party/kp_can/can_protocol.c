/**
 * @file can_protocol.c
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
/* Includes ------------------------------------------------------------------*/
#include "can_protocol.h"
#include "utilities.h"
#include <string.h>
/** @defgroup Utilities
  * @{
  */
/** @defgroup  KPCAN
  * @{
  */


/** @defgroup  KPCAN Private TypesDefinitions 私有类型
  * @{
  */
/**
  * @}
  */
/** @defgroup KPCAN Private Defines 私有宏定义
  * @{
  */
/* CAN帧类型 */

#define FRAME_TYPE_SINGLE_WRITE_REQUEST 	    0x00    //主设备写
#define FRAME_TYPE_SINGLE_READ_REQUEST        0x01    //主设备读
#define FRAME_TYPE_SINGLE_ACTION_REQUEST 	    0x02    //主设备失能从设备回调
#define FRAME_TYPE_SLAVE_HEART_BIT            0x03    //从设备心跳帧类型
#define FRAME_TYPE_BLOCK_WRITE      	      	0x08    //主设备块写

#define FRAME_TYPE_ACK									      0x0B    //从设备应答帧类型



#define CAN_MULT_FRAME_BUFFER_LENTH           32

/**
 * @brief 块传输状态
 * 
 */
#define BLOCK_TRANSMIT_ING                    0xff
#define BLOCK_TRANSMIT_FAIL                   0x55
#define BLOCK_TRANSMIT_SUCC                   0xAA

/*一个CAN帧中的8个字节的含义*/
typedef enum
{
  BYTE_FRAME_TYPE = 0,
  BYTE_MAINSET_INDEX,
  BYTE_SUBSET_INDEX,
  BYTE_MULTI_FRAME_FLAG,
  BYTE_DATA_0,
  BYTE_DATA_1,
  BYTE_DATA_2,
  BYTE_DATA_3,
  BYTE_NUM,
}KP_Byte_TypeDef;

/**
  * @}
  */

/** @defgroup KPCAN Private Macros 私有宏指令
  * @{
  */

/**
  * @}
  */
 /** @defgroup KPCAN Private Variables 私有变量
  * @{
  */
 /**
  * @}
  */

/** @defgroup  KPCAN Private FunctionPrototypes 私有函数
  * @{
  */

/**
 * @brief Get the midNode object 获取主id节点，如果节点不存在则返回空
 * 
 * @param MItemHead 主id首节点
 * @param m_id 主id
 * @return p_KPMitem 
 */
static p_KPMitem get_midNode(KP_handle kph, uint8_t m_id)
{
  p_KPMitem next;
  if(kph == NULL){
    return NULL;
  }
  next = (p_KPMitem)kph->MItemHead;
  if (next->m_id == m_id)
  {
    return next;
    /* code */
  }
  while (next)
  {
    /* code */
    if (next->m_id == m_id)
    {
      return next;
      /* code */
    }
    
    next = next->next;
  }
  return NULL;
}
/**
 * @brief 新节点插入进链表
 * 
 * @param head 
 * @param node 
 * @return KP_State_TypeDef 
 */
static KP_State_TypeDef insert_midNode(KP_handle kph, p_KPMitem node)
{
  p_KPMitem next, pre;
  if(kph == NULL){
    return KP_ERR;
  }
  next = ((p_KPMitem)kph->MItemHead)->next;
  pre = (p_KPMitem)kph->MItemHead;
  while (next)
  { 
    pre = next;
    next = next->next;
  }
  pre->next = node;
  node->next = NULL;
  node->head = kph;
  return KP_OK;
}
/**
 * @brief Get the sidNode object 获取子id节点，如果节点不存在则返回空
 * 
 * @param MItemHead 主id首节点
 * @param s_id 子id
 * @return p_KPSitem 
 */
static p_KPSitem get_sidNode(p_KPMitem MItemHead, uint8_t s_id)
{
  p_KPSitem next;
  if(MItemHead == NULL){
    return NULL;
  }
  next = MItemHead->SItemHead;
  if (next->s_id == s_id)
  {
    return next;
    /* code */
  }
  while (next)
  {
    /* code */
    if (next->s_id == s_id)
    {
      return next;
      /* code */
    }
    
    next = next->next;
  }
  return NULL;
}
/**
 * @brief 新节点插入链表
 * 
 * @param head 主id节点
 * @param node 
 * @return KP_State_TypeDef 
 */
static KP_State_TypeDef insert_sidNode(p_KPMitem head, p_KPSitem node)
{
  p_KPSitem next, pre;
  if(head == NULL){
    return KP_ERR;
  }
  next = head->SItemHead->next;
  pre = head->SItemHead;
  while (next)
  { 
    pre = next;
    next = next->next;
  }
  pre->next = node;
  node->next = NULL;
  node->head = head;
  return KP_OK;
}

/**
 * @brief 新节点插入链表
 * 
 * @param head 主id节点
 * @param node 
 * @return KP_State_TypeDef 
 */
static KP_State_TypeDef insert_AckNode(KP_handle kph, p_KPAckitem node)
{
  p_KPAckitem next, pre = NULL;
  if(kph == NULL){
    return KP_ERR;
  }
  next = (p_KPAckitem)kph->ACKItemHead;
  while (next)
  { 
    pre = next;
    next = next->next;
  }
  if(pre)
  {
    pre->next = node;
  }else{
    kph->ACKItemHead = node;
    
  }
  node->next = NULL;

  return KP_OK;
}
/**
 * @brief 查找结点，不删除
 * 
 * @param head 主id节点
 * @param item 条目指针
 * @return p_KPAckitem 查找到的结点
 */
static p_KPAckitem get_AckNode(KP_handle kph, p_KPSitem item)
{
  p_KPAckitem next;
  if(kph == NULL){
    return NULL;
  }
  next = (p_KPAckitem)(kph->ACKItemHead);
  while (next)
  { 
    //判断
    if(next->item == item){
      return next;
    }

    next = next->next;
  }
  return NULL;
}

/**
 * @brief 删除结点
 * 
 * @param head 主id节点
 * @param node 
 * @return KP_State_TypeDef 状态
 */
static KP_State_TypeDef del_AckNode(KP_handle kph, p_KPAckitem node)
{
  p_KPAckitem next, pre = NULL;
  if(kph == NULL){
    return KP_ERR;
  }
  next = (p_KPAckitem)(kph->ACKItemHead);
  while (next)
  { 
    //判断
    if(next == node){
      if(pre){
        pre->next = next->next;
        kph->kp_free(next);
      }else{
        kph->kp_free(next);
        kph->ACKItemHead = NULL;
      }
      break;
    }
    pre = next;
    next = next->next;
  }
  return KP_OK;
}
/**
 * @brief 获取互斥量
 * 
 * @param s_item 
 * @return KP_State_TypeDef 
 * @par KP_OK 可以进行改写
 * @par KP_ERR 不能进行改写
 */
static KP_State_TypeDef getSItemMutex(p_KPSitem s_item)
{ 
  if (s_item->mutex == 1)
  {
    s_item->mutex = 0;
    return KP_OK;
    /* code */
  }else return KP_ERR;
  
}
/**
 * @brief 强制获取互斥量
 * 
 * @param s_item 
 * @return KP_State_TypeDef 
 * @par KP_OK 可以进行改写
 * @par KP_ERR 不能进行改写
 */
static KP_State_TypeDef getSItemMutexForce(p_KPSitem s_item)
{ 

  s_item->mutex = 0;
  return KP_OK;
  /* code */
 
}
/**
 * @brief 退还互斥量
 * 
 * @param s_item 
 * @return KP_State_TypeDef 
 * @par KP_OK 退还成功
 * @par KP_ERR 退还失败
 */
static KP_State_TypeDef giveSItemMutex(p_KPSitem s_item)
{ 
  if (s_item->mutex == 0)
  {
    s_item->mutex = 1;
    return KP_OK;
    /* code */
  }else return KP_ERR;
  
}

 /**
  * @}
  */






/** @defgroup  KPCAN Public Functions 共有函数
  * @{
  */

/**
 * @brief 通信节点初始化
 * 
 * @param dev_id 节点id，和CAN id一致
 * @param name 节点名字符串
 * @param dev_type 节点类型
 * @param malloc 内存分配函数接口
 * @param free  内存释放函数接口
 * @param delay ms延时函数接口
 * @param send CAN帧发送接口
 * @return KP_handle 
 */
KP_handle KP_Init(uint16_t dev_id, const char *name, KP_Dev_TypeDef dev_type, kp_mallocDef malloc, kp_freeDef free, kp_delayDef delay, kp_sendFrameDef send)
{
  if(malloc == NULL || free == NULL){
    return NULL;
  }
  KP_handle kph = NULL;
  kph = (KP_handle)malloc(sizeof(KP_HandleTypeDef));
  if(kph == NULL){
    return NULL;
  }
  memset(kph, 0, sizeof(KP_HandleTypeDef));
  kph->dev_id = dev_id;
  kph->dev_type = dev_type;
  kph->heartbeat = 0;
  if(name != NULL)
  strncpy(kph->name, name, 30);
  kph->kp_malloc = malloc;
  kph->kp_free = free;
  kph->ACKItemHead = NULL;
  kph->kp_delay = delay;
  kph->kp_send = send;

  kph->MItemHead = malloc(sizeof(KPMitem));
  if(((p_KPMitem)kph->MItemHead) == NULL){
    free(kph);
    return NULL;
  }
  memset(((p_KPMitem)kph->MItemHead), 0, sizeof(KPMitem));
  ((p_KPMitem)(kph->MItemHead))->head = kph;

  ((p_KPMitem)kph->MItemHead)->SItemHead = (SHeadTypeDef)malloc(sizeof(KPSitem));
  if (((p_KPMitem)kph->MItemHead)->SItemHead == NULL)
  {
    free(((p_KPMitem)kph->MItemHead));
    free(kph);
    return NULL;
  }
  memset(((p_KPMitem)kph->MItemHead)->SItemHead, 0, sizeof(KPSitem));
  ((p_KPMitem)kph->MItemHead)->SItemHead->mutex = 1;
  ((p_KPMitem)kph->MItemHead)->SItemHead->head = (p_KPMitem)kph->MItemHead;
  ((p_KPMitem)kph->MItemHead)->SItemHead->s_id = 0;
  return kph;
}
/**
 * @brief 清除
 * 
 * @param kph 
 * @return KP_State_TypeDef 
 */
KP_State_TypeDef KP_DeInit(KP_handle kph)
{
  kp_freeDef free = kph->kp_free;
  p_KPMitem m_next = NULL, m_now = NULL;
  p_KPSitem s_next = NULL, s_now = NULL;
  if(kph == NULL){
    return KP_ERR;
  }
  m_now = ((p_KPMitem)kph->MItemHead);
  m_next = m_now->next;
  while (m_now)
  {
    m_next = m_now->next;

    s_now = m_now->SItemHead;
    s_next = s_now->next;
    while (s_now)
    {
      s_next = s_now->next;
      kph->kp_free(s_now);
      s_now = s_next;
      /* code */
    }
    kph->kp_free(m_now);
    m_now = m_next;
    /* code */
  }
  memset(kph, 0, sizeof(KP_HandleTypeDef));
  free(kph);
  return KP_OK;
}

/**
 * @brief 创建具体条目,如果已存在条目，则改为设置的参数
 * 
 * @param kph 控制句柄
 * @param m_id 主 id <=0xff
 * @param s_id 子 id <=0xff
 * @param data_type 数据类型
 * @par   KP_DATA_TYPE_INT = 1,     //4字节int
 * @par   KP_DATA_TYPE_FLOAT,       //4字节float  
 * @par   KP_DATA_TYPE_FUC,         //4字节函数指针
 * @par   KP_DATA_TYPE_BLOCK,      //用于大数据的存储空间,此时需要用到 param1:buffer size
 * @param buff 函数指针类型时有用，其他情况无效
 * @param buff_size 数据类型为KP_DATA_TYPE_BLOCK时  对参数对应block空间的大小。其他数据类型时，此参数无效
 * @return item_t 条目地址
 */
item_t KP_CreateItem(KP_handle kph, uint8_t m_id, uint8_t s_id, KP_Data_TypeDef data_type, uint32_t buff, uint32_t buff_size)
{
  p_KPSitem s_item = NULL;
  p_KPMitem m_item = NULL;
  if(kph == NULL || kph->kp_malloc == NULL || kph->kp_free == NULL || m_id > MAIN_ID_MAX || s_id >SUB_ID_MAX){
    return NULL;
  }
  //1 查看是否已经创建 若已经创建，则更新条目信息，不额外创建
  m_item  = get_midNode(kph, m_id);
  if(m_item == NULL){
    //未找到对应的主节点
    m_item = (p_KPMitem)kph->kp_malloc(sizeof(KPMitem));
    if(m_item == NULL){
      return NULL;
    }
    memset(m_item, 0, sizeof(KPMitem));
    m_item->m_id = m_id;
    m_item->item_num = 0;
    //挂载到主节点链表
    if(insert_midNode(kph, m_item) == KP_ERR){
      return NULL;
    }
    m_item->SItemHead = (SHeadTypeDef)kph->kp_malloc(sizeof(KPSitem));
    if(m_item->SItemHead == NULL){
      kph->kp_free(m_item);
    }
    memset(m_item->SItemHead, 0, sizeof(KPSitem));
    m_item->SItemHead->mutex = 1;
    m_item->SItemHead->head = m_item;
  }
  //2 主节点链表head下查找对应子节点
  s_item = get_sidNode(m_item, s_id);
  if(s_item == NULL){
    //2.1 需要新建,挂载对应位置
    s_item = (p_KPSitem)kph->kp_malloc(sizeof(KPSitem));
    if(s_item == NULL){
      return NULL;
    }
    s_item->buff = NULL;
    s_item->mutex = 1;
    if(insert_sidNode(m_item, s_item) == KP_ERR){
      return NULL;
    }
    
  }else{
    kph->kp_free(s_item->buff);
  }
  //配置参数，
  if(getSItemMutex(s_item) != KP_OK){
    return NULL;
  }
  s_item->s_id = s_id;
  s_item->mutex = 1;
  s_item->data_type = data_type;
  s_item->head = m_item;
  s_item->ack_flag = 0;
  if(data_type == KP_DATA_TYPE_BLOCK){
    s_item->buff_size = buff_size + 4;//2Byte len  2Byte CRC
    
  }else{
    s_item->buff_size = 4;//float int 函数指针
  }
  
  

  s_item->buff = kph->kp_malloc(s_item->buff_size);
  if(s_item->buff == NULL){
    giveSItemMutex(s_item);
    return NULL;
  }
  memset(s_item->buff, 0, s_item->buff_size);
  
  if(data_type == KP_DATA_TYPE_FUC){
    //传函数指针

    memcpy( s_item->buff, &buff, 4);
  
  }
  giveSItemMutex(s_item);
  return s_item;
}

/**
 * @brief 解析CAN数据帧
 * 
 * @param kph 处理通信节点描述句柄
 * @param rcv 接收的can数据帧
 * @param reply 需要回复的can数据帧，外部提前开辟空间
 * @return KP_State_TypeDef 是否需要回复
 * @par KP_ERR 不需要回复
 * @par KP_OK  需要回复
 * @par KP_BT_ING  快传输中，可以不回复
 * @par KP_BT_Fail 块传输失败必须回复
 * @par KP_BT_Succ 块传输成功必须回复
 */
KP_State_TypeDef KP_Despatch(KP_handle kph, KP_canframe rcv, KP_canframe reply)
{
  p_KPSitem s_item = NULL;
  p_KPMitem m_item = NULL;
  p_KPAckitem ack_item = NULL;
  callback callback_fun = NULL;
  uint16_t crc;
  uint16_t block_index;
  if(rcv->can_id != kph->dev_id){
    return KP_ERR;
  }

  if(rcv->data[BYTE_FRAME_TYPE] == 0xff ||
	  rcv->data[BYTE_FRAME_TYPE] == FRAME_TYPE_SLAVE_HEART_BIT)  //心跳数据帧
  {
    kph->heartbeat++;
    if(kph->heartbeat == 0)
      kph->heartbeat = 1;
  }
  if(rcv->dlc != 8){
    return KP_ERR;
  }
  m_item = get_midNode(kph, rcv->data[BYTE_MAINSET_INDEX]);
  if(m_item == NULL){
    return KP_ERR;
  }
  s_item = get_sidNode(m_item, rcv->data[BYTE_SUBSET_INDEX]);
  if(s_item == NULL){
    return KP_ERR;
  }

  //准备应答数据帧
  reply->can_id                       = rcv->can_id;
  reply->dlc                          = BYTE_NUM;
  reply->data[BYTE_FRAME_TYPE]        = FRAME_TYPE_ACK;
  reply->data[BYTE_MAINSET_INDEX]     = m_item->m_id;
  reply->data[BYTE_SUBSET_INDEX]      = s_item->s_id;
  reply->data[BYTE_MULTI_FRAME_FLAG]  = 0;

  switch (rcv->data[BYTE_FRAME_TYPE])
  {

  case FRAME_TYPE_SINGLE_WRITE_REQUEST://单条目写请求
    /* code */
    getSItemMutexForce(s_item);
    memcpy(s_item->buff, &rcv->data[BYTE_DATA_0], BYTE_DATA_3 - BYTE_DATA_0 + 1);
    giveSItemMutex(s_item);
    //应答
    memcpy(&reply->data[BYTE_DATA_0],&rcv->data[BYTE_DATA_0], BYTE_DATA_3 - BYTE_DATA_0 + 1);
    reply->data[BYTE_MULTI_FRAME_FLAG] = reply->data[BYTE_DATA_0]\
                                        +reply->data[BYTE_DATA_1]\
                                        +reply->data[BYTE_DATA_2]\
                                        +reply->data[BYTE_DATA_3];
    return KP_OK;


  case FRAME_TYPE_SINGLE_READ_REQUEST://单条目读请求
    /* code */

    memcpy(&reply->data[BYTE_DATA_0],s_item->buff, BYTE_DATA_3 - BYTE_DATA_0 + 1);
    reply->data[BYTE_MULTI_FRAME_FLAG] = reply->data[BYTE_DATA_0]\
                                        +reply->data[BYTE_DATA_1]\
                                        +reply->data[BYTE_DATA_2]\
                                        +reply->data[BYTE_DATA_3];
    return KP_OK;


  case FRAME_TYPE_SINGLE_ACTION_REQUEST://单帧回调函数请求
    /* code */
    if(kph->dev_type==KP_SLAVE){
        memcpy(&callback_fun, s_item->buff, 4);
        callback_fun();
        reply->data[BYTE_MULTI_FRAME_FLAG] = reply->data[BYTE_DATA_0]\
                                        +reply->data[BYTE_DATA_1]\
                                        +reply->data[BYTE_DATA_2]\
                                        +reply->data[BYTE_DATA_3];
        return KP_OK;
    }
    return KP_ERR;
  case FRAME_TYPE_SLAVE_HEART_BIT:  //心跳数据帧
      getSItemMutexForce(s_item);
      memcpy(s_item->buff, &rcv->data[BYTE_DATA_0], BYTE_DATA_3 - BYTE_DATA_0 + 1);
      giveSItemMutex(s_item);
    return KP_ERR;

  case FRAME_TYPE_BLOCK_WRITE://块条目写入请求
    getSItemMutexForce(s_item);
    //设置块传输应答
    reply->data[BYTE_MULTI_FRAME_FLAG]  = 0xFF;
    
    //开始第一帧数据
    if(rcv->data[BYTE_MULTI_FRAME_FLAG] == 0){
      reply->data[BYTE_MULTI_FRAME_FLAG] = 0;
      memset(s_item->buff, 0, s_item->buff_size);
      
      memcpy((void *)((uint32_t)s_item->buff + rcv->data[BYTE_MULTI_FRAME_FLAG] * 4), &rcv->data[BYTE_DATA_0], BYTE_DATA_3 - BYTE_DATA_0 + 1);
      
      reply->data[BYTE_DATA_0] = BLOCK_TRANSMIT_ING;//传输中
      giveSItemMutex(s_item);
		
      return KP_BT_ING;

    }
    else if(rcv->data[BYTE_MULTI_FRAME_FLAG] == 0xff){
    //结束帧
      reply->data[BYTE_MULTI_FRAME_FLAG]  = 0xff;
      //校验 
      crc = crc16_2((uint8_t const *)s_item->buff, (*(uint16_t *)((uint32_t)&rcv->data[BYTE_DATA_0])));

      //CRC16 校验通过
	  block_index = KP_BLOCKS_INDEX(s_item->buff);
	  memcpy(&reply->data[BYTE_DATA_2], &block_index,2);
	  memcpy((void *)((uint32_t)s_item->buff + s_item->buff_size - 4), &rcv->data[BYTE_DATA_0], BYTE_DATA_3 - BYTE_DATA_0 + 1);
	  reply->data[BYTE_DATA_1] = 0;
      if(crc == (*(uint16_t *)((uint32_t)&rcv->data[BYTE_DATA_2])) ){
        reply->data[BYTE_DATA_0] = BLOCK_TRANSMIT_SUCC;//成功
        
        giveSItemMutex(s_item);

        return KP_BT_Succ;
      }else{
      //crc 校验不通过  
        reply->data[BYTE_DATA_0] = BLOCK_TRANSMIT_FAIL;//失败
        
        giveSItemMutex(s_item);
		  
        return KP_BT_Fail;

      }

    }else{

      memcpy((void *)((uint32_t)s_item->buff + rcv->data[BYTE_MULTI_FRAME_FLAG] * 4), &rcv->data[BYTE_DATA_0], BYTE_DATA_3 - BYTE_DATA_0 + 1);
      reply->data[BYTE_DATA_0] = BLOCK_TRANSMIT_ING;//传输中
      giveSItemMutex(s_item);
      return KP_BT_ING;

    }
  case FRAME_TYPE_ACK:
  //应答帧
    if(s_item->ack_flag==0xff){
        if(s_item->data_type != KP_DATA_TYPE_BLOCK)
        {
            s_item->ack_flag=1;
        }else if(rcv->data[BYTE_DATA_0] == BLOCK_TRANSMIT_SUCC
                   && KP_BLOCKS_INDEX(s_item->buff) == *(uint16_t*)&rcv->data[BYTE_DATA_2]){

            s_item->ack_flag = 1;

        }else{
            s_item->ack_flag = 0;
        }
        break;

    }
    ack_item = get_AckNode(kph, s_item);
    if(ack_item == NULL){
      return KP_ERR;
    }
    switch (ack_item->ACK_STATE)
    {
    case ACK_STATE_WAIT:
      if(s_item->data_type == KP_DATA_TYPE_BLOCK){
          if(KP_BLOCKS_INDEX(s_item->buff) == *(uint16_t*)&rcv->data[BYTE_DATA_2]){
              ack_item->ACK_STATE = ACK_STATE_WAIT_SUCCESS;
          }
      }else{
        ack_item->ACK_STATE = ACK_STATE_WAIT_SUCCESS;
      }
      break;
    case ACK_STATE_WAIT_BLOCK_F:
      ack_item->ACK_STATE = ACK_STATE_WAIT_BLOCK_FS;
      break;
    case ACK_STATE_WAIT_BLOCK_END:
      ack_item->ACK_STATE = ACK_STATE_WAIT_BLOCK_ENDS;
      break;
    case ACK_STATE_READ_WAIT:
      //将数据更新到数据区
      ack_item->ACK_STATE = ACK_STATE_READ_SUCCESS;
      memcpy(s_item->buff, &rcv->data[BYTE_DATA_0], 4);
      break;
    default:
      break;
    }
    return KP_ERR;

  default:

    break;
  }

  return KP_OK;
}
/**
 * @brief 获取对应条目的数据，首先检查对应空间大小是否符合要求，然后内存拷贝或者传出内存空间的指针，严格遵守条目的数据类型
 * 
 * @param kph 处理通信节点描述句柄
 * @param item_t item
 * @param dest 目的地址
 * @param size 目的地址空间大小，务必等于条目创建时设定的大小
 * @return KP_State_TypeDef 
 */
KP_State_TypeDef KP_GetItem(KP_handle kph, item_t item, void *dest, uint16_t size)
{
   p_KPSitem s_item = (p_KPSitem)item;

  p_KPMitem m_item = NULL;
  if (s_item->head->m_id > MAIN_ID_MAX || s_item->s_id >SUB_ID_MAX)
  {
    return KP_ERR;
    /* code */
  }
  
  m_item = get_midNode(kph, s_item->head->m_id);
  if(m_item == NULL){
    return KP_ERR;
  }
  s_item = get_sidNode(m_item, s_item->s_id);
  if(s_item == NULL){
    return KP_ERR;
  }
  if(s_item->buff_size != size){
    return KP_ERR;
  }
  memcpy(dest, s_item->buff, s_item->buff_size);
  return KP_OK;
}

/**
 * @brief 获取对应条目数据所在的地址,通常用于块条目
 * 
 * @param kph 控制句柄
 * @param item_t item，条目指针
 * @param s_id 子id
 * @param size 用于获取条目中数据的大小创建属性时的大小
 * @return void* 遇到错误返回 NULL，正常情况下返回对应条目数据所在的地址
 */
void * KP_GetItemAddr(KP_handle kph, item_t item, uint16_t *size)
{
  p_KPSitem s_item = (p_KPSitem)item;

  if(s_item == NULL){
    return NULL;
  }
  if(s_item->data_type == KP_DATA_TYPE_BLOCK)
  {
    *size = s_item->buff_size - 4;
  }else{
    *size = s_item->buff_size;
  }

  return s_item->buff;
}
/**
 * @brief KP_GetItemACK,对于KP_WriteItem，KP_ReadItem操作的同一条目
 *          处于多线程重入的情况下，此函数的值不安全，不可靠。
 * @param kph
 * @param item
 * @return
 * 0xff waiting
 * 0   block waiting err
 * 1   ok
 */
int KP_GetItemACK(KP_handle kph, item_t item)
{
  p_KPSitem s_item = (p_KPSitem)item;

  if(s_item == NULL){
    return 0;
  }
  return s_item->ack_flag;
}
/**
 * @brief 更新条目的数据,获取数据帧（帧类型为0xFF,无意义，需要用户指定）,
 * 如果时块条目，该条目的首4字节，高20bit 整个文件大小，后12bit 当前块在文件所有块中的index（从0开始）
 * 
 * @param kph 控制句柄
 * @param item 条目创建时返回的id
 * @param src 数据源地址
 * @param size 数据大小
 * @param reply 新数据整合为一帧，一般为null
 * @return KP_State_TypeDef 
 */
KP_State_TypeDef KP_PutItem(KP_handle kph, item_t item, void *src, uint16_t size, KP_canframe reply)
{
  p_KPSitem s_item = (p_KPSitem)item;
  if(item == NULL){//只做非零检查，有风险
    return KP_ERR;
  }
  if(s_item->buff_size != size && s_item->data_type != KP_DATA_TYPE_BLOCK){
    return KP_ERR;
  }
  if(s_item->buff_size < size + 4 && s_item->data_type == KP_DATA_TYPE_BLOCK){
    return KP_ERR;
  }
  if(getSItemMutex(s_item) != KP_OK){
    return KP_ERR;
  }
  memcpy(s_item->buff, src, size);
  if(s_item->data_type == KP_DATA_TYPE_BLOCK){
      //整理尾帧，计算crc

      memcpy((void*)((uint32_t)s_item->buff + s_item->buff_size - 4), (void*)&size, 2);


      uint16_t crc = crc16_2((uint8_t const *)src, size);

      memcpy((void*)((uint32_t)s_item->buff + s_item->buff_size - 2), (void*)&crc, 2);

  }
  giveSItemMutex(s_item);
    if(reply != NULL){
    reply->can_id = kph->dev_id;
    reply->dlc = 8;
    reply->data[BYTE_FRAME_TYPE] = 0xff;
    reply->data[BYTE_MAINSET_INDEX] = s_item->head->m_id;
    reply->data[BYTE_SUBSET_INDEX] = s_item->s_id;
    reply->data[BYTE_MULTI_FRAME_FLAG] = 0;
    memcpy(&reply->data[BYTE_DATA_0], s_item->buff, size);
  }
  return KP_OK;
}
/**
 * @brief 获取数据长度为4字节的数据帧，帧类型，为 0xff（无意义）
 * 
 * @param kph 
 * @param item 
 * @param reply 
 * @return KP_State_TypeDef 
 */
KP_State_TypeDef KP_GetItemFrame(KP_handle kph, item_t item, KP_canframe reply)
{
  p_KPSitem s_item = (p_KPSitem)item;
  if(reply != NULL && s_item->buff_size == 4){
    reply->can_id = kph->dev_id;
    reply->dlc = 8;
    reply->data[BYTE_FRAME_TYPE] = 0xff;
    reply->data[BYTE_MAINSET_INDEX] = s_item->head->m_id;
    reply->data[BYTE_SUBSET_INDEX] = s_item->s_id;
    memcpy(&reply->data[BYTE_DATA_0], s_item->buff, 4);
    reply->data[BYTE_MULTI_FRAME_FLAG] = reply->data[BYTE_DATA_0]\
                                        +reply->data[BYTE_DATA_1]\
                                        +reply->data[BYTE_DATA_2]\
                                        +reply->data[BYTE_DATA_3];//和校验
    
	return KP_OK;
  }
  return KP_ERR;
}
/**
 * @brief 向其他设备写条目中的信息，单帧,很快即可得到回复。对于块，需要很长时间得到回复，如果timeout=0不保证操作正确.
 *        尽量避免同一条目的重入，尤其是块条目。
 *        如果写回调条目，意味着使从设备执行对应回调函数，从节点在回调完成后进行应答
 * 
 * @param kph 
 * @param item 
 * @param timeout 等待设备回复的时间，如果为0，则代表不等待结果，直接返回正确
 *               （如果要获取真实状态，可以通过KP_GetItemACK获取）。
 * @param asheartbit 是否作为心跳类型发送，只对4字节数据条目有效。
 * @return KP_State_TypeDef 
 */
KP_State_TypeDef KP_WriteItem(KP_handle kph, item_t item, uint32_t timeout, int asheartbit)
{
  KP_CANFrame_TypeDef frame;
  uint16_t framecount = 0;
  p_KPSitem s_item = (p_KPSitem)item;
  p_KPAckitem ack_item = NULL;
  if(item == NULL){
    return KP_ERR;
  }
  if(asheartbit==0){
    //创建一个
	  ack_item = (p_KPAckitem)kph->kp_malloc(sizeof(KPAitem));
	  if(ack_item == NULL){
		return KP_ERR;
	  }
	  ack_item->item = (p_KPSitem)item;
	  ack_item->next = NULL;
	  ack_item->ACK_STATE = ACK_STATE_WAIT;
	  //将节点插入链表
	  if(insert_AckNode(kph, ack_item) == KP_ERR){
		kph->kp_free(ack_item);
		return KP_ERR;
	  }
  }
  if((s_item->data_type == KP_DATA_TYPE_BLOCK || s_item->data_type == KP_DATA_TYPE_FUC)
          && asheartbit){
      return KP_ERR;
  }



  switch (s_item->data_type)
  {
      //单帧
  case KP_DATA_TYPE_INT:
  case KP_DATA_TYPE_FLOAT:
      if(KP_GetItemFrame(kph, item, &frame) != KP_OK){
        del_AckNode(kph, ack_item);
        return KP_ERR;
      }
      if(asheartbit){
          frame.data[BYTE_FRAME_TYPE] = FRAME_TYPE_SLAVE_HEART_BIT;
          kph->kp_send(&frame);
          return KP_OK;
      }else{
          frame.data[BYTE_FRAME_TYPE] = FRAME_TYPE_SINGLE_WRITE_REQUEST;
          kph->kp_send(&frame);
      }

      break;
  case KP_DATA_TYPE_FUC:

    if(KP_GetItemFrame(kph, item, &frame) != KP_OK){
      del_AckNode(kph, ack_item);
      return KP_ERR;
    }
    frame.data[BYTE_FRAME_TYPE] = FRAME_TYPE_SINGLE_ACTION_REQUEST;
    kph->kp_send(&frame);

    break;

  //块 
  case KP_DATA_TYPE_BLOCK:
  frame.can_id = kph->dev_id;
  frame.dlc = 8;
  frame.data[BYTE_FRAME_TYPE] = FRAME_TYPE_BLOCK_WRITE;
  frame.data[BYTE_MAINSET_INDEX] = s_item->head->m_id;
  frame.data[BYTE_SUBSET_INDEX] = s_item->s_id;


  framecount = KP_BLOCK_BSIZE(s_item->buff, s_item->buff_size-4);

  if(framecount % 4 > 0){
      framecount = framecount >> 2;
      framecount++;
  }else{
      framecount = framecount >> 2;
  }


  for (uint16_t i = 0; i < framecount; i++)
  {
    frame.data[BYTE_MULTI_FRAME_FLAG] = i;
    //中间帧
    memcpy(&frame.data[BYTE_DATA_0], (void *)((uint32_t)s_item->buff + i * 4) , 4);
    kph->kp_send(&frame);
  }
  //结束帧
  frame.data[BYTE_MULTI_FRAME_FLAG] = 0XFF;
  memcpy(&frame.data[BYTE_DATA_0], (void *)((uint32_t)s_item->buff + s_item->buff_size - 4) , 4);
  kph->kp_send(&frame);
  ack_item->ACK_STATE = ACK_STATE_WAIT;
  break;
  default:
    break;
  }


  //发送操作结束，等待
  if(timeout == 0){
    del_AckNode(kph, ack_item);
    s_item->ack_flag = 0xff;
    return KP_OK;
  }

  kph->kp_delay(timeout);

  if(ack_item->ACK_STATE == ACK_STATE_WAIT ){
    del_AckNode(kph, ack_item);
    return KP_ERR;
  }else{
    del_AckNode(kph, ack_item);
    return KP_OK;
  }

}
/**
 * @brief 从其他设备读条目中的信息，只支持单帧，需要等待从设备回复,且不允许读回调条目
 * 
 * @param kph 为其他设备注册的通信描述结构体
 * @param item 当前设备注册的条目，（目标设备应该有一个相同m_id s_id的条目），完全支持，float int类型，对于回调类型不需要从设备ack，不支持读块条目
 * @param dest 数据的存放目的地
 * @param timeout 等待设备回复的时间，如果为0，则代表不等待结果，直接返回正确,但是会返回内部buffer中的数据
 *               （如果要获取真实状态，可以通过KP_GetItemACK获取）。
 * @return KP_State_TypeDef 
 */
KP_State_TypeDef KP_ReadItem(KP_handle kph, item_t item, void *dest, uint16_t timeout)
{
  KP_CANFrame_TypeDef frame;
  p_KPSitem s_item = (p_KPSitem)item;

  p_KPAckitem ack_item = NULL;
  if(item == NULL){
    return KP_ERR;
  }
  frame.can_id = kph->dev_id;
  frame.dlc = 8;
  frame.data[BYTE_MAINSET_INDEX] = s_item->head->m_id;
  frame.data[BYTE_SUBSET_INDEX] = s_item->s_id;
  frame.data[BYTE_MULTI_FRAME_FLAG] = 0;
  switch (s_item->data_type)
  {
  //单帧
  case KP_DATA_TYPE_INT:
  case KP_DATA_TYPE_FLOAT:
    frame.data[BYTE_FRAME_TYPE] = FRAME_TYPE_SINGLE_READ_REQUEST;
    frame.data[BYTE_MULTI_FRAME_FLAG] = frame.data[BYTE_DATA_0]\
                                        +frame.data[BYTE_DATA_1]\
                                        +frame.data[BYTE_DATA_2]\
                                        +frame.data[BYTE_DATA_3];//和校验
    break;
  case KP_DATA_TYPE_FUC:
    return KP_ERR;

  default:
    return KP_ERR;
  }

  //发送
  kph->kp_send(&frame);
  
  if(timeout == 0){
    s_item->ack_flag = 0xff;
    if(dest != NULL){
        memcpy(dest, s_item->buff, s_item->buff_size);
    }
    return KP_OK;
  }

  //创建一个回复结点
  ack_item = (p_KPAckitem)kph->kp_malloc(sizeof(KPAitem));
  if(ack_item == NULL){
    return KP_ERR;
  }
  ack_item->item = (p_KPSitem)item;
  ack_item->next = NULL;
  ack_item->ACK_STATE = ACK_STATE_READ_WAIT;

  //将节点插入链表
  if(insert_AckNode(kph, ack_item) == KP_ERR){
    kph->kp_free(ack_item);
    return KP_ERR;
  }

  //等待回复
  kph->kp_delay(timeout);

  if(ack_item->ACK_STATE == ACK_STATE_READ_SUCCESS){
    if(dest != NULL){
        memcpy(dest, s_item->buff, s_item->buff_size);
    }
    del_AckNode(kph, ack_item);
    return KP_OK;
  }else{
    del_AckNode(kph, ack_item);
    return KP_ERR;
  }
}

/**
 * @brief 阻塞获取心跳状态
 * 
 * @param kph 设备描述句柄
 * @param timeout 阻塞时间
 * @return KP_State_TypeDef 
 * 0 不在线
 * 其他 在线
 */
uint32_t KP_ISHeartBit(KP_handle kph, uint32_t timeout)
{
  if(kph == NULL){
    return KP_ERR;
  }
  kph->heartbeat=0;
  kph->kp_delay(timeout);
  return kph->heartbeat;
}
/**
 * @brief 设置心跳计数值
 * 
 * @param kph 设备描述句柄
 * @param count 计数值
 */
void KP_SetHeartbeat(KP_handle kph, uint32_t count)
{
  if(kph){
    kph->heartbeat=count;
  }
  
}
/**
 * @brief 获取心跳计数值
 * 
 * @param kph 设备描述句柄
 * @return uint32_t 心跳帧计数值
 */
uint32_t KP_GetHeartbeat(KP_handle kph)
{
  if(kph){
    return kph->heartbeat;
  }else{
    return 0;
  }
  
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




