/**
 * @file r_boot_slave.c
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
/* Includes ------------------------------------------------------------------*/
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "crc.h"
#include "r_boot_slave.h"
#include "r_boot_slave_config.h"

/* Private define ------------------------------------------------------------*/

#ifndef ENABLE_PROTOCOL_CALLBACK

    #ifndef ENABLE_AUTO_JUMP_IAP

    #define ENABLE_AUTO_JUMP_IAP

    #endif

    #define RING_BUFFER_SIZE 1
#else 
    #define RING_BUFFER_SIZE IAP_PROTOCOL_RING_BUFFER_SIZE
#endif


/* Private typedef -----------------------------------------------------------*/

/**
 * @brief 定义环节变量值联合体类型，均为4Byte长度，三种类型
 * 
 */
typedef union _env_variable_t
{
    uint32_t u32_value;
    int32_t i32_value;
    float f32_value;
    /* data */
}env_u_t;


typedef struct env_buffer
{
    volatile env_u_t env_array[variable_number+2];
}env_struct_t;

typedef struct _cbk_ctl_entry_t
{
    void *cbk_obj_list;
    int cbk_obj_count;
    int call_number;
}cbk_ctl_entry_t;





typedef struct _protocal_pkg_
{

    int data_buffer_size;
    char * data_buffer;
}protocal_frame_t;

struct ring_buffer_entry
{
    /* data */
    uint8_t ring_buffer[RING_BUFFER_SIZE];
    uint32_t head_ptr;
    uint32_t tail_ptr;
    uint32_t ring_buffer_mask;
    volatile uint8_t busy_flag;
};


typedef struct protocal_data
{
    /* data */
    struct ring_buffer_entry input_ring, output_ring;

}p_struct_t;

typedef struct __r_boots_entry
{
    p_struct_t p_entry;
    env_struct_t *env_entry;
    cbk_ctl_entry_t p_cbk_entry;
    int r_boots_mode;
}r_boots_entry;



/* Private define ------------------------------------------------------------*/

/**
 * @brief R_BOOT_VERSION version number V1.0.0.0
 */
#define R_BOOTS_VERSION_MAIN    (0x01) /*!< [31:24] main version */
#define R_BOOTS_VERSION_SUB1    (0x00) /*!< [23:16] sub1 version */
#define R_BOOTS_VERSION_SUB2    (0x00) /*!< [15:8]  sub2 version */
#define R_BOOTS_VERSION_RC      (0x00) /*!< [7:0]  release candidate */
#define R_BOOTS_VERSION         (( R_BOOTS_VERSION_MAIN << 24)\
                                |(R_BOOTS_VERSION_SUB1 << 16)\
                                |(R_BOOTS_VERSION_SUB2 << 8 )\
                                |(R_BOOTS_VERSION_RC))

#define BUSY    1
#define IDLE    0


#define R_BOOT_WAIT_MDOE        0

#define R_BOOT_CMD_MODE         1

#ifdef ENABLE_AUTO_JUMP_IAP

#define R_BOOT_AUTO_RUN_MDOE    2

#endif



#define PROTOCOL_FRAME_USEDDO(frame) do{\
    free(frame.data_buffer);\
}while(0)



/**
 * @brief 定义回调函数的描述对象
 * 
 */
#define CALLBACK_OBJECT_DEF(cbk)\
    static struct _iap_callback_object iap_cbk_obj_##cbk = {\
        .default_callback = cbk,\
        .user_callback = NULL,\
        .next = (void *)NULL,\
        .name = (char *)#cbk,\
        .type = rb_default\
    };\

/**
 * @brief 获取回调对象
 * 
 */
#define CALLBACK_OBJECT(cbk) \
    &iap_cbk_obj_##cbk



	

/* Private macro -------------------------------------------------------------*/

/**
 * @brief 
 * 1、定义枚举类型；
 * 2、声明通过字符串获取枚举变量函数；
 * 3、声明通过枚举变量获取枚举名字符串
 * enum envEnum { 
 *  current_app_length =1,
 *  current_app_crc ,
 *  current_app_ecrc ,
 *  current_app_addr ,
 *  new_app_length ,
 *  new_app_crc ,
 *  new_app_ecrc ,
 *  new_app_addr ,
 *  bootcmd ,
 *  app_max_size ,
 *  boot_address ,
 *  state_code ,
 *  variable_number =(16), }; 
 * const char *GetenvEnumString(const enum envEnum dummy); 
 * enum envEnum GetenvEnumValue(const char *string);
 */

/* Private variables ---------------------------------------------------------*/
static env_struct_t envs = {
    .env_array[r_boots_version]     = R_BOOTS_VERSION,
    .env_array[current_app_length]  = ENV_DEFAULT_VALUE,
    .env_array[current_app_crc]     = ENV_DEFAULT_VALUE,
    .env_array[current_app_ecrc]    = ENV_DEFAULT_VALUE,
    .env_array[current_app_addr]    = ENV_DEFAULT_CURRENT_APP_ADDR,
    .env_array[new_app_length]      = ENV_DEFAULT_VALUE,
    .env_array[new_app_crc]         = ENV_DEFAULT_VALUE,
    .env_array[new_app_ecrc]        = ENV_DEFAULT_VALUE,
    .env_array[new_app_addr]        = ENV_DEFAULT_NEW_APP_ADDR,
    .env_array[bootcmd]             = ENV_DEFAULT_VALUE,
    .env_array[app_max_size]        = ENV_DEFAULT_APP_MAX_SIZE,
    .env_array[boot_address]        = ENV_DEFAULT_BOOT_ADDR,
    .env_array[state_code]          = ENV_DEFAULT_VALUE,
    .env_array[delay_time]          = ENV_DEFAULT_DELAY_TIME,
    .env_array[variable_number]     = ENV_DEFAULT_VALUE,
    .env_array[crc16]               = ENV_DEFAULT_VALUE,
};

