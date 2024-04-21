/*
 * @Author       : Zeepunt
 * @Date         : 2023-06-17
 * @LastEditTime : 2023-06-17
 *  
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/zeepunt
 *  
 * Copyright (c) 2023 by Zeepunt, All Rights Reserved. 
 */
#ifndef __HTTPC_H__
#define __HTTPC_H__

#include "httpc_config.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

/**
 * HTTP Client 错误码
 */
#define HTTPC_ERR_OK    (0)  /**< 正常 */
#define HTTPC_ERR_FAIL  (-1) /**< 一般错误 */
#define HTTPC_ERR_PARAM (-2) /**< 参数错误 */
#define HTTPC_ERR_MEM   (-3) /**< 内存错误 */
#define HTTPC_ERR_SEND  (-4) /**< 发送数据失败 */

typedef enum _http_client_mode {
    HTTPC_MODE_UNKONW = 0,
    HTTPC_MODE_NORMAL = 1,  /**< 正常模式 */
    HTTPC_MODE_CHUNK  = 2,  /**< chunked 模式 */
    HTTPC_MODE_MAX    = 0xff,
} httpc_mode_t;

typedef enum _http_client_version {
    HTTP_VER_1_1 = 0, /**< HTTP/1.1 */
    HTTP_VER_2_0 = 1, /**< HTTP/2 */
} httpc_ver_t;

typedef struct _http_client_tls_info {
    char *cert;
    uint32_t cert_len;
    char *pk;
    uint32_t pk_len;
} httpc_tls_info_t;

#if defined(HTTPC_CFG_ENABLE_MBEDTLS)
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

typedef struct _http_client_mebdtls_context {
    mbedtls_net_context net_ctx;
    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_cfg;

    mbedtls_x509_crt x509_crt;
    mbedtls_pk_context pk_ctx;

    mbedtls_entropy_context entropy_ctx;
    mbedtls_ctr_drbg_context ctr_drbg_ctx;
} httpc_mebdtls_ctx_t;
#endif /* HTTPC_CFG_ENABLE_MBEDTLS */

typedef struct _http_mem_node {
    char *buf;    /**< 所分配的缓冲区 */
    int buf_len;  /**< 缓冲区的长度 */
    int buf_used; /**< 已使用的缓冲区空间 */
} httpc_mem_node_t;

typedef struct _http_client {
    /**
     * 以 https://github.com/Zeepunt/toyWebutils/tree/main/httpc 为例
     * 1. 协议是 HTTPS
     * 2. host = github.com
     * 3. path = Zeepunt/toyWebutils/tree/main/httpc
     */
    char *host;                       /**< 主机地址 */
    char *path;                       /**< 资源路径 */

    httpc_mem_node_t *header_node;    /**< header 缓冲区节点 */

    int socket;                       /**< 本次连接的套接字 */
    int response_code;                /**< http 响应码 */

    int content_length;               /**< 文本长度 */
    int chunked;                      /**< 是否使用 chunked 传输 */

    bool enable_mbedtls;              /**< 是否使用 HTTPS */
#if defined(HTTPC_CFG_ENABLE_MBEDTLS)
    httpc_mebdtls_ctx_t *mbedtls_ctx; /**< MbedTLS 上下文 */
#endif
} httpc_t;

/**
 * @brief  根据关键字从响应中找出对应的数据
 * @param  httpc HTTP Client 对象
 * @param  fields 关键字
 * @return NULL:失败, 否则成功
 */
char* httpc_header_get(httpc_t *httpc, const char *fields);

/**
 * @brief  自定义填充头部缓冲区
 * @param  httpc HTTP Client 对象
 * @param  fmt 填充的参数
 * @return HTTPC_ERR_FAIL:失败, HTTPC_ERR_PARAM:无效参数, 否则成功
 */
int httpc_header_set(httpc_t *httpc, const char *fmt, ...);

/**
 * @brief  发送 HTTP 请求, 并且接收和解析响应报文的头部信息
 * @param  httpc HTTP Client 对象
 * @param  send_buf 发送数据的缓冲区, 可为 NULL
 * @param  send_size 缓冲区的大小
 * @return 参考 HTTPC_ERR_XXX 状态码
 */
