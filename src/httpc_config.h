/*
 * @Author       : Zeepunt
 * @Date         : 2023-06-17 12:26:59
 * @LastEditTime : 2023-06-17 13:59:32
 *  
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/zeepunt
 *  
 * Copyright (c) 2023 by Zeepunt, All Rights Reserved. 
 */
#ifndef __HTTPC_CONFIG_H__
#define __HTTPC_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * Debug Module
 */
#define LOG   printf
#define DEBUG 1
#define INFO  1
#define WARN  1
#define ERROR 1

#define LOG_DEBUG(fmt, ...)                                                        \
    do {                                                                           \
        LOG("\033[37m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define LOG_INFO(fmt, ...)                                                         \
    do {                                                                           \
        LOG("\033[32m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define LOG_WARN(fmt, ...)                                                         \
    do {                                                                           \
        LOG("\033[33m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define LOG_ERROR(fmt, ...)                                                        \
    do {                                                                           \
        LOG("\033[31m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#if DEBUG
#define httpc_debug LOG_DEBUG
#else
#define httpc_debug
#endif

#if INFO
#define httpc_info LOG_INFO
#else
#define httpc_info
#endif

#if WARN
#define httpc_warn LOG_WARN
#else
#define httpc_warn
#endif

#if ERROR
#define httpc_error LOG_ERROR
#else
#define httpc_error
#endif

/**
 * Memory Manager 
 */
#define httpc_malloc malloc
#define httpc_calloc calloc
#define httpc_free   free

#define safe_free(p)       \
    do {                   \
        if (p) {           \
            httpc_free(p); \
            p = NULL;      \
        }                  \
    } while (0)

/**
 * Configuration Module
 */
#define HTTPC_CFG_ENABLE_MBEDTLS      /**< 开启 MbedTLS */
#define HTTPC_CFG_ENABLE_NGHTTP2      /**< 开启 NGHTTP2 */

#define HTTPC_CFG_SOCKET_TIMEOUT (10) /**< 接收和发送的超时时间, 单位秒 */

#endif /* __HTTPC_CONFIG_H__ */