/**
 * @brief 控制结构体
 * 
 */
static r_boots_entry rb_entry;
/* Private function prototypes -----------------------------------------------*/

__weak bool r_boots_write_flash(uint32_t addr, char *src, int len)
{
    return false;
}
__weak bool r_boots_read_flash(uint32_t addr, char *dest, int len)
{
    return false;
}
__weak bool r_boots_iap_jump(uint32_t app_addr)
{
    return false;
}
__weak bool r_boots_output_stream(void *addr, int len)
{
    return false;
}


/**
 * @brief 通过宏定义实现
 * 1、通过字符串获取枚举变量函数；
 * 2、通过枚举变量获取枚举名字符串

 enum envEnum GetenvEnumValue(const char *str) 
{ 
    if(!strcmp(str,"current_app_length")) return current_app_length; 
    if(!strcmp(str,"current_app_crc")) return current_app_crc; 
    if(!strcmp(str,"current_app_ecrc")) return current_app_ecrc; 
    if(!strcmp(str,"current_app_addr")) return current_app_addr; 
    if(!strcmp(str,"new_app_length")) return new_app_length; 
    if(!strcmp(str,"new_app_crc")) return new_app_crc; 
    if(!strcmp(str,"new_app_ecrc")) return new_app_ecrc; 
    if(!strcmp(str,"new_app_addr")) return new_app_addr; 
    if(!strcmp(str,"bootcmd")) return bootcmd; 
    if(!strcmp(str,"app_max_size")) return app_max_size; 
    if(!strcmp(str,"boot_address")) return boot_address; 
    if(!strcmp(str,"state_code")) return state_code; 
    if(!strcmp(str,"variable_number")) return variable_number; 
    return (enum envEnum)0; 
}

 const char *GetenvEnumString(enum envEnum value) 
{ 
    switch(value) 
    { 
        case current_app_length: return "current_app_length"; 
        case current_app_crc: return "current_app_crc"; 
        case current_app_ecrc: return "current_app_ecrc"; 
        case current_app_addr: return "current_app_addr"; 
        case new_app_length: return "new_app_length"; 
        case new_app_crc: return "new_app_crc"; 
        case new_app_ecrc: return "new_app_ecrc"; 
        case new_app_addr: return "new_app_addr"; 
        case bootcmd: return "bootcmd"; 
        case app_max_size: return "app_max_size"; 
        case boot_address: return "boot_address"; 
        case state_code: return "state_code"; 
        case variable_number: return "variable_number"; 
        default: return ""; 
    } 
}
*/
DEFINE_ENUM(envEnum,ENV_ENUM)


/**
 * @brief 初始化环形缓存
 * 
 * @param rb_ptr 
 * @return true 
 * @return false 
 */
static bool ring_buffer_init(struct ring_buffer_entry *rb_ptr)
{
    if(rb_ptr == NULL) return false;
    memset(rb_ptr, 0, sizeof(struct ring_buffer_entry));
    rb_ptr->ring_buffer_mask = RING_BUFFER_SIZE - 1;
    rb_ptr->busy_flag = IDLE;
    rb_ptr->head_ptr = 0;
    rb_ptr->tail_ptr = 0;
    return true;
}
/**
 * @brief 
 * 
 * @param rb_ptr 
 * @return uint32_t 
 */
static inline uint32_t ring_buffer_is_full(struct ring_buffer_entry *rb_ptr)
{
    return ((rb_ptr->head_ptr - rb_ptr->tail_ptr) & rb_ptr->ring_buffer_mask) == rb_ptr->ring_buffer_mask;
}
/**
 * @brief 
 * 
 * @param rb_ptr 
 * @return uint32_t 
 */
static inline uint32_t ring_buffer_is_empty(struct ring_buffer_entry *rb_ptr)
{
    return (rb_ptr->head_ptr == rb_ptr->tail_ptr);
}
/**
 * @brief 
 * 
 * @param rb_ptr 
 * @return uint32_t 
 */
static inline uint32_t ring_buffer_num_items(struct ring_buffer_entry *rb_ptr)
{
    return ((rb_ptr->head_ptr - rb_ptr->tail_ptr) & rb_ptr->ring_buffer_mask);
}
/**
 * @brief 
 * 
 * @param rb_ptr 
 * @param data 
 */
static void ring_buffer_push(struct ring_buffer_entry *rb_ptr, char data)
{
    if(ring_buffer_is_full(rb_ptr)){
        rb_ptr->tail_ptr = ((rb_ptr->tail_ptr + 1) & rb_ptr->ring_buffer_mask);
    }
    rb_ptr->ring_buffer[rb_ptr->head_ptr]  = data;
    rb_ptr->head_ptr = ((rb_ptr->head_ptr + 1) & rb_ptr->ring_buffer_mask);
}
/**
 * @brief 
 * 
 * @param rb_ptr 
 * @param src 
 * @param len 
 */
