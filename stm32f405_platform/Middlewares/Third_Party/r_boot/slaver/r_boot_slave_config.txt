/**
 * @file r_boot_slave_config.h
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
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
#ifndef __R_BOOT_SLAVE_CONFIG_H__
#define __R_BOOT_SLAVE_CONFIG_H__
#include <stdint.h>
#include <stdbool.h>

#include "r_boot_slave_env.h"


/********************************** 用户可修改 *********************************/



/* 环形缓存空间大小 */
#define IAP_PROTOCOL_RING_BUFFER_SIZE   (1024*4)

/* 环境变量地址存储地址和大小限制，需要和用户提供的读写flash接口内保持一致 */
#define FLASH_ENV_ADDRESS               (0x08008000U)
#define FLASH_ENV_MAX_SIZE              (256U)

/*用户接口*/
bool r_boots_write_flash(uint32_t addr, char *src, int len);
bool r_boots_read_flash(uint32_t addr, char *dest, int len);
bool r_boots_iap_jump(uint32_t app_addr);
bool r_boots_output_stream(void *addr, int len);



/* 环境变量相关参数 */
#define ENV_MAX_NUMBER                  (16)
#define ENV_DEFAULT_VALUE               (NULL)
#define ENV_DEFAULT_CURRENT_APP_ADDR    (0x08020000U)
#define ENV_DEFAULT_NEW_APP_ADDR        (0x08060000U)
#define ENV_DEFAULT_APP_MAX_SIZE        (256*1024)
#define ENV_DEFAULT_BOOT_ADDR           (ENV_DEFAULT_CURRENT_APP_ADDR)
#define ENV_DEFAULT_DELAY_TIME          (30000)         //seconds

/* r_printf缓存空间大小 */
#define CALLBACK_PRINTF_BUFFER_SIZE     (256)

/* 是否开启自动延时跳转，不开启请注释 */
#define ENABLE_AUTO_JUMP_IAP

/* 是否使用通信执行命令功能，不开启请注释 */
#define ENABLE_PROTOCOL_CALLBACK


/**
 * @brief 定义环境变量
 * 
 */
#define ENV_ENUM(XX) \
    XX(r_boots_version,=0) \
    XX(current_app_length,) \
    XX(current_app_crc,) \
    XX(current_app_ecrc,) \
    XX(current_app_addr,) \
    XX(new_app_length,) \
    XX(new_app_crc,) \
    XX(new_app_ecrc,) \
    XX(new_app_addr,) \
    XX(bootcmd,) \
    XX(app_max_size,) \
    XX(boot_address,) \
    XX(state_code,) \
    XX(delay_time,) \
    XX(variable_number,=ENV_MAX_NUMBER)\
    XX(crc16,) \






#endif


