/**
 * @file usr_rboot.c
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-04
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
#include "usr_rboot.h"


#include "d_flash.h"
#include "usr_can.h"
#include "r_boot_slave.h"
#include "stm32f405_demo1_flash.h"
#include "bitio.h"
#include "lz77.h"
#include "bspatch.h"
#include <stdlib.h>

#include "crc.h"
/**
 * @brief 擦除APP运行区
 * 
 * @param app_size 
 * @return true 
 * @return false 
 */
static bool IAP_EraseAPPRunArea(uint32_t app_size)
{ 
  if(app_size <= 128 * 1024){
    if(BSP_EraseSector(FLASH_SECTOR_5)){
		  return true;
    }
  }
  
  return false;
  
}
/**
 * @brief 擦除APP存储区
 * 
 * @param app_size 
 * @return true 
 * @return false 
 */
static bool IAP_EraseAPPStoreArea(uint32_t app_size)
{ 
  	
  if(app_size <= 128 * 1024){
    if(BSP_EraseSector(FLASH_SECTOR_6)){
		  return true;
    }
  }
  return false;
}

/**
 * @brief r_boot接口
 * 
 * @param addr 
 * @param src 
 * @param len 
 * @return true 
 * @return false 
 */
bool r_boots_write_flash(uint32_t addr, char *src, int len)
{
  bool ret = false;
  static const char *env_key = "r_boot_env";
  uint32_t df_state;

  if(addr == FLASH_ENV_ADDRESS){

    if(df_write(env_key, src, len, &df_state) != DF_F_OK){
      ret = false;
    }else{
      ret = true;
    }

  }else if(addr >= ENV_DEFAULT_CURRENT_APP_ADDR 
  && addr + len < ENV_DEFAULT_CURRENT_APP_ADDR + ENV_DEFAULT_APP_MAX_SIZE){
	if(addr == ENV_DEFAULT_CURRENT_APP_ADDR){
		IAP_EraseAPPRunArea(128 * 1024);
	}
    ret = BSP_WriteByteFlash((uint32_t)addr, (uint8_t *)src, len);

  }else if(addr >= ENV_DEFAULT_NEW_APP_ADDR 
  && addr + len < ENV_DEFAULT_NEW_APP_ADDR + ENV_DEFAULT_APP_MAX_SIZE){
	if(addr == ENV_DEFAULT_NEW_APP_ADDR){
		IAP_EraseAPPStoreArea(128 * 1024);
	}
    ret = BSP_WriteByteFlash((uint32_t)addr, (uint8_t *)src, len);

  }else{
    ret = false;
  }

  return ret;
}
/**
 * @brief r_boot接口
 * 
 * @param addr 
 * @param dest 
 * @param len 
 * @return true 
 * @return false 
 */
bool r_boots_read_flash(uint32_t addr, char *dest, int len)
{
  bool ret = false;
  static const char *env_key = "r_boot_env";
  if(addr == FLASH_ENV_ADDRESS){

    if(df_read(env_key, dest, len) != DF_F_OK){
      ret = false;
    }else{
      ret = true;
    }

  }else if(addr >= ENV_DEFAULT_CURRENT_APP_ADDR 
  && addr + len < ENV_DEFAULT_CURRENT_APP_ADDR + ENV_DEFAULT_APP_MAX_SIZE){

    memcpy(dest, (const void *)addr, len);

    ret = true;
  }else if(addr >= ENV_DEFAULT_NEW_APP_ADDR 
  && addr + len < ENV_DEFAULT_NEW_APP_ADDR + ENV_DEFAULT_APP_MAX_SIZE){

    memcpy(dest, (const void *)addr, len);

    ret = true;
  }else{
    ret = false;
  }

  return ret;
}
/**
 * @brief r_boot接口
 * 
 * @param app_addr 
 * @return true 
 * @return false 
 */
bool r_boots_iap_jump(uint32_t app_addr)
{
  typedef void (*iap_function)(void);
  iap_function iap_fun = NULL;
  uint32_t iap_addr = NULL;

  if(( ( *(volatile uint32_t *)app_addr) & 0x2FFE0000U) == 0x20000000){
	  HAL_DeInit();
    iap_addr = *(volatile uint32_t *)(app_addr + 4);
    iap_fun = (iap_function)iap_addr;
    __set_MSP(*(volatile uint32_t *)iap_addr);
    iap_fun();
  }
  return false;
}
/**
 * @brief r_boot接口
 * 
 * @param addr 
 * @param len 
 * @return true 
 * @return false 
 */
