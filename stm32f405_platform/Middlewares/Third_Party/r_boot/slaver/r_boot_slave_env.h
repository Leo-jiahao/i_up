/**
 * @file r_boot_slave_env.h
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
#ifndef __R_BOOT_SLAVE_ENV_H__
#define __R_BOOT_SLAVE_ENV_H__


/*定义环境变量相关的宏*/
#define ENUM_VALUE(name, assign) name assign,

#define ENUM_CASE(name, assign) case name: return #name;

#define ENUM_STRCMP(name,assign) if(!strcmp(str,#name)) return name;

#define DECLARE_ENUM(EnumType, ENUM_DEF)\
    enum EnumType {\
        ENUM_DEF(ENUM_VALUE)\
    };\
     const char *Get##EnumType##String(const enum EnumType dummy);\
     enum EnumType Get##EnumType##Value(const char *string);\

#define DEFINE_ENUM(EnumType,ENUM_DEF) \
    const char *Get##EnumType##String(enum EnumType value) \
  { \
    switch(value) \
    { \
      ENUM_DEF(ENUM_CASE) \
      default: return ""; /* handle input error */ \
    } \
  } \
    enum EnumType Get##EnumType##Value(const char *str) \
  { \
    ENUM_DEF(ENUM_STRCMP) \
    return (enum EnumType)0; /* handle input error */ \
  } \

  
#endif
