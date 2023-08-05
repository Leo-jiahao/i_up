/**
 * @file r_boot_master.c
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
#include "r_boot_master_config.h"
#include "r_boot_master.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#define R_BOOTM_NOFTP   0
#define R_BOOTM_FTP     1

#define R_BOOTM_LINE_ASYNC   0

#define R_BOOTM_LINE_SYNC    1



#define REQUEST_SYNC_LINE_START_INDEX  (REQUEST_ASYNC_MAX_BUM)

#define REQUEST_LINE_MAX_BUM     (REQUEST_ASYNC_MAX_BUM+REQUEST_SYNC_MAX_BUM)

typedef enum __session_state_t
{
    s_none = 0,
    s_connected,
    s_disconnected,

}session_state_t;



typedef struct _r_bootm_send_line_t
{
    int line_type;

    /*timeout的触发时间*/
    unsigned long trigger_time;

    //传输线index
    uint8_t line_index;

    //当前传输请求的一类参数个数
    uint8_t argc;

    //当前传输请求的二类参数个数
    uint8_t argbc;
    //当前传输请求的二类参数size向量组
    int argbs[ARG_COUNT_MAX];

    char *buffer;

    int buffer_size;
    //当前传输请求状态
    volatile line_state_t state;

    //请求的回调函数
    request_callback_t line_callback;

    char *sync_buffer;
    int sync_buffer_size;

}r_bootm_send_line_t,line_t;


/**
 * @brief ftp struct
 * 
 */
typedef struct _r_bootm_ftp_t {
    enum ftp_flag_t ftp_flag;
    char filename[FTP_FILE_NAME_MAX_LENGTH];
    char *filebuffer;
    int filesize;
    int f_tell;
    int pre_block_size;
    void (*ftp_ok_callback)(void);
    void (*ftp_error_callback)(void);
}r_bootm_ftp_t, ftp_t;

/**
 * @brief 描述设备连接
 * 
 */
typedef struct _r_bootm_session_t
{
    /* data */
    //会话链表
    struct _r_bootm_session_t *next;

    //当前会话的发送接口
    r_bootm_output_stream r_bootm_send_api;
    
    //描述符
    int rd;

    //请求数组
    r_bootm_send_line_t r_line[REQUEST_LINE_MAX_BUM];
    
    //会话状态
    session_state_t s_state;

    //会话的维持时间 ms
    unsigned long real_time;

    //save r_boot_slave information(version etc.)
    char slave_info[128];

    //print  callback, show all ack data from slave 
    r_bootm_log ack_log;
    
    r_bootm_ftp_t ftp;

    EnterMutex_t EnterMutex;
    LeaveMutex_t LeaveMutex;

}r_bootm_session_t, session_t;

#if R_BOOT_MASTER_LOG_ENABLED == 1
#define RBM_LOG(...)   printf(__VA_ARGS__)
#else
#define RBM_LOG(...)   
#endif

static session_t *r_bootm_session = NULL;


/**
 * @brief Get the args object,while change delim in string
 *  to '\0',
 * @param string would not be const
 * @param argv should be a char* array,length  ARG_COUNT_MAX
 * @return int argc
 */
static int get_args(char *string, char **argv) 
{
    char *p;
    p = strtok(string," ");
    int argc = 0;
    while (p)
    {
        /* code */
        argv[argc] = p;

        argc++;
        
        p = strtok(NULL," ");
    
    }
    return argc;
}
/**
 * @brief 
 * 
 * @param v 
 * @param array 
 */
static void write_int32_big_endian(int v, char *array)
{
    array[0] = (v >> 24) & 0xFF;
    array[1] = (v >> 16) & 0xFF;
    array[2] = (v >> 8) & 0xFF;
    array[3] = (v) & 0xFF;
}
/**
 * @brief 
 * 
 * @param array 
 * @return int 
 */