static void ring_buffer_push_array(struct ring_buffer_entry *rb_ptr,const char *src, int len)
{
    for (int i = 0; i < len; i++)
    {
        ring_buffer_push(rb_ptr, src[i]);
    }
}
/**
 * @brief 
 * 
 * @param rb_ptr 
 * @param data 
 * @return int 0 err;1 ok;
 */
static int ring_buffer_pop(struct ring_buffer_entry *rb_ptr, char *data)
{
    if(ring_buffer_is_empty(rb_ptr)){
        return 0;
    }
    *data = rb_ptr->ring_buffer[rb_ptr->tail_ptr];
    rb_ptr->tail_ptr = (rb_ptr->tail_ptr + 1) & rb_ptr->ring_buffer_mask;
    return 1;
}
/**
 * @brief 
 * 
 * @param rb_ptr 
 * @param dest 
 * @param len 
 * @return int 0 err
 */
static int ring_buffer_pop_array(struct ring_buffer_entry *rb_ptr,char *dest, int len)
{
    if(ring_buffer_is_empty(rb_ptr)){
        return 0;
    }
    char *data_ptr = dest;
    int cnt = 0;

    while ((cnt<len) && ring_buffer_pop(rb_ptr, data_ptr))
    {
        cnt++;
        data_ptr++;
        /* code */
    }
    return cnt;
    
}
static int ring_buffer_peak_array(struct ring_buffer_entry *rb_ptr,char *dest, int index)
{
    if(index >= ring_buffer_num_items(rb_ptr)){
        return 0;
    }
    int data_index = ((rb_ptr->tail_ptr + index) & rb_ptr->ring_buffer_mask);
    *dest = rb_ptr->ring_buffer[data_index];
    return 1;
}
/**
 * @brief 保持完整性，暂不做实际工作
 * 
 * @param rb_ptr 
 * @return true 
 * @return false 
 */
static bool ring_buffer_deinit(struct ring_buffer_entry *rb_ptr)
{
    return true;
}
/**
 * @brief 进入此函数之前必须提前判断环形缓存内剩余空间是否足够
 * 
 * @param rb_ptr 
 * @param src 
 * @param len 
 */
// static void ring_buffer_mcpy(struct ring_buffer_entry *rb_ptr, const void *src, int len)
// {
//     void *u_src = (void *)src;
//     int remain_size = RING_BUFFER_SIZE - rb_ptr->tail_ptr;
//     if(remain_size >= len){
//         memcpy(&rb_ptr->ring_buffer[rb_ptr->tail_ptr], src, len);
//         rb_ptr->tail_ptr += len;
//         rb_ptr->remain_size -= len;
//     }else{
//         memcpy(&rb_ptr->ring_buffer[rb_ptr->tail_ptr], src, remain_size);
//         u_src = (void *)((uint32_t)u_src+remain_size);
//         rb_ptr->tail_ptr = 0;
//         rb_ptr->remain_size -= remain_size;
//         len -= remain_size;
//         memcpy(&rb_ptr->ring_buffer[rb_ptr->tail_ptr], u_src, len);
//         rb_ptr->tail_ptr += len;
//         rb_ptr->remain_size -= len;
//     }
// }
/**
 * @brief 进入此函数之前应该提前判断环形缓存内是否有完整帧
 * 
 * @param rb_ptr 
 * @param src 
 * @param len 
 */
// static void ring_buffer_mread(struct ring_buffer_entry *rb_ptr, void *src, int len)
// {
//     int remain_size = RING_BUFFER_SIZE - rb_ptr->head_ptr;
//     if(remain_size >= len){
//         memcpy(src, &rb_ptr->ring_buffer[rb_ptr->head_ptr], len);
//         rb_ptr->head_ptr += len;
//         rb_ptr->remain_size += len;
//     }else{
//         memcpy(src, &rb_ptr->ring_buffer[rb_ptr->head_ptr], remain_size);
// 		src = (void *)((uint32_t)src+remain_size);
//         rb_ptr->head_ptr = 0;
//         rb_ptr->remain_size += remain_size;
//         len -= remain_size;
//         memcpy(src, &rb_ptr->ring_buffer[rb_ptr->head_ptr], len);
//         rb_ptr->head_ptr += len;
//         rb_ptr->remain_size += len;
//     }
// }
/**
 * @brief 向环形缓存中进行写操作
 * 
 * @param rb_ptr 
 * @param src 
 * @param size 
 * @return true 
 * @return false 
 */
static bool ring_buffer_write(struct ring_buffer_entry *rb_ptr, const void *src, int size)
{
    //while(rb_ptr->busy_flag == BUSY);
    rb_ptr->busy_flag = BUSY;
    ring_buffer_push_array(rb_ptr, (void *)&size, sizeof(int));
    ring_buffer_push_array(rb_ptr, (void *)src, size);
    rb_ptr->busy_flag = IDLE;
    return true;
}
/**
 * @brief 从环形缓存中读
 * 
 * @param rb_ptr 
 * @param dest 使用后需要free
 * @param len 
 * @return true 
 * @return false 
 */
