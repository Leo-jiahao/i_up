/**
 * @file d_flash.c
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 源文件
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

/* Includes ------------------------------------------------------------------*/
#include "d_flash.h"
#include "murmur3.h"
/** @defgroup Utilities
  * @{
  */
/** @defgroup  d flash
  * @{
  */

/** @defgroup  d_flash Private TypesDefinitions 私有类型
  * @{
  */
#if ENABLE_D_FLASH == 1

/**
 * @brief 全空间标记节点
 * 
 */
typedef struct
{
    uint32_t flag_A : 4;     //0xA
    uint32_t swapcount : 28;     //
}DF_HashNode_Flag;
/**
 * @brief 剩余空间哨兵节点
 * 
 */
typedef struct 
{
    /* data */
    uint32_t DF_free_addr;     //数据偏移地址
    uint32_t DF_remain_size;     //数据剩余空间
}DF_HashNode_Guard;
/**
 * @brief 数据节点
 * 
 */
typedef struct 
{
    /* data */
    uint32_t DF_addr;     //数据偏移地址,指向数据并非数据节点
    uint32_t DF_size;     //数据非对齐大小
}DF_DataNode;
/**
 * @brief Table Head Node表头的数据节点
 * 
 */
typedef struct 
{
    /* data */
    uint32_t DF_HashNode_addr;     //数据的第一个地址
    uint32_t DF_HashNode_size;     //数据长度
    uint32_t DF_HashNode_key;    //key的hash值
}DF_HashTHN_data;
typedef struct 
{
    /* data */
    DF_HashNode_Flag DF_flag;
    DF_HashNode_Guard DF_HashNode_Guard;//已经使用的数据起始地址和大小//作为数据表的第一个节点
    uint16_t item_num;
    uint16_t item_size;
}DF_FlashHead_TypeDef;
typedef struct 
{
    /* data */
    uint32_t DF_Flash_StartAddr;  //开始地址
    uint32_t DF_Flash_AllSize;   //全部大小
    DF_HashNode_Guard DF_Flash_Guard;//最新的守护节点
    DF_DataNode DF_Flash_DNArray[ITEM_MAX_NUM];//最新的数据结点地址
}DF_FlashCtl_TypeDef;

/**
  * @}
  */
/** @defgroup  d_flash Private Defines 私有宏定义
  * @{
  */

/**
  * @}
  */

/** @defgroup d_flash Private Macros 私有宏指令
  * @{
  */

/**
  * @}
  */
 /** @defgroup d_flash Private Variables 私有变量
  * @{
  */
 static DF_FlashCtl_TypeDef fctl={0,};
 /**
  * @}
  */

/** @defgroup  d_flash Private FunctionPrototypes 私有函数
  * @{
  */
/**
 * @brief 连续读flash
 * 
 * @param address 4Byte对齐
 * @param buff u32
 * @param length 4Byte对齐
 */
static void DF_ReadFlashFunction(uint32_t address, uint32_t *dest,uint16_t length)
{
	for(int i = 0; i < length; i++)
	{
		dest[i] = FLASH_READ_U32(address + i * 4);   //read data
	}
}
/**
 * @brief 字符串hash函数
 * 
 * @param str 输入的字符串
 * @return uint64_t 输出的hash值
 */
static uint32_t BKDHash(const char *str)
{
	uint32_t ret;
	MurmurHash3_x86_32(str, strlen(str), 1204, (void *)&ret);
	//ret = MurmurHash2_32(str, strlen(str));
	return ret;
}

/**
 * @brief 计算str的hash值，在0->item_num范围内找到一个值，作为条目的index,第一次会写key数组
 * 
 * @param str 
 * @param item_num 条目最大数
 * @param iswstr 1写入str，0不写入
 * @return int <0 error
 */