int httpc_send_request(httpc_t *httpc, char *send_buf, int send_size);

/**
 * @brief  接收 HTTP 响应报文的主体数据
 * @param  httpc HTTP Client 对象
 * @param  mode 接收的模式
 * @param  buf 接收数据的缓冲区
 * @param  size 缓冲区的大小
 * @return 参考 HTTPC_ERR_XXX 状态码
 */
int httpc_recv_response(httpc_t *httpc, httpc_mode_t mode, char *buf, int size);

/**
 * @brief  HTTP Client 初始化
 * @param  uri 所要访问的资源地址
 * @param  port 指定连接的端口
 * @param  node 外部提供的头部缓冲区
 * @param  version HTTP 协议版本
 * @param  ssl_info 证书信息
 * @return NULL:初始化失败, 否则成功
 */
httpc_t *httpc_init(const char *uri, char *port, httpc_mem_node_t *node, httpc_ver_t version, httpc_tls_info_t *ssl_info);

/**
 * @brief HTTP Client 销毁
 * @param httpc 初始化得到的 HTTP Client
 * @note  销毁后需要将 httpc 置为 NULL
 */
void httpc_deinit(httpc_t *httpc);

#if defined(HTTPC_CFG_ENABLE_NGHTTP2)
#include <nghttp2/nghttp2.h>

#define MAKE_HTTP2_HEADER_NV(NAME, VALUE)                                                    \
    {                                                                                        \
        (uint8_t *)NAME, (uint8_t *)VALUE, strlen(NAME), strlen(VALUE), NGHTTP2_NV_FLAG_NONE \
    }

typedef int (*httpc2_read_from_user_t)(int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags);
typedef int (*httpc2_write_to_user_t)(int32_t stream_id, uint8_t *buf, size_t length, uint32_t data_flags);

typedef struct _httpc2_cilent {
    httpc_t *httpc;                        /**< http client */
    nghttp2_session *session;              /**< nghttp2 session */
    httpc2_write_to_user_t write_callback; /**< 接收数据时的回调函数 */
} httpc2_t;

/**
 * @brief  提交一个 HTTP/2 请求
 * @param  httpc2 HTTP/2 Client
 * @param  nva HTTP/2 头部字段数组
 * @param  nva_len 数组大小
 * @param  read_callback 所需要发送的数据
 * @param  write_callbck 保存接收到的数据
 * @return <=0:失败, 否则返回所创建的 stream id
 * @note   1. 该函数会创建一个新的 stream, 所发送的 HEADERS 帧是由 nva 参数决定的, 所发送的 DATA 帧是由 read_callback 函数决定的
 * @note   2. 该函数只是将 HEADERS 和 DATA 帧保存到 nghttp2 的内部缓冲区, 并不是真正的发送函数
 */
int httpc2_submit_request(httpc2_t *httpc2, const nghttp2_nv *nva, size_t nva_len, httpc2_read_from_user_t read_callback, httpc2_write_to_user_t write_callback);

/**
 * @brief   运行一次 HTTP/2 的发送/接收操作
 * @param   httpc2 HTTP/2 Client
 * @returns 参考 HTTPC_ERR_XXX 状态码
 * @note    1. nghttp2 的 socket 是不阻塞的, 所以该函数不会阻塞
 * @note    2. 由于是只运行一次, 需要调用者不断调用, 以确保正常的发送接收
 */
int httpc2_core_run(httpc2_t *httpc2);

/**
 * @brief  HTTP/2 Client 初始化
 * @param  uri 所要访问的资源地址
 * @param  port 指定连接的端口
 * @param  ssl_info 证书信息
 * @return NULL:初始化失败, 否则成功
 */
httpc2_t *httpc2_init(const char *uri, char *port, httpc_tls_info_t *ssl_info);

/**
 * @brief HTTP/2 Client 销毁
 * @param httpc2 初始化得到的 HTTP/2 Client
 * @note  销毁后需要将 httpc2 置为 NULL
 */
void httpc2_deinit(httpc2_t *httpc2);
#endif /* HTTPC_CFG_ENABLE_NGHTTP2 */

#endif /* __HTTPC_H__ */