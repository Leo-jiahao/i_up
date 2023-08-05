/**
 * @file r_boot_slave.h
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
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __R_BOOT_SLAVE_H__
#define __R_BOOT_SLAVE_H__

/* Includes ------------------------------------------------------------------*/

#include "r_boot_slave_config.h"

/**
 * @brief 定义回调对象的类型
 * 
 */
typedef enum callback_object_type
{
    rb_default = 0,
    rb_user = 1,
}cbk_obj_t;
/**
 * @brief 定义回调函数描述结构体
 * 描述回调函数的
 * 回调号
 * 输入参数个数、
 * 参数类型数组
 * 
 */
struct _iap_callback_object
{
    /**
     * @brief argv[0] 参数1 
     *        ...
     */
    bool (*default_callback)(int argc, char **argv, struct _iap_callback_object *cbk_p);
    /**
     * @brief argv[0] 函数name 
     *        argv[1] 参数1
     *        ...
     */
    bool (*user_callback)(int argc, char **argv);
    struct _iap_callback_object *next;
    char *name;
    cbk_obj_t type;
};

/**
 * @brief 定义回调函数的描述对象
 * 
 */
#define R_BOOTS_CALLBACK_OBJDEF(cbk)\
    struct _iap_callback_object iap_cbk_obj_##cbk = {\
        .default_callback = NULL,\
        .user_callback = cbk,\
        .next = (void *)NULL,\
        .name = (char *)#cbk,\
        .type = rb_user\
    };\

/**
 * @brief 获取回调对象
 * 
 */
#define R_BOOTS_CALLBACK_OBJ(cbk) \
    &iap_cbk_obj_##cbk

#ifndef ENV_ENUM 

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

DECLARE_ENUM(envEnum,ENV_ENUM)

/**
 * @brief api描述结构体
 * 
 */

bool r_boots_init(void);




void r_boots_input(const void *str, int len);


bool r_boots_call(const void *name, int argc, char **argv);

void r_boots_new_cbk_obj(struct _iap_callback_object *cbk_obj);

int r_boots_ckb_printf(char *extern_str, const char * format, ...);

void r_boots_run(const uint32_t time);




#endif /* __R_BOOT_H__ */