static int DF_getIndexFromHash(const char *str, uint16_t item_num, uint8_t iswstr)
{
	uint32_t hash = BKDHash(str);
	uint16_t ret = hash % item_num;
	uint8_t i=0;
	uint32_t tabaddr = fctl.DF_Flash_StartAddr + sizeof(DF_FlashHead_TypeDef);
	DF_HashTHN_data h_n;
	do{
		DF_ReadFlashFunction(tabaddr+ ret*sizeof(DF_HashTHN_data),(uint32_t *)&h_n,sizeof(h_n)>>2);
		if(hash == h_n.DF_HashNode_key)
		{
			return ret;
		}
		ret+=1;
		ret = ret % item_num;
		i++;
		if (i>item_num)
		{
			/* code */
			return -1;
		}
		
	}while(h_n.DF_HashNode_addr!=0xFFFFFFFF && h_n.DF_HashNode_size !=0xFFFFFFFF);
	//写入str,是否加入
	if(ret==0)
	{
		ret = 99;
	}else{
		ret = ret - 1;
	}
	if(iswstr)
	while(FLASH_WRITE(sizeof(DF_FlashHead_TypeDef) + fctl.DF_Flash_StartAddr + ret*sizeof(DF_HashTHN_data)+8,(uint32_t *)&hash,1) != true);

	return ret;
}
/**
 * @brief 遍历找到最后一个合法的guard节点，最后一个节点的下一个节点值为 0xffffffff
 * 
 * @param p_g 
 * @return int Err -1(gnode偏移地址超出边界)
 */
static int DF_FindEndGNode(DF_HashNode_Guard *p_g)
{
	
	if(p_g->DF_free_addr >= fctl.DF_Flash_AllSize)return FErr;
	while (FLASH_READ_U32(p_g->DF_free_addr + fctl.DF_Flash_StartAddr) !=0xFFFFFFFF)
	{
		if(p_g->DF_free_addr >= fctl.DF_Flash_AllSize)return FErr;
		p_g->DF_remain_size = FLASH_READ_U32(p_g->DF_free_addr+4+fctl.DF_Flash_StartAddr);
		p_g->DF_free_addr = FLASH_READ_U32(p_g->DF_free_addr+fctl.DF_Flash_StartAddr);
	}

	return FSuc;
}

/**
 * @brief 将数据写入到flash中，更新guard节点，更新data节点。如果空间不足，报错
 * 
 * @param src 数据源地址
 * @param size 数据字节长度
 * @param p_d 数据节点，如果是首节点，则节点地址和大小都是 0xFFFFFFFF
 * @param h_i head index.条目数，如果是首节点，则用其找到首节点地址
 * @return int Err -1数据错误  -2:空间不足
 */
static int DF_Write2Flash(char *src,uint32_t size,DF_DataNode *p_d,uint16_t h_i)
{
	uint32_t gnewaddr,dnewaddr;
	DF_HashNode_Guard *p_g = (DF_HashNode_Guard *)&fctl.DF_Flash_Guard;
	uint32_t len = size;
	if(src == NULL || p_d ==NULL)
	{
		return FErr;
	}
	if(len + sizeof(DF_HashNode_Guard) + sizeof(DF_DataNode) > p_g->DF_remain_size)//空间不足，报错
	{
		return -2;
	}
	gnewaddr = p_g->DF_free_addr;
	if(p_d->DF_addr <= fctl.DF_Flash_AllSize)		//中间节点地址
	{
		dnewaddr = p_d->DF_addr + p_d->DF_size;
	}else if(p_d->DF_addr == 0xFFFFFFFF && p_d->DF_size == 0xFFFFFFFF)			//首节点地址
	{
		dnewaddr = h_i * sizeof(DF_HashTHN_data) + sizeof(DF_FlashHead_TypeDef);
	}
	
	while(FLASH_WRITE(p_g->DF_free_addr + fctl.DF_Flash_StartAddr + sizeof(DF_HashNode_Guard),(uint32_t *)src,len>>2) != true);	//写数据
	p_d->DF_addr = p_g->DF_free_addr + sizeof(DF_HashNode_Guard);												//更新传入的数据节点，
	p_d->DF_size = size;
	fctl.DF_Flash_DNArray[h_i].DF_addr = p_d->DF_addr;											//更新控制结构体中的数据节点
	fctl.DF_Flash_DNArray[h_i].DF_size = p_d->DF_size;
	while(FLASH_WRITE(dnewaddr+fctl.DF_Flash_StartAddr,(uint32_t *)p_d,sizeof(DF_DataNode)>>2) != true);							//更新flash中的data node
	p_g->DF_remain_size = p_g->DF_remain_size - (len + sizeof(DF_HashNode_Guard)+sizeof(DF_DataNode));			//更新ctl中的哨兵结点
	p_g->DF_free_addr = p_g->DF_free_addr+ len + sizeof(DF_HashNode_Guard)+sizeof(DF_DataNode);
	while(FLASH_WRITE(gnewaddr+fctl.DF_Flash_StartAddr,(uint32_t *)p_g,sizeof(DF_HashNode_Guard)>>2) != true);					//更新flash中guard_node
	return FSuc;
}
/**
 * @brief 遍历找到最后一个合法的Data节点，最后一个节点的下一个节点值为 0xffffffff
 * 
 * @param p_h 头节点指针
 * @param p_d 得到最后的合法节点，如果最后节点就是头节点则赋值 0xFFFFFFFF
 * @return int Err -1
 */