static bool ring_buffer_read(struct ring_buffer_entry *rb_ptr, protocal_frame_t *frame)
{
    //while(rb_ptr->busy_flag == BUSY);
    rb_ptr->busy_flag = BUSY;
    if(ring_buffer_pop_array(rb_ptr, (void *)&frame->data_buffer_size, sizeof(int)) == 0){
        rb_ptr->busy_flag = IDLE;
        return false;
    }
    frame->data_buffer = (char *)malloc(frame->data_buffer_size);
    if(ring_buffer_pop_array(rb_ptr, frame->data_buffer, frame->data_buffer_size) == 0) {
        rb_ptr->busy_flag = IDLE;
        return false;
    }
    rb_ptr->busy_flag = IDLE;
    return true;
}
/**
 * @brief 获取argc
 * 
 * @param buffer 
 * @return uint8_t 
 */
static __inline uint8_t prtcl_get_callnum(const char *buffer)
{
    
    return buffer[0];
}
/**
 * @brief 获取argc
 * 
 * @param buffer 
 * @return uint8_t 
 */
static __inline uint8_t prtcl_get_argc(const char *buffer)
{
    
    return buffer[1];
}

/**
 * @brief 获取所有字符串参数数组
 * 
 * @param buffer 
 * @param len buffer size
 * @param argv 参数向量表
 * @return true 
 * @return false 
 */
static bool prtcl_get_argv(const void *buffer, int len, char **argv)
{
    char *u_string = (char *)buffer;

    if(u_string == NULL ) return false;

    int argc = prtcl_get_argc(buffer);

    int count = 0;

    int i = 2;
	argv[0] = &u_string[i];
    while(i++ < len && count < argc){
        if(u_string[i] == '\0'){
			count++;
			if(count >= argc){
				return true;
			}else{
				argv[count] = &u_string[i+1];
			}
        }
    }

    return false;
    
}
/**
 * @brief 获取数据块 argc
 * 
 * @param buffer 
 * @param len 
 * @return __inline int 
 * -1 err
 */
static __inline int prtcl_get_argbc(const char *buffer, int len)
{
    int argc = buffer[1];
    int i = 0;
    int count = 0;
    while(++i){
        if(i > len){
            return -1;
        }
        if(buffer[i] == '\0'){
            count++;
            if(count >= argc){
                return buffer[i+1];
            }
        }
    }
    return -1;
}
/**
 * @brief 
 * 
 * @param array 
 * @return int 
 */
static int read_int32_big_endian(const char *array)
{

    int ch1 = array[0];

    int ch2 = array[1];

    int ch3 = array[2];

    int ch4 = array[3];

    return ((ch1 << 24) + (ch2 << 16) + (ch3 << 8) +(ch4 << 0));
}
/**
 * @brief 获取argbc（1,2...）个数据buffer 
 * 
 * @param buffer 
 * @param len 
 * @param argbs 
 * @param argb 
 * @return __inline int -1 error, 0 ok
 */
static __inline int prtcl_get_argb(const void *buffer, int len, char **argb)
{
    char *u_string_s = (char *)buffer;
    int argc = u_string_s[1];
    int argbc = 0;
    int index = 0;
    int count = 0;
    int argbs;
    
    while(++index){
        if(index > len){
            return -1;
        }
        if(u_string_s[index] == '\0'){
            count++;
            if(count >= argc){
                argbc = u_string_s[index+1];
				index+=2;
				break;
            }
        }
    }
    
    for (int i = 0; i < argbc; i++)
    {
        if(index > len){
            return -1;
        }
        /* code */
        argbs = read_int32_big_endian(&u_string_s[index]);
        index += 4;
        argb[i] = &u_string_s[index];
        index += argbs;
    }
    
    return 0;
}
/**
 * @brief 计算crc,暂不使用
 * 
 * @param flash_src_addr 
 * @param len 
 * @return uint16_t 0(error)
 */
 static uint16_t __r_boot_crc(uint32_t flash_src_addr, int len)
 {
     bool ret;
     char *tmp = (char *)malloc(1024);
     uint16_t crc = 0xffff;
     int remain = 0;
     int batch = 0;
     batch = len / 1024;
     remain = len % 1024;
     for (int i = 0; i < batch; i++)
     {
         ret = r_boots_read_flash(flash_src_addr + i * 1024, tmp, 1024);
         if(ret == false){
             free(tmp);
             return false;
         }
         crc = crc16_3((const uint8_t *)tmp, 1024, crc);
         /* code */
     }
     crc = crc16_3((const uint8_t *)tmp, remain, crc);

     free(tmp);

     return crc;
    
 }

/**
 * @brief 只能在注册的回调函数中调用，格式化输出数据到output ring buffer，
 * 注意一个回调函数每次运行此函数只能被执行一次
 * 整个字符串最大长度为CALLBACK_PRINTF_BUFFER_SIZE字节,可配置
 * 
 * @param cbk_p 
 * @param __format 
 * @param ... 
 * @return int 
 */
static int r_printf (struct _iap_callback_object * const cbk_p, const char *__format, ...)
{
#ifdef ENABLE_PROTOCOL_CALLBACK
    
    static char u_string[CALLBACK_PRINTF_BUFFER_SIZE];
    int ret = 0;
    int u_string_index = 0;

    u_string[0] = rb_entry.p_cbk_entry.call_number;

    u_string_index = 1;

    va_list p;
    va_start (p, __format);
    ret = vsprintf (&u_string[u_string_index], __format, p);
    va_end(p);

    if(ring_buffer_write(&rb_entry.p_entry.output_ring, u_string, ret + u_string_index + 1) == false) 
    {
        return false;
    }else{
        return ret+1;
    }

#else

    return NULL;

#endif
}