static int read_int32_big_endian(char *array)
{
    int ch1 = array[0];
    int ch2 = array[1];
    int ch3 = array[2];
    int ch4 = array[3];
    return ((ch1 << 24) + (ch2 << 16) + (ch3 << 8) +(ch4 << 0));
}
/**
 * @brief release line buffer and state
 * 
 * @param rd 
 * @param line_index 
 * @return bool 
 */
static bool __r_bootm_release_line(line_t *line)
{
    if(line == NULL){
        return false;
    }
    
    line->line_callback = NULL;
    line->state = line_none;
    if(line->buffer){
        free(line->buffer);
        line->buffer = NULL;
    }
    
    if(line->line_type == R_BOOTM_LINE_SYNC && line->sync_buffer_size && line->sync_buffer){
        free(line->sync_buffer);
        line->sync_buffer_size = 0;
        line->sync_buffer = NULL;
    }
    RBM_LOG("in session.release line:%d\n", line->line_index);
    return true;
}
/**
 * @brief 
 * 
 * @param rd 
 * @return session_t* 
 */
static session_t *__get_session_from_rd(int rd)
{
    session_t *entry = r_bootm_session;

    for (; entry; entry = entry->next)
    {
        if(entry->rd == rd){
            break;
        }
        /* code */
    }
    return entry;
}

/**
 * @brief 
 * 
 * @param rd 
 * @param line_idnex 
 * @return line_t* 
 */
static line_t *__get_line_from_rd_line_index(int rd, int line_idnex)
{
    session_t *entry = __get_session_from_rd(rd);

    if(entry == NULL || line_idnex >= REQUEST_LINE_MAX_BUM){
        return NULL;
    }
    
    return &entry->r_line[line_idnex];
}
/**
 * @brief 
 * 
 * @param entry 
 * @param line_idnex 
 * @return line_t* 
 */
static line_t *__get_line_from_session_line_index(session_t *entry, int line_idnex)
{
    if(entry == NULL || line_idnex >= REQUEST_LINE_MAX_BUM){
        return NULL;
    }
    
    return &entry->r_line[line_idnex];
}

/**
 * @brief 
 * 
 * @param timeout 
 */
static void r_bootm_delay(int timeout)
{
    R_BOOTM_DELAY_MS(timeout);
}

/**
 * @brief release line buffer and state
 * 
 * @param rd 
 * @param line_index 
 * @return bool 
 */
bool r_bootm_release_line(int rd, int line_index)
{
    line_t *line = __get_line_from_rd_line_index(rd,line_index);
    if(line == NULL){
        return false;
    }

    return __r_bootm_release_line(line);
}
/**
 * @brief 创建
 * 
 * @param output_stream ,
 * @return int rd 描述符
 */
int r_bootm_session_init(r_bootm_output_stream output_stream, 
r_bootm_log session_log,
EnterMutex_t u_EnterMutex,LeaveMutex_t u_LeaveMutex)
{
    session_t *entry = r_bootm_session;
    for (; entry; entry = entry->next)
    {
        if(entry->r_bootm_send_api == output_stream){
            return entry->rd;
        }

        if(entry->next == NULL){
            entry->next = (session_t *)malloc(sizeof(session_t));
            if(entry->next == NULL){
                RBM_LOG("malloc a session entry error\n");
                return 0;
            }else{
                entry = entry->next;
                memset(entry, 0, sizeof(session_t));
                entry->r_bootm_send_api = output_stream;
                entry->ack_log = session_log;
                entry->EnterMutex = u_EnterMutex;
                entry->LeaveMutex = u_LeaveMutex;
                for( int i = REQUEST_SYNC_LINE_START_INDEX; i < REQUEST_LINE_MAX_BUM; i++){
                    entry->r_line[i].line_type = R_BOOTM_LINE_SYNC;
                }
                
                entry->rd = (int)entry;
                return entry->rd;
            }
        }
        /* code */
    }
    RBM_LOG("r_bootm create first session entry\n");
    r_bootm_session = (session_t *)malloc(sizeof(session_t));
    if(r_bootm_session == NULL)
    {
        RBM_LOG("malloc a session entry error\n");
        return 0;
    }
    memset(r_bootm_session, 0, sizeof(session_t));
    r_bootm_session->EnterMutex = u_EnterMutex;
    r_bootm_session->LeaveMutex = u_LeaveMutex;
    r_bootm_session->rd = (int)r_bootm_session;
    r_bootm_session->r_bootm_send_api = output_stream;
    r_bootm_session->ack_log = session_log;
    for( int i = REQUEST_SYNC_LINE_START_INDEX; i < REQUEST_LINE_MAX_BUM; i++){
        r_bootm_session->r_line[i].line_type = R_BOOTM_LINE_SYNC;
    }
    return r_bootm_session->rd;
}
/**
 * @brief 数据输入到协议栈
 * should mutex with r_bootm_output_stream
 * @param rd 
 * @param buffer 
 * @param size 包括 \0
 * @return int 
 */