static int DF_FindEndDNode(DF_HashTHN_data *p_h,DF_DataNode *p_d)
{
	if(p_h->DF_HashNode_addr == 0xFFFFFFFF && p_h->DF_HashNode_size == 0xFFFFFFFF)
	{
		p_d->DF_addr = 0xFFFFFFFF;
		p_d->DF_size = 0xFFFFFFFF;
		return FSuc;
	}
	p_d->DF_addr = p_h->DF_HashNode_addr+p_h->DF_HashNode_size;
	p_d->DF_size = p_h->DF_HashNode_size;
	while (FLASH_READ_U32(p_d->DF_addr + fctl.DF_Flash_StartAddr) !=0xFFFFFFFF)
	{
		if(p_d->DF_addr >= fctl.DF_Flash_AllSize)return FErr;
		p_d->DF_size = FLASH_READ_U32(p_d->DF_addr+4+fctl.DF_Flash_StartAddr);
		p_d->DF_addr = FLASH_READ_U32(p_d->DF_addr+fctl.DF_Flash_StartAddr) + p_d->DF_size;
	}
	p_d->DF_addr-=p_d->DF_size;
	return FSuc;
}
/**
 * @brief 保存有效数据到目标地址,转入前应该确保擦除，转移完成后最后写入flag，交换次数增加。不擦除上一分区
 * 
 * @param swapaddr 目标分区地址
 * @param swapsize 目标分区大小
 * @return int -2 目标分区大小不够
 */
static int DF_Flash_transfer(uint32_t swapaddr,uint32_t swapsize,uint32_t swapcount)
{
	uint32_t dataddr;
	DF_FlashHead_TypeDef hn;
	DF_HashTHN_data thn;
	//1 头部信息
	hn.item_num = ITEM_MAX_NUM;
	hn.item_size = sizeof(DF_HashTHN_data);
	hn.DF_flag.flag_A = 0xA;
	hn.DF_flag.swapcount = swapcount+1;
	dataddr = sizeof(DF_FlashHead_TypeDef) + ITEM_MAX_NUM * sizeof(DF_HashTHN_data);
	for (int i = 0; i < ITEM_MAX_NUM; i++)
	{
		// 2 查找有效条目
		if(fctl.DF_Flash_DNArray[i].DF_addr != 0)
		{
			if(dataddr >= swapsize)//目标分区大小不够
			{
				return -2;
			}
			thn.DF_HashNode_addr = dataddr;
			thn.DF_HashNode_size = fctl.DF_Flash_DNArray[i].DF_size;
			memcpy(
				(void *)&thn.DF_HashNode_key,
				(void *)(fctl.DF_Flash_StartAddr + sizeof(DF_FlashHead_TypeDef) + i * sizeof(DF_HashTHN_data) + sizeof(uint32_t)*2),
				sizeof(thn.DF_HashNode_key));
			//3 写条目的头节点
			while(FLASH_WRITE(swapaddr+i*sizeof(DF_HashTHN_data)+sizeof(DF_FlashHead_TypeDef),(void *)&thn,sizeof(thn)>>2) != true);
			//4 写条目数据
			while(FLASH_WRITE(swapaddr+dataddr, (void *)(fctl.DF_Flash_DNArray[i].DF_addr+fctl.DF_Flash_StartAddr),thn.DF_HashNode_size>>2) != true);
	//5 更新fctl相关信息
			fctl.DF_Flash_DNArray[i].DF_addr = thn.DF_HashNode_addr;
			dataddr += sizeof(DF_DataNode)+thn.DF_HashNode_size;
		}
	}
	fctl.DF_Flash_AllSize = swapsize;
	fctl.DF_Flash_StartAddr = swapaddr;
	fctl.DF_Flash_Guard.DF_free_addr = dataddr;
	fctl.DF_Flash_Guard.DF_remain_size = swapsize - dataddr;
	//6 最后写入头部信息
	hn.DF_HashNode_Guard.DF_free_addr = dataddr;
	hn.DF_HashNode_Guard.DF_remain_size = fctl.DF_Flash_Guard.DF_remain_size;
	while(FLASH_WRITE(swapaddr,(uint32_t *)&hn,sizeof(hn)>>2) != true);
	return 0;
}
/**
 * @brief 用户初始化，两个独立分区，默认最开始使用分区1，
 * 
 * @param state 返回状态信息 
 * @return DF_Code 返回状态码
 * @par Transfer_Err:state转存分区的大小，读写正常只是单独分区已满需要转存，并非故障类错误
 * @par D_NODE_ERR:严重错误，正常情况下不会发生
 */
