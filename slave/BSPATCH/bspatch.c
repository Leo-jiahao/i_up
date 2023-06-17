/**
 * @file bspatch.c
 * @author Leo-jiahao (leo884823525@gmail.com)
 * @brief 
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
#include "bspatch.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief 将八字节数组转为 int64，[0]是低位
 * 
 * @param buff 
 */
int32_t offtin(uint8_t *buff)
{
    int64_t y;
    y = buff[7] & 0x7F;
    y = y * 256; y += buff[6];
    y = y * 256; y += buff[5];
    y = y * 256; y += buff[4];
    y = y * 256; y += buff[3];
    y = y * 256; y += buff[2];
    y = y * 256; y += buff[1];
    y = y * 256; y += buff[0];

    if(buff[7] & 0x80) y = -y;

    return (int32_t)y;
}


/**
 * @brief 将旧文件和patch比对，增删改，到新文件中,使用O3优化有错误
 * 
 * @param old        旧文件地址
 * @param old_size   旧文件字节长度
 * @param new        新文件地址
 * @param new_size   新文件字节长度
 * @param patch      解压后的补丁包地址 + head长度
 * @param patch_size 解压后的补丁包长度 - head长度
 * @param __write_flash 向内部flash写字节流函数  (void * addr, uint32_t size, void *src);
 * @return int 
 * @par -1 err
 * @par  0 ok
 */
int bspatch(
    const uint8_t *old  , int32_t old_size, 
    uint8_t       *new  , int32_t new_size, 
    const uint8_t *patch, int32_t patch_size, write __write_flash)
{
    uint8_t buff2new = NULL;
    uint8_t buff[8];
    int old_pos = 0, new_pos = 0, patch_pos = 0;
    int ctl[3];

    while (new_pos < new_size)
    {
        //读ctl(xi, yi, zi)
        for (int i = 0; i <= 2; i++)
        {
            //从解压后的patch包中读8字节
            if(patch_pos >= patch_size){
                return -1;
            }
            memcpy(buff, (void *)((uint32_t)patch + patch_pos), 8);
            patch_pos += 8;
            ctl[i] = offtin(buff);
        }
        
        //check
        if( ctl[0] < 0 || ctl[0] > INT32_MAX || 
            ctl[1] < 0 || ctl[1] > INT32_MAX || 
            new_pos + ctl[0] > new_size){
                return -1;
        }

        //diff(i),old 和patch 做add操作，赋值到new
        // buff2new = malloc(ctl[0]);
        // if(buff2new == NULL){
        //     return -1;
        // }

        //1 old+diff=new
        for (uint32_t i = 0; i < ctl[0]; i++)
        {
            if(old_pos+i >= 0 && old_pos + i < old_size){
              buff2new = *(uint8_t *)((uint32_t)patch + patch_pos +i) + *(uint8_t*)((uint32_t)old + old_pos + i);
              __write_flash((void *)((uint32_t)new + new_pos + i), 1, &buff2new);
            }
        }
        
        // free(buff2new);
        // buff2new = NULL;

        patch_pos += ctl[0];
        new_pos += ctl[0];
        old_pos += ctl[0];
        
        //check
        if (new_pos + ctl[1] > new_size)
        {
            /* code */
            return -1;
        }

        //将extra(i)追加到new
        
        __write_flash((void *)((uint32_t)new + new_pos), ctl[1], (uint8_t *)((uint32_t)patch + patch_pos));
        
        patch_pos += ctl[1];
        new_pos += ctl[1];
        old_pos += ctl[2];


    }
    
    return 0;
}