/**
 * @brief list of environment variables
 * 
 * @param cbk_p 
 */
static void env_list(struct _iap_callback_object *cbk_p)
{    
    char *u_str = NULL;
    u_str = (char *)malloc(256);
    strcpy(u_str,"env list:\r\n\t");
    for (enum envEnum i = current_app_length; i < variable_number; i++)
    {
            
        strcat(u_str, GetenvEnumString(i));
        strcat(u_str, "\r\n\t");
            
    }
    
    r_printf(cbk_p, u_str);
        
    free(u_str);
    
}

/*==================定义回调函数==============================*/
/**
 * @brief Get the all callback object
 * 获取所有已经注册的回调函数 回调号+name
 * 
 * @param argc 0
 * @param argv 0
 * @param cbk_p 
 */
static bool list_all(int argc, char **argv, struct _iap_callback_object *cbk_p)
{
    static char u_str[256] = "callback_function list:\r\n\t";

    for (struct _iap_callback_object *entry = cbk_p;  entry; entry = entry->next)
    {
        strcat(u_str, entry->name);
        strcat(u_str, "\r\n\t");
    }
    r_printf(cbk_p, u_str);
    return true;
}

CALLBACK_OBJECT_DEF(list_all)

enum param_type{
    none_t = 0,
    int_t,
    float_t,
    str_t,
};

/**
 * @brief Set the env object
 * 设置行的环境变量值，只存在ram变量中，当前有效一次
 * @param argc 
 * @param argv 
 * @param cbk_p 
 */
static bool set_env(int argc, char **argv, struct _iap_callback_object *cbk_p)
{
    if(argc == 1 && strcmp(argv[0], "-h") == 0){
        r_printf(cbk_p, "name:%s,\r\nparam_1:name/-l,\r\nparam_2:val/[]\r\n,param_3:type(1:int,2:float)/[].\r\n", cbk_p->name);
        return true;
    }
    if(argc == 1 && strcmp(argv[0], "-l") == 0){
        env_list(cbk_p);
        return true;
    }
    enum envEnum env_num = GetenvEnumValue(argv[0]);
    if(env_num == 0){
        r_printf(cbk_p, "set_env:%s error.\r\n", argv[0]);
        return false;
    }
    switch (atoi(argv[2]))
    {
    case int_t:
        envs.env_array[env_num].i32_value = atoi(argv[1]);
        r_printf(cbk_p, "set_env:%s 0X%X ok.\r\n", argv[0], envs.env_array[env_num].i32_value);
        break;
    case float_t:
        envs.env_array[env_num].f32_value = atof(argv[1]);
        r_printf(cbk_p, "set_env:%s 0X%X ok.\r\n", argv[0], envs.env_array[env_num].f32_value);
        break;
    default:
        r_printf(cbk_p, "set_env:%s error.\r\n", argv[0]);
        break;
    }
    return true;
}

/**
 * @brief 为回调函数创建一个描述结构体
 * 
 */

CALLBACK_OBJECT_DEF(set_env)

/**
 * @brief Get the env object
 * 获取环境变量值，
 * @param argc 
 * @param argv 
 * @param cbk_p 
 */
static bool get_env(int argc, char **argv, struct _iap_callback_object *cbk_p)
{
    if(argc == 1 && strcmp(argv[0], "-h") == 0){
        r_printf(cbk_p, "name:%s,\nparam_1:env_name/-l/-h,\nparam_2:type(1:int,2:float)/[]\r\n", cbk_p->name);
        return true;
    }
    if(argc == 1 && strcmp(argv[0], "-l") == 0){
        env_list(cbk_p);
        return true;
    }
    enum envEnum env_num = GetenvEnumValue(argv[0]);
    if(env_num == 0){
        r_printf(cbk_p, "get_env:%s error.\r\n", argv[0]);
        return false;
    }
    switch (atoi(argv[1]))
    {
    case int_t:
        r_printf(cbk_p, "get_env:%s 0X%X ok.\r\n", argv[0], envs.env_array[env_num].i32_value);
        break;
    case float_t:
        r_printf(cbk_p, "get_env:%s %f ok.\r\n", argv[0], envs.env_array[env_num].f32_value);
        break;
    default:
        r_printf(cbk_p, "get_env:%s error.\r\n", argv[0]);
        break;
    }
    return true;
}
CALLBACK_OBJECT_DEF(get_env)

/**
 * @brief 调用用户提供的写入函数，保存当前所有用户变量
 * 
 */
static bool save_env(int argc, char **argv, struct _iap_callback_object *cbk_p)
{
    if(argc == 1 && strcmp(argv[0], "-h") == 0){
        r_printf(cbk_p, "name:%s,no param.\r\n", cbk_p->name);
        return true;
    }
    
    rb_entry.env_entry->env_array[crc16].u32_value = \
    crc16_2((const uint8_t *)rb_entry.env_entry, sizeof(env_struct_t)-4);
    bool ret = r_boots_write_flash(FLASH_ENV_ADDRESS, (char *)rb_entry.env_entry, sizeof(env_struct_t));
    if(ret == false){
        r_printf(cbk_p, "save_env error.\r\n");
        
    }else{
        r_printf(cbk_p, "save_env ok.\r\n");
        
    }
    return ret;
    
}
/**
 * @brief 为回调函数创建一个描述结构体
 * 
 */
