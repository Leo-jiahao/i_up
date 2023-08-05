# 一种CAN通信模块说明

**date:2022/12/20
author:Leo-jiahao (leo884823525@gmail.com)
version：2.0.0**

## 1.1、概述
可4字节单数据类型传输  
可支持块的数据传输，块大小限制， (256 - 1 - 1) * 4 = 1016字节  
可支持文件传输，基于块，有定义一种文件传输协议，文件最大限制为：1MB， 1*2^20 = 1,048,576 Bytes = 1MB  
支持回调函数命令类功能。**对于此功能，主设备注册时建议保持为空值其他值也无所谓，本质上发送协议中没有使用，从设备的注册条目时默认值必须为函数指针的值，这点不一致，需要注意**  


## 1.2、其他说明
1. 控制句柄：用于描述一个通信设备，管理该通信设备的所有条目节点。如果是子设备，只需要初始化一个用于描述自生的控制句柄。
如果是主设备，则需要为每一个子设备都初始化一个独立的控制句柄。
2. 条目：类似于CANopen里的对象字典，用于描述一个参数对象。具体的区分参数为：主id、子id，各自允许的取值范围为[0,255], 和类型，int,float,
block，三种常用类型。  
3. 条目的管理，是通过两级链表实现的，主id，有一个最长256的链表，每一级链表上有最长256个节点，理论上可以有256*256个节点（条目），
但是MCU内部ram极其有限，且有块传输协议，没必要定义太多过分详细的节点，遇到这种情况**建议使用结构体基于块传输协议实现**。  

## 1.3、移植
1. 总共3个文件，can_protocol.c, can_protocol.h, kpcan.h。只需将此三个文件都拷贝、添加到工程中。
2. 留给用户调用的接口位于kpcan.h，一般不需改动。

## 1.4、使用
1. 创建、初始化,传入内存分配接口，延时函数接口等，成功的话会返回一个和设备高度绑定的控制句柄，
   后续对条目的操作都是基于此控制句柄
   ```c
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

   ```
2. 注册条目，函数内会动态创建一个条目，并挂载到当前控制句柄下的主链表、子链表，如果是首次创建此主id,则新建一个子链表，再进行挂载。  
   如果此节点已经被创建过，则修改此节点的数据类型、重新分配（先free old_buff）此节点的buff等,再进行挂载。  
   注册成功会返回条目的指针，错误时，返回0
   ```c
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
   item_t KP_CreateItem(KP_handle kph, uint8_t m_id, uint8_t s_id, KP_Data_TypeDef data_type, void *buff, uint32_t buff_size)

   ```
3. 解析的CAN数据帧的设备id应该和和控制句柄一致，如果不一致会报错退出。  
   对于4字节条目数据，协议内解析成功后会自动更新到对应条目的数据buff，如果需要应答（如，主设备需要读从设备条目），则通过函数返回值和reply帧进行传递。  
   对于块数据，**只支持主设备写，不支持读操作**，第一帧，会清空条目的buff区，中间帧默认不进行应答，结束帧会进行crc校验后进行应答,**这里的应答是通过返回值和reply告知外部用户，并非内部自行应答**  
   ```c
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

   ```
4. 获取条目的数据,此接口时间损耗多一点，但是相当安全，推荐使用。  
   从控制句柄中，查找对应主id、子id，并且判断空间大小是否一致，如果一致，memcpy()到目的地址
   
   ```c
   /**
    * @brief 获取对应条目的数据，首先检查对应空间大小是否符合要求，然后内存拷贝或者传出内存空间的指针，严格遵守条目的数据类型
    * 
    * @param kph 处理通信节点描述句柄
    * @param m_id 主id
    * @param s_id 子id
    * @param dest 目的地址
    * @param size 目的地址空间大小，务必等于条目创建时设定的大小
    * @return KP_State_TypeDef 
    */
   KP_State_TypeDef KP_GetItem(KP_handle kph, uint8_t m_id, uint8_t s_id, void *dest, uint16_t size)


   ```

5. 更新条目的数据内容,*正常情况下，是通过主id，子id进行更新，但是考虑到效率问题，这里使用了条目指针*  
   **建议后续做出更改，如果只是这种方式不太安全**，虽然加入了互斥锁进行保护，但仍然有风险。
   ```c
   /**
    * @brief 更新条目的数据,并且获取发送帧
    * 
    * @param kph 控制句柄
    * @param item 条目创建时返回的id
    * @param src 数据源地址
    * @param size 数据大小
    * @param reply 新数据整合为一帧
    * @return KP_State_TypeDef 
    */
   KP_State_TypeDef KP_PutItem(KP_handle kph, item_t item, void *src, uint16_t size, KP_canframe reply)

   ```