static DF_Code DF_FlashInitFunction(void *state)
{
	DF_HashTHN_data hn={0,};
	DF_DataNode dn={0,};
	DF_HashNode_Flag flag1,flag2;
	DF_FlashHead_TypeDef head={0,};
	DF_HashNode_Flag *p_fnode = &head.DF_flag;
	DF_HashNode_Guard *p_gnode = &head.DF_HashNode_Guard;
	memset((void*)&fctl, 0, sizeof(DF_FlashCtl_TypeDef));

	DF_ReadFlashFunction(FLASH_START_ADDR_1B,(uint32_t *)&flag1,sizeof(DF_HashNode_Flag)>>2);
	DF_ReadFlashFunction(FLASH_START_ADDR_2B,(uint32_t *)&flag2,sizeof(DF_HashNode_Flag)>>2);
	//1 定位到正在使用的分区
	if(flag1.flag_A==0xA && flag2.flag_A==0xA)
	{	
		if(flag1.swapcount==flag2.swapcount+1)
		{
			//分区1
			fctl.DF_Flash_AllSize = FLASH_SIZE_1B;
			fctl.DF_Flash_StartAddr = FLASH_START_ADDR_1B;
		}else if(flag2.swapcount==flag1.swapcount+1)
		{
			//分区2
			fctl.DF_Flash_AllSize = FLASH_SIZE_2B;
			fctl.DF_Flash_StartAddr = FLASH_START_ADDR_2B;
		}else
		{
			if(flag1.swapcount<flag2.swapcount)
			{
				fctl.DF_Flash_AllSize = FLASH_SIZE_1B;
				fctl.DF_Flash_StartAddr = FLASH_START_ADDR_1B;
			}else
			{
				fctl.DF_Flash_AllSize = FLASH_SIZE_2B;
				fctl.DF_Flash_StartAddr = FLASH_START_ADDR_2B;
			}
		}
	}else if(flag1.flag_A==0xA && flag2.flag_A==0xF)
	{
		//分区1
		fctl.DF_Flash_AllSize = FLASH_SIZE_1B;
		fctl.DF_Flash_StartAddr = FLASH_START_ADDR_1B;
	}else if(flag1.flag_A==0xF && flag2.flag_A==0xA)
	{
		//分区2
		fctl.DF_Flash_AllSize = FLASH_SIZE_2B;
		fctl.DF_Flash_StartAddr = FLASH_START_ADDR_2B;
	}else
	{
		//分区1 初始
		fctl.DF_Flash_AllSize = FLASH_SIZE_1B;
		fctl.DF_Flash_StartAddr = FLASH_START_ADDR_1B;
		head.DF_flag.swapcount = 1;
	}
	//2 1区第一次上电，需要擦除，写入初始数据
	if(head.DF_flag.swapcount == 1)
	{

		p_fnode->flag_A=0xA;
		p_fnode->swapcount = 1;
		p_gnode->DF_remain_size = 
			fctl.DF_Flash_AllSize - sizeof(DF_HashTHN_data)*ITEM_MAX_NUM  - sizeof(DF_FlashHead_TypeDef);
		
		p_gnode->DF_free_addr = 
			sizeof(DF_HashTHN_data)*ITEM_MAX_NUM  + sizeof(DF_FlashHead_TypeDef);

		head.item_num = ITEM_MAX_NUM;
		head.item_size = sizeof(DF_HashTHN_data);
		while( FLASH_ERASE_1B!= true);
		while(
			FLASH_WRITE(fctl.DF_Flash_StartAddr,
			(void *)&head,
			sizeof(head)>>2) != true);
		fctl.DF_Flash_Guard.DF_remain_size = p_gnode->DF_remain_size;
		fctl.DF_Flash_Guard.DF_free_addr = p_gnode->DF_free_addr;
	}
	else
	{
		//3 避免读时查找，提前定位数据节点
		DF_ReadFlashFunction(fctl.DF_Flash_StartAddr,(uint32_t *)&head,sizeof(head)>>2);

		for (int i = 0; i < ITEM_MAX_NUM; i++)
		{
			//3.1 头节点
			DF_ReadFlashFunction(
				fctl.DF_Flash_StartAddr + sizeof(DF_FlashHead_TypeDef) + i * sizeof(DF_HashTHN_data),
				(uint32_t *)&hn,
				sizeof(DF_HashTHN_data)>>2);
			//3.2 查找最新数据节点
			if(hn.DF_HashNode_addr != 0xFFFFFFFF && hn.DF_HashNode_size != 0xFFFFFFFF)
			{
				if(DF_FindEndDNode(&hn,&dn) < 0)
				{
					return D_NODE_ERR;
				}
			//3.3更新节点
				fctl.DF_Flash_DNArray[i].DF_addr = dn.DF_addr;
				fctl.DF_Flash_DNArray[i].DF_size = dn.DF_size;
			}
		}
			//3.4 查找最新的Guard节点
		if(DF_FindEndGNode(p_gnode)<0)
		{	//err
			
			*(uint32_t *)state =  p_gnode->DF_remain_size;
			return G_NODE_ERR;
		}
		//4 判断是否需要转存数据
		if(p_gnode->DF_remain_size <= (100-TRANSFER_THRESHOLD_PERCENT)*fctl.DF_Flash_AllSize/100)
		{
			if(fctl.DF_Flash_StartAddr == FLASH_START_ADDR_1B)
			{	//4.1 擦除目标区域
				FLASH_ERASE_2B;
				//4.2 转移数据
				if(DF_Flash_transfer(FLASH_START_ADDR_2B,FLASH_SIZE_2B,p_fnode->swapcount)<0){
					*(uint32_t *)state = FLASH_SIZE_2B;
					return Transfer_Err;
				}
			}else
			{
				//4.1 擦除目标区域
				FLASH_ERASE_1B;
				//4.2 转移数据
				if(DF_Flash_transfer(FLASH_START_ADDR_1B,FLASH_SIZE_1B,p_fnode->swapcount)<0){
					*(uint32_t *)state = FLASH_SIZE_1B;
					return Transfer_Err;
				}
			}
		}else
		{
			fctl.DF_Flash_Guard.DF_remain_size = p_gnode->DF_remain_size;
			fctl.DF_Flash_Guard.DF_free_addr = p_gnode->DF_free_addr;	
		}
	}
	*(uint32_t *)state =fctl.DF_Flash_Guard.DF_remain_size;
	return DF_F_OK;
}