void r_bootm_despatch( int rd, char *buffer, int size)
{
    session_t *entry = __get_session_from_rd(rd);
    if(entry == NULL || buffer[0] >= REQUEST_LINE_MAX_BUM){
        return;
    }
    
    RBM_LOG("in session:%ul.find line:%d\n", entry->rd, buffer[0]);

    line_t *line = &entry->r_line[buffer[0]];
    if(entry->EnterMutex) entry->EnterMutex();
    if(line->state != line_wait){
        RBM_LOG("in session:%ul.find line:%d, state not wait(2):%d\n", entry->rd, buffer[0],line->state);
        if(entry->LeaveMutex) entry->LeaveMutex();
        return;
    }
    if(line->line_type == R_BOOTM_LINE_ASYNC){
        if(line->line_callback){
            line->state = line_succeed;
            line->line_callback(rd, &buffer[1], size-1, buffer[0],line->state);
            RBM_LOG("in session:%ul.find async line:%d, callback:finished\n", entry->rd, buffer[0]);
        }else{
            
            __r_bootm_release_line(line);
            RBM_LOG("in session:%ul.find async line:%d, no callback,force release\n", entry->rd, buffer[0]);
        }
    }else if(line->line_type == R_BOOTM_LINE_SYNC){

        RBM_LOG("in session:%ul.find sync line:%d, get ack data.\n", entry->rd, buffer[0]);
        line->sync_buffer_size = size;
        line->sync_buffer = malloc(size);
        memcpy(line->sync_buffer, buffer, size);
        line->state = line_succeed;

    }
    
    if(r_bootm_session->ack_log){
        r_bootm_session->ack_log(buffer, size);
    }
    if(entry->LeaveMutex) entry->LeaveMutex();
    return;
}

/**
 * @brief find target session, get a line to send a request by argc argv and argbc argbs argbv
 * 
 * @param rd 
 * @param argc 
 * @param argv 
 * @param argbc 
 * @param argbs 
 * @param argbv 
 * @param cbk NULL:no callback when slave ack 
 * @param type 0 async;1 sync
 * @param timeout s when sync valid
 * @return int line_index
 * err -1
 * no cbk oxff
 */