CALLBACK_OBJECT_DEF(save_env)


/**
 * @brief 将数据写入new app地址下
 * argv[0]:filename
 * argv[1]:filesize
 * argv[2]:Byte stream size
 * argv[3]:ftell
 * argv[4]:Byte stream (对于文件数据，完全由可能在中间出现 \0 但是我们放在最后一个参数中，已经进行了强制获取)
 */
static bool write_file(int argc, char **argv, struct _iap_callback_object *cbk_p)
{
    bool ret = false;
    if(argc == 1 && strcmp(argv[0], "-h") == 0){
        r_printf(cbk_p, "name:%s,\r\n\
        param0:file name,\r\n\
        param1:filesize,\r\n\
        param2:Byte stream size,\r\n\
        param3:ftell.\r\n\
        param4:Byte stream.\r\n\
        return:filename,ftell:\r\n", cbk_p->name);
        return true;
    }
    char *filename = argv[0];
    int file_size = atoi(argv[1]);
    int frame_size = atoi(argv[2]);
    int ftell = atoi(argv[3]);
    char *data = argv[4];
    if(strcmp("new_app.bin", filename) == 0){
        
        ret = r_boots_write_flash(envs.env_array[new_app_addr].u32_value + ftell, data, frame_size);
    }
    if(ret == true){
        ftell += frame_size;
    }
    if(ftell >= file_size){
        //更新 new APP 相关环境变量
        rb_entry.env_entry->env_array[new_app_length].u32_value = file_size;
        
        rb_entry.env_entry->env_array[new_app_crc].u32_value = crc16_2(
         (const uint8_t *)rb_entry.env_entry->env_array[new_app_addr].u32_value,
         rb_entry.env_entry->env_array[new_app_length].u32_value
        );

        rb_entry.env_entry->env_array[new_app_ecrc].u32_value = crc16_2(
         (const uint8_t *)rb_entry.env_entry->env_array[new_app_addr].u32_value,
         rb_entry.env_entry->env_array[app_max_size].u32_value
        );

    }
    r_printf(cbk_p, "%s,ftell:%d ok\r\n",filename,ftell);
    return true;
}
/**
 * @brief 为回调函数创建一个描述结构体
 * 
 */

CALLBACK_OBJECT_DEF(write_file)


static bool boot_s(int argc, char **argv, struct _iap_callback_object *cbk_p)
{
    bool ret = false;
    if(argc == 1 && strcmp(argv[0], "-h") == 0){
        r_printf(cbk_p, "name:%s,\r\n[param1]:boot addr.\r\n",\
        cbk_p->name);
        return true;
    }
    if(argc >= 1){
        ret = r_boots_iap_jump(atoi(argv[0]));
            
    }else{
        ret = r_boots_iap_jump(envs.env_array[boot_address].u32_value);

    }
    if(ret == false){
        r_printf(cbk_p, "boot error\r\n");
    }
    return ret;
}

CALLBACK_OBJECT_DEF(boot_s)

/**
 * @brief 更新当前APP文件，将new app转存到current app
 * 
 * @param argc 0
 * @param argv 0
 * @param cbk_p 
 * @return true 
 * @return false 
 */
static bool upload(int argc, char **argv, struct _iap_callback_object *cbk_p)
{
    bool ret = false;
    if(argc == 1 && strcmp(argv[0], "-h") == 0){
        r_printf(cbk_p, "name:%s,copy new app to current app\r\n[param1]:dest addr.\r\n[param2]:src addr\r\n",\
        cbk_p->name);
        return true;
    }
    uint32_t dest = NULL, src = NULL, len = 0;
    if(argc >= 1)
    {
        dest = (uint32_t)atoi(argv[0]);
    }else{
        dest = envs.env_array[current_app_addr].u32_value;
    }
    if(argc >= 2)
    {
        src = (uint32_t)atoi(argv[1]);
    }else{
        src = envs.env_array[new_app_addr].u32_value;
    }
    len = envs.env_array[app_max_size].u32_value;
    ret = r_boots_write_flash(dest,(char *)src,len);
    
    if(ret == false){
        r_printf(cbk_p, "upload error.\r\n");
    }else{
        r_printf(cbk_p, "upload ok.\r\n");
    }
    //更新 current APP CRC值
    rb_entry.env_entry->env_array[current_app_length].u32_value = \
    rb_entry.env_entry->env_array[new_app_length].u32_value;
    
    rb_entry.env_entry->env_array[current_app_crc].u32_value = crc16_2(
     (const uint8_t *)rb_entry.env_entry->env_array[current_app_addr].u32_value,
     rb_entry.env_entry->env_array[new_app_length].u32_value
    );

    rb_entry.env_entry->env_array[current_app_ecrc].u32_value = crc16_2(
     (const uint8_t *)rb_entry.env_entry->env_array[current_app_addr].u32_value,
     rb_entry.env_entry->env_array[app_max_size].u32_value
    );
    return ret;
}

CALLBACK_OBJECT_DEF(upload)

