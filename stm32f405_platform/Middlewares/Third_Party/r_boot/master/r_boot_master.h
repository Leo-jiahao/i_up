/**
 * @file r_boot_master.h
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
#ifndef __R_BOOT_MASTER_H__
#define __R_BOOT_MASTER_H__

#ifdef __cplusplus
    extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>


typedef void (*EnterMutex_t)(void);


typedef void (*LeaveMutex_t)(void);


typedef enum __line_state_t
{
    line_none = 0,
    line_sending,
    line_wait,
    line_succeed,
    line_timeout,
}line_state_t;


enum param_type{
    none_t = 0,
    int_t,
    float_t,
};
enum ftp_flag_t{
    ftp_none = 0,
    ftp_ing,
    ftp_error,
    ftp_succeed,
};
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
/**
 * @brief 请求回调函数，正常情况应该在调用释放请求资源，以结束此次请求
 * 
 */
typedef void (*request_callback_t)(int rd,char *buffer, int buffer_size, int line_index, line_state_t line_state);

typedef bool (*r_bootm_output_stream)(char * buffer, int buffer_size);

typedef void (*r_bootm_log)(char * buffer, int buffer_size);


int r_bootm_session_init(r_bootm_output_stream output_stream, 
r_bootm_log session_log,
EnterMutex_t u_EnterMutex,LeaveMutex_t u_LeaveMutex);

void r_bootm_despatch( int rd, char *buffer, int size);

void r_bootm_timer_handler(int msecs);

//sync requestion
int r_bootm_request(int rd, int argc, char **argv, int argbc, int *argbs,char **argbv, request_callback_t cbk, int type, int timeout);

int r_bootm_request_by_string_async(int rd, char *string, request_callback_t cbk);

char * r_bootm_list_all(int rd, int* ret_size, int timeout);

bool r_bootm_set_env(int rd, char *env_name, env_u_t *value,enum param_type value_type, int timeout);

bool r_bootm_get_env(int rd, char *env_name, env_u_t *value,enum param_type value_type, int timeout);

bool r_bootm_save_env(int rd, int timeout);

void r_bootm_boot_s(int rd, int boot_addr);

bool r_bootm_upload(int rd,  int dest, int src, int timeout);



//FTP
enum ftp_flag_t r_bootm_ftp_state(int rd);

bool r_bootm_ftp_sync(int rd, char *filename, char *file_buffer, int size, int timeout_ms);

bool r_bootm_ftp_async(int rd, 
char *filename, 
char *file_buffer, 
int size, 
void (*ok_callback)(void), 
void (*error_callback)(void));

#ifdef __cplusplus
    }
#endif  /* __cplusplus */


#endif /* __R_BOOTM_H__ */