6. 将条目数据整合到一个完整的can帧，不支持块数据
   ```c
   /**
    * @brief 直接获取数据帧
    * 
    * @param kph 
    * @param item 
    * @param reply 
    * @return KP_State_TypeDef 
    */
   KP_State_TypeDef KP_GetItemFrame(KP_handle kph, item_t item, KP_canframe reply)

   ```
7. 获取条目的数据buff空间地址，空间大小,*有风险*，可以使用主id、子id进行操作。
   现在这样无法判断空间是否被释放，因此可能造成硬件级别故障，前面的接口也有类似风险。**可以重构优化**
   ```c
   /**
    * @brief 获取对应条目数据所在的地址,通常用于块条目
    * 
    * @param kph 控制句柄
    * @param m_id 主id
    * @param s_id 子id
    * @param size 用于获取条目中数据的大小创建属性时的大小
    * @return void* 遇到错误返回 NULL，正常情况下返回对应条目数据所在的地址
    */
   void * KP_GetItemAddr(KP_handle kph, item_t item, uint16_t *size)
   ```
8. 向CAN总线上其他设备写条目信息，需要等待回复，如果指定timeout=0则，无需等待。*同样使用条目指针，是有风险的*  
   如果是4字节条目，在delay后会很快（*这里会在解析函数中操作ack帧，并将结果同步到此函数*）得到回复。  
   如果是块条目，这里需要等待所有帧都发送结束或者第一帧就出错才会退出。
   ```c
   /**
    * @brief 向其他设备写条目中的信息，单帧,很快即可得到回复。对于块，需要很长时间得到回复，如果timeout=0不保证操作正确.
    *        尽量避免同一条目的重入，尤其是块条目
    * 
    * @param kph 
    * @param item 
    * @param timeout 
    * @return KP_State_TypeDef 
    */
   KP_State_TypeDef KP_WriteItem(KP_handle kph, item_t item, uint32_t timeout)
   ```
9. 从其他设备读条目，只支持单帧，在delay后会很快（*这里会在解析函数中操作ack帧，并将结果同步到此函数*）得到回复。  
   会将数据读到当前设备为从设备创建的控制句柄中注册的条目buff中。
   ```c
   /**
    * @brief 从其他设备读条目中的信息，只支持单帧，需要等待从设备回复
    * 
    * @param kph 为其他设备注册的通信描述结构体
    * @param item 当前设备注册的条目，（目标设备应该有一个相同m_id s_id的条目），完全支持，float int类型，对于回调类型不需要从设备ack，不支持读块条目
    * @param dest 数据的存放目的地
    * @param timeout 等待设备回复的时间，如果为0，则代表不等待结果，直接返回正确。经过测试1M下，推荐40
    * @return KP_State_TypeDef 
    */
   KP_State_TypeDef KP_ReadItem(KP_handle kph, item_t item, void *dest, uint16_t timeout)


   ```

## 1.5、文件传输协议
1. 概述  
   文件传输，是完全基于块传输协议的。  
   只不过是在每一块的第一帧，即前4个字节，用前20bits表示总体文件大小，后12bits表示当前块的序列  
   CAN帧数据的data[3],表示块的帧序号，data[3]==0块传输开始，后面的20bits表示大小，12bits块序号  
   data[3] == 255 表示块传输结束 data[4] + data[5] * 256 == 当前块字节大小，data[6] + data[7] * 256 == 当前块的crc16  
   根据crc校验状态进行返回*同样使用 KP_WriteItem 接口*。  
   ```c
   /**
    * @brief 向其他设备写条目中的信息，单帧,很快即可得到回复。对于块，需要很长时间得到回复，如果timeout=0不保证操作正确.
    *        尽量避免同一条目的重入，尤其是块条目
    * 
    * @param kph 
    * @param item 
    * @param timeout 
    * @return KP_State_TypeDef 
    */
   KP_State_TypeDef KP_WriteItem(KP_handle kph, item_t item, uint32_t timeout)

   ```
   因此需要用户自行更新条目的数据buff,同样接收方也使用*KP_ReadItem接口*，也需要用户在外部，获取文件块传输状态后，将块数据进行拷贝、转移
   ```c
   /**
    * @brief 从其他设备读条目中的信息，只支持单帧，需要等待从设备回复
    * 
    * @param kph 为其他设备注册的通信描述结构体
    * @param item 当前设备注册的条目，（目标设备应该有一个相同m_id s_id的条目），完全支持，float int类型，对于回调类型不需要从设备ack，不支持读块条目
    * @param dest 数据的存放目的地
    * @param timeout 等待设备回复的时间，如果为0，则代表不等待结果，直接返回正确。经过测试1M下，推荐40
    * @return KP_State_TypeDef 
    */
   KP_State_TypeDef KP_ReadItem(KP_handle kph, item_t item, void *dest, uint16_t timeout)

   ```
   

## 1.6、心跳功能
```c
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
```