/**
 * @brief 将回调函数对象注册到对象链表中,并依次分配回调函数序号
 * 
 * @param cbk_ctl 
 * @param cbk_obj 
 */
static void register_callback_obj(cbk_ctl_entry_t* cbk_ctl, struct _iap_callback_object *cbk_obj)
{
    struct _iap_callback_object *entry = cbk_ctl->cbk_obj_list;
    for (; entry; entry = entry->next)
    {
        if(entry->next == NULL){
            entry->next = cbk_obj;

            cbk_ctl->cbk_obj_count++;
            return;
        }
        /* code */
    }
    cbk_ctl->cbk_obj_list = cbk_obj;
    cbk_ctl->cbk_obj_count++;
}
/**
 * @brief 将创建的所有回调函数对象，注册进链表中
 * 
 * @param cbk_obj_head 
 */
static void cbk_callback_init(cbk_ctl_entry_t*  cbk_ctl)
{

    register_callback_obj(cbk_ctl, CALLBACK_OBJECT(list_all));

    register_callback_obj(cbk_ctl, CALLBACK_OBJECT(set_env));

    register_callback_obj(cbk_ctl, CALLBACK_OBJECT(get_env));

    register_callback_obj(cbk_ctl, CALLBACK_OBJECT(save_env));
    
    register_callback_obj(cbk_ctl, CALLBACK_OBJECT(write_file));

    register_callback_obj(cbk_ctl, CALLBACK_OBJECT(boot_s));

    register_callback_obj(cbk_ctl, CALLBACK_OBJECT(upload));



}
/**
 * @brief 处理输入参数执行回调
 * 
 * @param cbk_ctl 回调函数模块控制块
 * @param argc 参数count 
 * @param argv 参数向量表
 * @example argc = 2  argv[0] = "name" argv[1] = "参数1"
 * @return 
 */
static bool cbk_callback_run(cbk_ctl_entry_t * const cbk_ctl, int argc, char **argv)
{
    struct _iap_callback_object *entry = cbk_ctl->cbk_obj_list;
    char *call_name = argv[0];
    for (; entry; entry = entry->next)
    {

        if(call_name != NULL && strcmp(entry->name, call_name) == 0){
            break;
        }

        /* code */
    }

    if(entry == NULL) return false;

    switch (entry->type)
    {
    case rb_default:
        if(entry->default_callback)
        {
            return entry->default_callback(argc-1, &argv[1], entry);
        }else
        {
            return false;
        }
    case rb_user:
        if(entry->user_callback)
        {
            return entry->user_callback(argc, argv);
        }else
        {
            return false;
        }
    default:
        break;
    }
    return false;
    
}
/* Public macro --------------------------------------------------------------*/


/* Public variables ----------------------------------------------------------*/


/* Public function prototypes ------------------------------------------------*/


/**
 * @brief 初始化r_boot相关数据结构
 * 
 * @param r_boot_write_flash 写数据到flash的
 * @param r_boot_read_flash 从flash读数据的
 * @param r_boot_iap_jump 跳转API，如果为NULL，则使用默认的API
 * @param r_boot_output_stream 数据输出的API
 * @return true 
 * @return false 
 */
bool r_boots_init(void)
{
    

    ring_buffer_init(&rb_entry.p_entry.input_ring);

    ring_buffer_init(&rb_entry.p_entry.output_ring);

    rb_entry.p_cbk_entry.cbk_obj_count = 0;
    rb_entry.p_cbk_entry.cbk_obj_list = NULL;


    cbk_callback_init(&rb_entry.p_cbk_entry);

    
    rb_entry.env_entry = &envs;

    env_struct_t *temp_env = (env_struct_t *)malloc(sizeof(env_struct_t));

    r_boots_read_flash(FLASH_ENV_ADDRESS, (char *)temp_env, sizeof(env_struct_t));

    if(temp_env->env_array[crc16].u32_value == \
    crc16_2((const uint8_t *)temp_env, sizeof(env_struct_t)-4)){

        memcpy(rb_entry.env_entry, temp_env, sizeof(env_struct_t));

    }
    
    free(temp_env);

    rb_entry.r_boots_mode = R_BOOT_WAIT_MDOE;
    //计算current APP CRC，new APP CRC
    if(rb_entry.env_entry->env_array[current_app_length].i32_value != ENV_DEFAULT_VALUE){

        //1 计算current APP CRC
       rb_entry.env_entry->env_array[current_app_crc].u32_value = crc16_2(
        (const uint8_t *)rb_entry.env_entry->env_array[current_app_addr].u32_value,
        rb_entry.env_entry->env_array[current_app_length].u32_value
       );
    }
    if(rb_entry.env_entry->env_array[new_app_length].i32_value != ENV_DEFAULT_VALUE){

        //1 计算new APP CRC 
        rb_entry.env_entry->env_array[new_app_crc].u32_value = crc16_2(
        (const uint8_t *)rb_entry.env_entry->env_array[new_app_addr].u32_value,
        rb_entry.env_entry->env_array[new_app_length].u32_value
       );
    
    }
    // 计算current APP ECRC
    rb_entry.env_entry->env_array[current_app_ecrc].u32_value = crc16_2(
        (const uint8_t *)rb_entry.env_entry->env_array[current_app_addr].u32_value,
        rb_entry.env_entry->env_array[app_max_size].u32_value
    );

    // 计算new APP ECRC
    rb_entry.env_entry->env_array[new_app_ecrc].u32_value = crc16_2(
        (const uint8_t *)rb_entry.env_entry->env_array[new_app_addr].u32_value,
        rb_entry.env_entry->env_array[app_max_size].u32_value
    );

    return true;
}