bool r_boots_output_stream(void *addr, int len)
{
	char *tmpbuffer = (char *)malloc(len +4);
	*(uint32_t *)tmpbuffer = (len << 12 | 0);
	memcpy(&tmpbuffer[4], addr, len);
	
	KP_PutItem(kph, items[r_boot_slave_put], tmpbuffer, len+4, NULL);
    if(KP_WriteItem(kph, items[r_boot_slave_put],200,0) != KP_OK){
		free(tmpbuffer);
      return false;
    }else{
		free(tmpbuffer);
      return true;
    }
}

/**
 * @brief 
 * 
 * @param addr 内部flash，区域
 * @param size 字节长度
 * @param src 源地址
 * @return int 
 * -1 err
 * size
 */
static int flash_write(void * addr, uint32_t size, void *src)
{
  if(BSP_WriteByteFlash((uint32_t)addr, src, size) == true){
    return size;
  }else{
    return -1;
  }
}
/**
 * @brief 增量升级接口
 * 
 * @param argc 
 * @param argv 
 * @return true 
 * @return false 
 */
static bool bsdiff_upgrade(int argc, char **argv)
{

    if((argc == 1 && strcmp(argv[0], "-h") == 0) || argc != 7){
      r_boots_ckb_printf(NULL, "\
      param0:old_file_addr,\r\n\
      param1:old_file_size,\r\n\
      param2:old_file_crc16,\r\n\
      param3:patch_file_addr,\r\n\
      param4:patch_file_size,\r\n\
      param5:patch_file_crc16,\r\n\
      param6:new_file_crc,\r\n\
      return:step state.\r\n");
      return true;
    }
    uint32_t old_file_addr = atoi(argv[0]);
    uint32_t old_file_size = atoi(argv[1]);
    uint32_t old_file_crc16 = atoi(argv[2]);
    uint32_t patch_file_addr = atoi(argv[3]);
    uint32_t patch_file_size = atoi(argv[4]);
    uint32_t patch_file_crc16 = atoi(argv[5]);
    uint32_t new_file_crc = atoi(argv[6]);

    // 1、计算增量包CRC
    if(crc16_2((void *)patch_file_addr, patch_file_size) != patch_file_crc16){
      r_boots_ckb_printf(NULL, "step 1:check patch crc error Exit.\r\n");
      return false;
    }
    // 2、擦除解压文件存放区
    bFile_t *bf;
    bf = bitIO_open((void *)patch_file_addr, patch_file_size);
    bf->write = flash_write;
    if(BSP_EraseSector(FLASH_SECTOR_7) != true){
      r_boots_ckb_printf(NULL, "step 2:prepare decompress patch area error Exit.\r\n");
      return false;
    }
    // 3.解压增量包
    uint32_t decomp_addr = BSP_GetFlashAddr(FLASH_SECTOR_7);

    int decomp_size = decode(bf, (void *)decomp_addr, 128*1024);
    if(decomp_size < 0){
      r_boots_ckb_printf(NULL, "step 3:decompress patch file error Exit.\r\n");
      return false;
    }

    // 4.验证增量包信息
    if(memcmp((void *)decomp_addr, "ENDSLEY/BSDIFF43", 16) != 0){
      r_boots_ckb_printf(NULL, "step 4:check decompress file head error Exit.\r\n");
      return false;
    }
    uint32_t new_size = offtin((uint8_t *)(decomp_addr+16));
    uint32_t old_size = offtin((uint8_t *)(decomp_addr+24));
    // 5.check old app
    if(old_size != old_file_size || old_file_crc16 != crc16_2((void *)old_file_addr, old_size)){
      r_boots_ckb_printf(NULL, "step 5:check old file crc error Exit.\r\n");
      return false;
    }
    // 6.擦除融合后新APP 存放区
    if(IAP_EraseAPPStoreArea(new_size) != true){
      r_boots_ckb_printf(NULL, "step 6:prepare new app area error Exit.\r\n");
      return false;
    }

    // 7.融合计算到new APP存放区
    uint32_t new_app_addr = BSP_GetFlashAddr(FLASH_SECTOR_6);
    if(bspatch((const uint8_t *)old_file_addr, old_size,
              (uint8_t *)new_app_addr, new_size,
              (uint8_t *)(decomp_addr + 32), decomp_size - 32, flash_write) < 0){
      r_boots_ckb_printf(NULL, "step 7:generate new app error Exit.\r\n");
      return false;
    }
    r_boots_ckb_printf(NULL, "bsdiff_upgrade ok.\r\n");
    return true;
}

R_BOOTS_CALLBACK_OBJDEF(bsdiff_upgrade)


/**
 * @brief 用户注册一些功能命令，需要在r_boots_init后调用
 * 
 * 1、增量升级命令
 * 
 * 
 * 
 */
void usr_rboot_init(void)
{
  r_boots_new_cbk_obj(R_BOOTS_CALLBACK_OBJ(bsdiff_upgrade));
}