int r_bootm_request(int rd, int argc, char **argv, int argbc, int *argbs,char **argbv, request_callback_t cbk, int type, int timeout)
{
    session_t *entry = __get_session_from_rd(rd);

    if(entry == NULL){
        return -1;
    }
    line_t *line = NULL;
    int s,e;
    if(type == R_BOOTM_LINE_ASYNC){
        s = 0;
        e = REQUEST_ASYNC_MAX_BUM;
        if(cbk == NULL){
            return -1;
        }
    }else{
        
        s = REQUEST_SYNC_LINE_START_INDEX;
        e = REQUEST_LINE_MAX_BUM;
        if(cbk){
            return -1;
        }
    }
    for (int i = s; i < e; i++)
    {
        /* code */
        if(entry->r_line[i].state == line_none){
            line = &entry->r_line[i];
            line->line_index = i;
            break;
        }
    }

    if(line == NULL){
            
        RBM_LOG("in session:%ul.request a line error.\n",entry->rd);
        return -1;
    }

    RBM_LOG("in session:%ul.request a send line ok:%d.\n",entry->rd,line->line_index);

    line->argc = argc;

    line->argbc = argbc;

    memcpy(line->argbs, argbs,sizeof(int)*argbc);

    line->buffer_size = 1+1;//call num(1byte) + argc(1byte)
    for (int i = 0; i < argc; i++)
    {
        line->buffer_size += strlen(argv[i])+1;
    }
    line->buffer_size += 1;//argb num(1byte)
    for (int i = 0; i < argbc; i++)
    {
        line->buffer_size += sizeof(int);
        line->buffer_size += argbs[i];
    }
    line->buffer = (char *)malloc(line->buffer_size);
    if(line->buffer == NULL){
        RBM_LOG("in session:%ul.malloc data to line error.\n",entry->rd);
        line->state = line_none;
        return -1;
    }
    //write data to buffer
    RBM_LOG("in session:%ul.write data to line.\n",entry->rd);
    int byte_index = 0;
    line->buffer[byte_index] = line->line_index;
    byte_index++;
    line->buffer[byte_index] = line->argc;
    byte_index++;
    for (int i = 0; i < argc; i++)
    {
        strcpy(&line->buffer[byte_index], argv[i]);
        byte_index += strlen(argv[i])+1;
    }
    line->buffer[byte_index] = line->argbc;
    byte_index++;
    for (int i = 0; i < argbc; i++)
    {   
        write_int32_big_endian(argbs[i], &line->buffer[byte_index]);
        byte_index+=4;

        memcpy(&line->buffer[byte_index],argbv[i],argbs[i]);
        byte_index += argbs[i];
    }

    bool ret = false;

    RBM_LOG("in session:%ul.start transmit.\n",entry->rd);
    if(entry->EnterMutex) entry->EnterMutex();
    line->state = line_sending;
    RBM_LOG("in session:%ul.transmit buffer size:%d.\n",entry->rd,line->buffer_size);
    ret = entry->r_bootm_send_api(line->buffer, line->buffer_size);

    if(ret == true){
        line->state = line_wait;
    }else{
        RBM_LOG("in session:%ul.transmit err.\n",entry->rd);
        line->state = line_none;
        free(line->buffer);
        if(entry->LeaveMutex) entry->LeaveMutex();
        return -1;
    }
    
    line->line_callback = cbk;
    if(type == R_BOOTM_LINE_ASYNC){
        line->trigger_time = entry->real_time + REQUEST_TIMEOUT_S*1000;
        
    }else{
        line->trigger_time = entry->real_time + timeout*1000;
    }
    if(entry->LeaveMutex) entry->LeaveMutex();
    
    RBM_LOG("in session:%ul.start transmit finished, waiting...\n",entry->rd);
    return line->line_index;
}

/**
 * @brief only argc and argv in string, no argbc 
 * you can do something about slave ack in cbk 
 * @param rd 
 * @param string example:"list_all -h","set_env -h",
 * @param cbk 
 * @return int 
 */
int r_bootm_request_by_string_async(int rd, char *string, request_callback_t cbk)
{
    char *argv[ARG_COUNT_MAX];
    int argc = 0;
    int line_index;
    argc = get_args(string,argv);
    line_index = r_bootm_request(rd,argc, argv, 0, NULL, NULL, cbk, R_BOOTM_LINE_ASYNC, 0);
    if(line_index == -1){
        RBM_LOG("in session:%ul.request string error.\n",rd);
    }
    return line_index;
}

/**
 * @brief 
 * 
 * @param rd 
 * @param argc 
 * @param argv 
 * @param argbc 
 * @param argbs 
 * @param argbv 
 * @param ack_size 
 * @param delay_ms 
 * @param timeout s
 * @return char* 
 */