/**
 * @brief 写入条目及其内容，如果不存在则创建，存在则修改
 * 
 * @param key 
 * @param src 
 * @param size 
 * @param state
 * @return DF_Code 
 * @par 其他 state为剩余空间大小
 * @par Write_ERR 参数错误时 state为NULL
 */
static DF_Code DF_FlashInsertFunction(const char *key, void *src, uint32_t size, uint32_t *state)
{
	//DF_HashTHN_data hn;
	DF_DataNode dn;
	int ret;
	int item_index;

	item_index = DF_getIndexFromHash(key,USER_ITEM_MAX_NUM,1);
	if(item_index < 0 || item_index > USER_ITEM_MAX_NUM)return H_NODE_ERR;
	if(fctl.DF_Flash_DNArray[item_index].DF_addr==0){//新数据
		dn.DF_addr = 0xFFFFFFFF;
		dn.DF_size = 0xFFFFFFFF;
	}else
	{
		dn.DF_addr = fctl.DF_Flash_DNArray[item_index].DF_addr;
		dn.DF_size = fctl.DF_Flash_DNArray[item_index].DF_size;
	}

	ret = DF_Write2Flash(src,size,&dn,item_index);
	if(ret == -1)//参数错误
	{
		*state = 0;
		return Write_ERR;
	}else if(ret == -2)//空间不足
	{
		*state = fctl.DF_Flash_Guard.DF_remain_size;
		return Storage_Full;
	}
	*state = fctl.DF_Flash_Guard.DF_remain_size;
	return DF_F_OK;
}
/**
 * @brief read item
 * 
 * @param key 
 * @param dest 
 * @param size 
 * @return DF_Code 
 */