#ifdef ENABLE_PROTOCOL_CALLBACK
/**
 * @brief 将数据帧传入到协议栈内部
 * 
 * @param entry 
 * @param str 
 * @param len 
 */
void r_boots_input(const void *str, int len)
{
    ring_buffer_write(&rb_entry.p_entry.input_ring, str, len);
}
/**
 * @brief 直接调用函数
 * 
 * @param name 函数注册名
 * @param argc 参数个数
 * @param argv 参数字符串数组
 */
bool r_boots_call(const void *name, int argc, char **argv)
{
    return false;
}
#else
/**
 * @brief 将数据帧传入到协议栈内部
 * 
 * @param entry 
 * @param str 
 * @param len 
 */
void r_boots_input(const void *str, int len)
{
    
}
/**
 * @brief 直接调用函数
 * 
 * @param name 函数注册名
 * @param argc 参数个数
 * @param argv 参数字符串数组
 */
bool r_boot_call(const void *name, int argc, char **argv)
{
    return cbk_callback_run(rb_entry.p_cbk_entry, name, argc, argv);
}

#endif


/**
 * @brief 只能在用户回调函数中被调用，且只能一次
 * 
 * @param extern_str 额外的字符串数据，添加到应答帧的首部
 * @param format 
 * @param ... 
 * @return int 
 */
int r_boots_ckb_printf(char *extern_str, const char * format, ...)
{
#ifdef ENABLE_PROTOCOL_CALLBACK
    static char u_string[CALLBACK_PRINTF_BUFFER_SIZE];
    int ret = 0;
    int u_string_index = 0;

    u_string[0] = rb_entry.p_cbk_entry.call_number;
    if(extern_str){
        strcpy(&u_string[1], extern_str);
        u_string_index = strlen(u_string)+2;
        u_string[u_string_index-1] = '\0';
    }else{
        u_string_index = 1;
    }
    va_list p;
    va_start (p, format);
    ret = vsprintf (&u_string[u_string_index], format, p);
    va_end(p);
    r_boots_output_stream(u_string,ret + u_string_index + 1);

    return ret;
#else

    return NULL;

#endif
}
/**
 * @brief 注册用户回调对象
 * 
 * @param cbk_obj 
 */
void r_boots_new_cbk_obj(struct _iap_callback_object *cbk_obj)
{
    register_callback_obj(&rb_entry.p_cbk_entry, cbk_obj);
}
/**
 * @brief 协议栈解析数据，并执行
 * 
 * @param time 系统时间，单位ms
 */
void r_boots_run(const uint32_t time)
{
    protocal_frame_t frame;
    bool ret = false;
    int argc = 0;
    char **argv = NULL;

    int argbc = 0;

    switch (rb_entry.r_boots_mode)
    {
    case R_BOOT_WAIT_MDOE:
    case R_BOOT_CMD_MODE:

        ret = ring_buffer_read(&rb_entry.p_entry.input_ring, &frame);
        if(ret == true){

            if(rb_entry.r_boots_mode == R_BOOT_WAIT_MDOE 
            && time / 1000 < envs.env_array[delay_time].i32_value){
                
                rb_entry.r_boots_mode = R_BOOT_CMD_MODE;
            }
            
        }else{
            if(rb_entry.r_boots_mode == R_BOOT_WAIT_MDOE 
            && time / 1000 > envs.env_array[delay_time].i32_value){

#ifdef R_BOOT_AUTO_RUN_MDOE

                rb_entry.r_boots_mode = R_BOOT_AUTO_RUN_MDOE;
#else
                rb_entry.r_boots_mode = R_BOOT_CMD_MODE;
#endif
            }
            break;
        }
        rb_entry.p_cbk_entry.call_number = prtcl_get_callnum(frame.data_buffer);
        argc = prtcl_get_argc(frame.data_buffer);
        
        argbc = prtcl_get_argbc(frame.data_buffer, frame.data_buffer_size);

        argv = (char **)malloc((argc + argbc) * sizeof(char *));

        prtcl_get_argv(frame.data_buffer, frame.data_buffer_size, argv);

        prtcl_get_argb(frame.data_buffer, frame.data_buffer_size, &argv[argc]);

        ret = cbk_callback_run(&rb_entry.p_cbk_entry, argc + argbc, argv);
        
        free(argv);

        PROTOCOL_FRAME_USEDDO(frame);

        break;

#ifdef R_BOOT_AUTO_RUN_MDOE

    case R_BOOT_AUTO_RUN_MDOE:
        argc = 1;
        argv[0] = "boot_s";
        ret = cbk_callback_run(&rb_entry.p_cbk_entry, argc, argv);
        rb_entry.r_boots_mode = R_BOOT_CMD_MODE;
        break;
#endif

    default:
        break;
    }


    ret = ring_buffer_read(&rb_entry.p_entry.output_ring, &frame);

    if(ret == false) return;


    r_boots_output_stream( frame.data_buffer, frame.data_buffer_size);
    

    PROTOCOL_FRAME_USEDDO(frame);


}