char * r_bootm_request_sync(int rd, 
int argc, 
char **argv, 
int argbc, 
int *argbs,
char **argbv, 
int * ack_size, 
void (*delay_ms)(int), int timeout)
{
    
    int line_index;
    char *ack_buffer = NULL;

    if(argc <= 0 || delay_ms == NULL || argbc < 0){
        return NULL;
    }
    line_index = r_bootm_request(rd,argc, argv, argbc, argbs, argbv, NULL, R_BOOTM_LINE_SYNC, timeout);
    if(line_index < 0){
        RBM_LOG("in session:%ul.request string error.\n",rd);
        return NULL;
    }
    line_t *line = __get_line_from_rd_line_index(rd, line_index);
    while(1){
        delay_ms(1);
        if(line->state == line_succeed){
            ack_buffer = malloc(line->sync_buffer_size-1);
            if(ack_buffer == NULL){
                RBM_LOG("in session:%ul.request string malloc ack buffer err.\n",rd);
                return NULL;
            }
            memcpy(ack_buffer, &line->sync_buffer[1], line->sync_buffer_size-1);
            *ack_size = line->sync_buffer_size-1;
            RBM_LOG("in session:%ul.request string succeed.\n",rd);
            break;
        }else if(line->state == line_timeout){
            RBM_LOG("in session:%ul.request string timeout.\n",rd);
            break;
        }else{

        }
    }
    __r_bootm_release_line(line);

    return ack_buffer;
}

/**
 * @brief example:"list_all -h","set_env -h",
 * 
 * @param rd 
 * @param string 
 * @param ack_size 
 * @param delay_ms 
 * @param timeout block time ms
 * @return char* * should be free
 */
char * r_bootm_request_by_string_sync(int rd, char *string, int * ack_size, void (*delay_ms)(int), int timeout)
{
    char *argv[ARG_COUNT_MAX];
    int argc = 0;
    int line_index;
    char *ack_buffer = NULL;
    argc = get_args(string,argv);
    if(argc <= 0 || delay_ms == NULL || string == NULL){
        return NULL;
    }
    return r_bootm_request_sync(rd, argc, argv,0,NULL,NULL,ack_size,delay_ms, timeout);

}

/**
 * @brief 
 * 
 * @param msecs 距离上一次进入的时间跨度
 */
void r_bootm_timer_handler(int msecs)
{
    session_t *entry = r_bootm_session;

    for (; entry; entry = entry->next)
    {
        entry->real_time += msecs;
        for (int i = 0; i < REQUEST_ASYNC_MAX_BUM; i++)
        {
            /* code */
            if(entry->r_line[i].state == line_wait && entry->real_time >= entry->r_line[i].trigger_time ){
                entry->r_line[i].line_callback(entry->rd, NULL, 0, i, line_timeout);
                
            }
        }
        for (int i = REQUEST_ASYNC_MAX_BUM; i < REQUEST_LINE_MAX_BUM; i++)
        {
            /* code */
            if(entry->r_line[i].state == line_wait && entry->real_time >= entry->r_line[i].trigger_time ){
                entry->r_line[i].state = line_timeout;
            }
        }
        /* code */
    }
    
}

//============================default sync requestions============================
/**
 * @brief 
 * 
 * @param rd 
 * @param ret_size 
 * @param timeout 
 * @return char* should be freed 
 */
char * r_bootm_list_all(int rd, int* ret_size, int timeout)
{
    return r_bootm_request_by_string_sync(rd,"list_all", ret_size,r_bootm_delay, timeout);
}
/**
 * @brief 
 * 
 * @param rd 
 * @param env_name 
 * @param value 
 * @param value_type 
 * @param timeout 
 * @return true 
 * @return false 
 */
bool r_bootm_set_env(int rd, char *env_name, env_u_t *value,enum param_type value_type,int timeout)
{
    char u_string[256] = {0,};
    char *ack_buffer;
    int ack_buffer_size;
    switch (value_type)
    {
    case int_t:
        /* code */
        sprintf(&u_string[strlen(u_string)],"set_env %s %d %d",env_name , value->i32_value,value_type);
        break;
    case float_t:

        sprintf(&u_string[strlen(u_string)],"set_env %s %d %d",env_name , value->f32_value,value_type);
        /* code */
        break;
    default:
        return false;
    }
    ack_buffer = r_bootm_request_by_string_sync(rd,u_string, &ack_buffer_size,r_bootm_delay, timeout);
    if(ack_buffer == NULL){
        return false;
    }
    if(strstr(ack_buffer,"ok")!=NULL){
        free(ack_buffer);
        return true;
    }else{
        free(ack_buffer);
        return false;
    }
}
/**
 * @brief 
 * 
 * @param rd 
 * @param env_name 
 * @param value 
 * @param value_type 
 * @param timeout 
 * @return true 
 * @return false 
 */
