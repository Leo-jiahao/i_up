/**
 * @file usr_can.c
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
#include "usr_can.h"
#include <stdlib.h>
#include "d_flash.h"
#include "r_boot_slave.h"



const ITEM_TypeDef items_attri[item_num] = {
    [r_boot_slave_get]              = { .m_id = 1, .s_id = 0 , .type = KP_DATA_TYPE_BLOCK     , .buff_size = 2048 },
    
    [r_boot_slave_put]             = { .m_id = 2, .s_id = 1 , .type = KP_DATA_TYPE_BLOCK   , .buff_size = 2048 },

};


item_t items[item_num] = {NULL, };

KP_handle kph;
/**
 * @brief 发送接口
 * 
 * @param msg 
 */
static void sendCANFrame(KP_canframe msg)
{

    BSP_CAN1_Transmit(msg->can_id, msg->data, msg->dlc);
}
/**
 * @brief 数据接收处理
 * 
 * @param msg 
 */
static void CAN1_Receive(CANRxMSG_TypeDef *msg)
{

    KP_State_TypeDef kp_state = KP_ERR;

    void *blockaddr = NULL;
    uint16_t blocksize;
	KP_CANFrame_TypeDef kp_msg,reply;
	kp_msg.can_id = msg->cob_id;
	kp_msg.dlc = msg->length;
	memcpy(kp_msg.data, msg->data, kp_msg.dlc);
    kp_state = KP_Despatch(kph, &kp_msg, &reply);

    if( kp_state == KP_BT_Succ &&
         items_attri[r_boot_slave_get].m_id == kp_msg.data[1] &&
         items_attri[r_boot_slave_get].s_id == kp_msg.data[2]){

        //获取块数据地址 获取块注册时的大小
        blockaddr =  KP_GetItemAddr(kph, items[r_boot_slave_get], &blocksize); 

        r_boots_input((const void *)( ( uint32_t ) blockaddr + 4),\
         KP_BLOCK_VALID_BSIZE(blockaddr, blocksize));
		
        
    }


    if(kp_state == KP_OK  || kp_state == KP_BT_Fail || kp_state == KP_BT_Succ ){
    //回复reply
        
        BSP_CAN1_Transmit(reply.can_id, reply.data, reply.dlc);
    }
}
/**
 * @brief CAN初始化
 * 
 * @param node_id 
 * @return int 
 */
int Dev_CAN_Init(uint8_t node_id)
{
    int i = 0;
    bool ret;

    ret = BSP_CAN1_Init(CAN_1M, node_id);
    if(ret == true){
        return 0;
    }

    BSP_CAN1_SetRxCallBack(CAN1_Receive);

    kph = KP_Init(node_id, "node", KP_SLAVE, malloc, free, HAL_Delay, sendCANFrame);
    for (i = 0; i < item_num; i++)
    {

        items[i] = KP_CreateItem(kph, items_attri[i].m_id, items_attri[i].s_id, items_attri[i].type, NULL, items_attri[i].buff_size);
    
    }
    return 1;

}
/**
 * @brief CAN接口复位
 * 
 */
void Dev_CAN_DeInit(void)
{
    BSP_CAN1_DeInit();
    KP_DeInit(kph);
}