static DF_Code DF_FlashReadItemFunction(const char *key, void *dest, uint32_t size)
{
	int item_index;

	item_index = DF_getIndexFromHash(key,USER_ITEM_MAX_NUM,0);
	if(item_index < 0 || item_index > USER_ITEM_MAX_NUM)return H_NODE_ERR;
	
	if(fctl.DF_Flash_DNArray[item_index].DF_addr == 0 && fctl.DF_Flash_DNArray[item_index].DF_size == 0)
	{
		return Read_Err;
	}
	if(size != fctl.DF_Flash_DNArray[item_index].DF_size){
		return Read_SizeErr;
	}else
	{
		DF_ReadFlashFunction(fctl.DF_Flash_DNArray[item_index].DF_addr + fctl.DF_Flash_StartAddr, (uint32_t *)dest, size >>2);
	}

	return DF_F_OK;
}

 #endif
 /**
  * @}
  */
/** @defgroup  d_flash  Public Functions 共有函数
  * @{
  */

/**
 * @brief 初始化
 * 
 * @param state 返回状态信息 
 * @return DF_Code 返回状态码
 * @par Transfer_Err:state转存分区的大小，读写正常只是单独分区已满需要转存，并非故障类错误
 * @par D_NODE_ERR:严重错误，正常情况下不会发生
 */
DF_Code df_init(void *state)
{
    #if ENABLE_D_FLASH == 1
	return DF_FlashInitFunction(state);
    #else
    return DF_S_ERR;
	#endif
}

/**
 * @brief size必须是4的整数倍，否则参数错误
 * 
 * @param key 
 * @param dest 
 * @param size 
 * @return DF_Code 
 */
DF_Code df_read(const char *key, void *dest, uint32_t size)
{
    #if ENABLE_D_FLASH == 1
	if(size % 4 != 0)
	return DataKey_SizeErr;
	return DF_FlashReadItemFunction(key,dest,size);
    
	#else
    return DF_S_ERR;
	#endif
}
/**
 * @brief size必须是4的整数倍，否则参数错误，写入条目及其内容，如果不存在则创建，存在则修改
 * 
 * @param key 
 * @param src 
 * @param size 
 * @param state
 * @return DF_Code 
 * @par 其他 state为剩余空间大小
 * @par Write_ERR DataKey_SizeErr 参数错误时 state为NULL
 */
DF_Code df_write(const char *key, void *src, uint32_t size,uint32_t *state)
{
    #if ENABLE_D_FLASH == 1


	if(size % 4 != 0)
	return DataKey_SizeErr;
	return DF_FlashInsertFunction(key,src,size,state);



    #else
    return DF_S_ERR;
	#endif
}
/**
 * @brief 强制擦除
 * 
 * @return DF_Code 
 */
DF_Code df_erase(void)
{
    #if ENABLE_D_FLASH == 1
    
	FLASH_ERASE_1B;
	FLASH_ERASE_2B;
	memset((void *)&fctl,0,sizeof(DF_FlashCtl_TypeDef));
	return DF_F_OK;

    #else
    return DF_S_ERR;
	#endif
}