bool r_bootm_get_env(int rd, char *env_name, env_u_t *value,enum param_type value_type, int timeout)
{
    char u_string[256] = {0,};
    char *ack_buffer;
    int ack_buffer_size;
    if(env_name == NULL || value_type != int_t|| value_type != float_t){
        return false;
    }
    sprintf(u_string,"get_env %s %d",env_name,value_type);
    
    ack_buffer = r_bootm_request_by_string_sync(rd,u_string, &ack_buffer_size,r_bootm_delay, timeout);
    if(ack_buffer == NULL){
        return false;
    }

    if(strstr(ack_buffer,"ok")!=NULL){
        switch (value_type)
        {
        case int_t:
            /* code */
            sscanf(ack_buffer,"get_env:%s 0X%X ok.\r\n",u_string,&value->i32_value);
            break;
        case float_t:
            /* code */
            sscanf(ack_buffer,"get_env:%s %f ok.\r\n",u_string,&value->f32_value);
            break;
        default:
            free(ack_buffer);
            return false;
        }
        
        free(ack_buffer);
        return true;
    }

    free(ack_buffer);
    return false;
    
}
/**
 * @brief 
 * 
 * @param rd 
 * @param ret_size 
 * @param timeout 
 * @return true 
 * @return false 
 */
bool r_bootm_save_env(int rd, int timeout)
{
    char u_string[256] = {0,};
    char *ack_buffer;
    int ack_buffer_size;
    strcpy(u_string,"save_env");

    ack_buffer = r_bootm_request_by_string_sync(rd,u_string, &ack_buffer_size,r_bootm_delay, timeout);
    if(ack_buffer == NULL){
        return false;
    }

    if(strstr(ack_buffer,"ok")!=NULL){
        free(ack_buffer);
        return true;
    }
    
    free(ack_buffer);
    return false;
}
/**
 * @brief 
 * 
 * @param rd 
 * @param boot_addr 
 */
void r_bootm_boot_s(int rd, int boot_addr)
{
    char u_string[256] = {0,};
    char *ack_buffer;
    int ack_buffer_size;
    sprintf(u_string,"boot_s %d",boot_addr);
    ack_buffer = r_bootm_request_by_string_sync(rd,u_string, &ack_buffer_size,r_bootm_delay, 0);
    if(ack_buffer){
        free(ack_buffer);
    }
}
/**
 * @brief 
 * 
 * @param rd 
 * @param dest 
 * @param src 
 * @param timeout 
 * @return true 
 * @return false 
 */
bool r_bootm_upload(int rd, int dest, int src, int timeout)
{
    char u_string[256] = {0,};
    char *ack_buffer;
    int ack_buffer_size;

    sprintf(u_string,"upload %d %d",dest,src);

    ack_buffer = r_bootm_request_by_string_sync(rd,u_string, &ack_buffer_size,r_bootm_delay, timeout);
    if(ack_buffer == NULL){
        free(ack_buffer);
        return false;
    }
    if(strstr(ack_buffer,"ok")!=NULL){
        free(ack_buffer);
        return true;
    }else{
        free(ack_buffer);
        return false;
    }
}

/**
 * @brief 
 * 
 * @param rd 
 * @param filename 
 * @param file_buffer 
 * @param size 
 * @param timeout 
 * @return true 
 * @return false 
 */
