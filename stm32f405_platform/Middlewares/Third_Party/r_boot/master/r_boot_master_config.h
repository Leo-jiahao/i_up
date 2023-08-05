/**
 * @file r_boot_master_config.h
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

#ifndef __R_BOOT_MASTER_CONFIG_H__
#define __R_BOOT_MASTER_CONFIG_H__

#include <stdio.h>
#include <unistd.h>
#define R_BOOT_MASTER_LOG_ENABLED 1


#define ARG_COUNT_MAX       (10)


/**最大请求数量*/
#define REQUEST_ASYNC_MAX_BUM     (20)

#define REQUEST_SYNC_MAX_BUM     (5)

/*请求超时时间*/
#define REQUEST_TIMEOUT_S (3)

/*each one request file buffer size*/
#define FTP_ERB_MAX_SIZE            (512)
#define FTP_FILE_NAME_MAX_LENGTH    (30)


#define R_BOOTM_DELAY_MS(timeout)   usleep(timeout*1000)


// /*保护超时处理函数和数据解析*/
// void mutex_lock(void);

// void mutex_unlock(void);


#endif