bool r_bootm_ftp_sync(int rd, char *filename, char *file_buffer, int size, int timeout)
{
    const int argc = 5;
    char *argv[5] = {NULL,};
    char tmp_string[5][32];
    argv[0] = &tmp_string[0][0];
    argv[1] = &tmp_string[1][0];
    argv[2] = &tmp_string[2][0];
    argv[3] = &tmp_string[3][0];
    argv[4] = &tmp_string[4][0];
    const int argbc = 1;
    int argbs;
    char tmp_argb[FTP_ERB_MAX_SIZE];
    char *argbuf[1];
    argbuf[0] = tmp_argb;
    int block_size = FTP_ERB_MAX_SIZE;
    int block_num = size / block_size;
    int remain = size % block_size;

    char *ack_buffer;
    int ack_buffer_size;

    if(filename == NULL || file_buffer == NULL || size <= 0){
        return false;
    }

    strcpy(argv[0],"write_file");
    strcpy(argv[1],filename);
    sprintf(argv[2], "%d", size);
    sprintf(argv[3], "%d", block_size);
    for(int i = 0; i < block_num; i++){
        
        sprintf(argv[4], "%d", i * block_size);
        argbs = block_size;
        memcpy(argbuf[0], &file_buffer[i*block_size], argbs);
        ack_buffer = r_bootm_request_sync(rd, argc, argv,argbc, &argbs,argbuf,&ack_buffer_size, r_bootm_delay,timeout);
        if(ack_buffer == NULL){
            return false;
        }
        if (strstr(ack_buffer, "ok"))
        {
            free(ack_buffer);
        }else{
            free(ack_buffer);
            return false;
        }
    }

    if(remain){
        sprintf(argv[3], "%d", remain);
        sprintf(argv[4], "%d", block_num * block_size);
        argbs = remain;
        memcpy(&argbuf[0][0], &file_buffer[block_num*block_size], argbs);
        ack_buffer = r_bootm_request_sync(rd, argc, argv,argbc, &argbs,argbuf,&ack_buffer_size, r_bootm_delay,timeout);
        if(ack_buffer == NULL){
            return false;
        }
        if (strstr(ack_buffer, "ok"))
        {
            free(ack_buffer);
        }else{
            free(ack_buffer);
            return false;
        }
    }
    return true;

}

/**
 * @brief 
 * 
 * @param buffer 
 * @param buffer_size 
 * @param line_index 
 * @param line_state 
 */
static void async_ftp_request_callback(int rd, char *buffer, int buffer_size, int line_index, line_state_t line_state)
{
    session_t *entry = __get_session_from_rd(rd);
    if(entry == NULL){
        return;
    }
    ftp_t *ftp = &entry->ftp;
    const int argc = 5;
    char *argv[5] = {NULL,};
    char tmp_string[5][32];
    argv[0] = &tmp_string[0][0];
    argv[1] = &tmp_string[1][0];
    argv[2] = &tmp_string[2][0];
    argv[3] = &tmp_string[3][0];
    argv[4] = &tmp_string[4][0];
    const int argbc = 1;
    int argbs;
    
    char *argbuf[1];

    char *ack_buffer;
    int ack_buffer_size;

    int ret = 0;

    r_bootm_release_line(rd, line_index);

    do
    {
        /* code */
        if(line_state != line_succeed){
            ftp->ftp_flag = ftp_error;
            break;
        }
        if(strstr(buffer,"ok") == NULL){
            ftp->ftp_flag = ftp_error;
            break;
        }
        ftp->f_tell += ftp->pre_block_size;
        if(ftp->f_tell == ftp->filesize){
            
            ftp->ftp_flag = ftp_succeed;

            break;;
        }
        strcpy(argv[0],"write_file");

        strcpy(argv[1],ftp->filename);

        sprintf(argv[2], "%d", ftp->filesize);

        if(ftp->filesize - ftp->f_tell >= FTP_ERB_MAX_SIZE){
            ftp->pre_block_size = FTP_ERB_MAX_SIZE;
        }else{
            ftp->pre_block_size = ftp->filesize - ftp->f_tell;
        }
        argbs = ftp->pre_block_size;

        sprintf(argv[3], "%d", argbs);
        sprintf(argv[4], "%d", ftp->f_tell);
        RBM_LOG("in session:%ul.ftp:%d / %d.\n",rd,ftp->f_tell, ftp->filesize);
        argbuf[0] = &ftp->filebuffer[ftp->f_tell];

        ret = r_bootm_request(rd, argc, argv, argbc, &argbs, argbuf,  async_ftp_request_callback, R_BOOTM_LINE_ASYNC, 0);
        if(ret < 0){
            ftp->ftp_flag = ftp_error;
            break;
        }

        return;

    } while (0);
    switch (ftp->ftp_flag)
    {
    case ftp_none:
    case ftp_ing:

        break;
    case ftp_error:
        if(ftp->ftp_error_callback){
            ftp->ftp_error_callback();
        }
        /* code */
        break;
    case ftp_succeed:
        if(ftp->ftp_ok_callback){
            ftp->ftp_ok_callback();
        }
        break;
    default:
        break;
    }

    free(ftp->filebuffer);

    memset(ftp, 0, sizeof(ftp));


}

/**
 * @brief 
 * 
 * @param rd 
 * @return enum ftp_flag_t 
 */
enum ftp_flag_t r_bootm_ftp_state(int rd)
{
    session_t *entry = __get_session_from_rd(rd);
    if(entry == NULL){
        return ftp_error;
    }
    return entry->ftp.ftp_flag;
    
}

/**
 * @brief 
 * 
 * @param rd 
 * @param filename 
 * @param file_buffer 
 * @param size 
 * @param ok_callback 
 * @param error_callback 
 * @return true 
 * @return false 
 */
bool r_bootm_ftp_async(int rd, 
char *filename, 
char *file_buffer, 
int size, 
void (*ok_callback)(void), 
void (*error_callback)(void))
{
    session_t *entry = __get_session_from_rd(rd);
    if(entry == NULL){
        return false;
    }
    ftp_t *ftp = &entry->ftp;

    const int argc = 5;
    char *argv[5] = {NULL,};
    char tmp_string[5][32];
    argv[0] = &tmp_string[0][0];
    argv[1] = &tmp_string[1][0];
    argv[2] = &tmp_string[2][0];
    argv[3] = &tmp_string[3][0];
    argv[4] = &tmp_string[4][0];
    const int argbc = 1;
    int argbs = 0;

    char *argbuf[1];
    

    int ack_buffer_size;

    int ret = 0;

    if(ftp->ftp_flag != ftp_none){
        return false;
    }

    do
    {
        /* code */
        strcpy(ftp->filename, filename);
        ftp->filesize = size;
        ftp->f_tell = 0;

        if(ftp->filesize - ftp->f_tell >= FTP_ERB_MAX_SIZE){
            ftp->pre_block_size = FTP_ERB_MAX_SIZE;
        }else{
            ftp->pre_block_size = ftp->filesize - ftp->f_tell;
        }

        
        ftp->ftp_flag = ftp_ing;
        ftp->ftp_error_callback = error_callback;
        ftp->ftp_ok_callback = ok_callback;

        strcpy(argv[0],"write_file");
        strcpy(argv[1],ftp->filename);
        sprintf(argv[2], "%d", ftp->filesize);
        sprintf(argv[3], "%d", ftp->pre_block_size);
        sprintf(argv[4], "%d", ftp->f_tell);
        argbs = ftp->pre_block_size;
        ftp->filebuffer = (char *)malloc(size);
        memcpy(ftp->filebuffer,file_buffer, size);
        argbuf[0] = &ftp->filebuffer[ftp->f_tell];
        ret = r_bootm_request(rd, argc, argv, argbc, &argbs, argbuf,  async_ftp_request_callback, R_BOOTM_LINE_ASYNC, 0);
        if(ret < 0){
            ftp->ftp_flag = ftp_none;
            break;
        }
        RBM_LOG("in session:%ul.start ftp.\n",entry->rd);
        return true;

    } while (0);

    RBM_LOG("in session:%ul.start ftp error.\n",entry->rd);

    free(ftp->filebuffer);

    return false;
} 


