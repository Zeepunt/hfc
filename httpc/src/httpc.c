/*
 * @Author       : Zeepunt
 * @Date         : 2023-06-17
 * @LastEditTime : 2024-04-21
 *  
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/zeepunt
 *  
 * Copyright (c) 2023 by Zeepunt, All Rights Reserved. 
 */
#include <fcntl.h>
#include <httpc/httpc.h>

#if defined(MBEDTLS_SSL_ALPN)
static const char *s_mebdtls_alpn_list[3] = {
    "http/1.1",
    "h2",
    NULL
};
#endif

/**
 * @brief  将数据发送到服务器
 * @param  httpc HTTP Client 对象
 * @param  buf 发送数据的缓冲区
 * @param  size 缓冲区的大小
 * @return HTTPC_ERR_FAIL:失败, HTTPC_ERR_PARAM:无效参数, 否则返回数据大小
 */
static int httpc_send(httpc_t *httpc, char *buf, int size)
{
    if ((NULL == httpc) || (NULL == buf) || (httpc->socket < 0))
        return HTTPC_ERR_PARAM;

#if defined(HTTPC_CFG_ENABLE_MBEDTLS)
    if (httpc->enable_mbedtls) {
        return mbedtls_ssl_write(&(httpc->mbedtls_ctx->ssl_ctx), buf, size);
    } else {
        return send(httpc->socket, buf, size, 0);
    }
#else
    return send(httpc->socket, buf, size, 0);
#endif
}

/**
 * @brief  接收服务器的数据
 * @param  httpc HTTP Client 对象
 * @param  buf 接收数据的缓冲区
 * @param  size 缓冲区的大小
 * @return HTTPC_ERR_FAIL:失败, HTTPC_ERR_PARAM:无效参数, 否则返回数据大小
 */
static int httpc_recv(httpc_t *httpc, char *buf, int size)
{
    if ((NULL == httpc) || (NULL == buf) || (httpc->socket < 0))
        return HTTPC_ERR_PARAM;

#if defined(HTTPC_CFG_ENABLE_MBEDTLS)
    if (httpc->enable_mbedtls) {
        return mbedtls_ssl_read(&(httpc->mbedtls_ctx->ssl_ctx), buf, size);
    } else {
        return recv(httpc->socket, buf, size, 0);
    }
#else
    return recv(httpc->socket, buf, size, 0);
#endif
}

/**
 * @brief  发送报文头部数据
 * @param  httpc HTTP Client 对象
 * @return HTTPC_ERR_FAIL:失败, HTTPC_ERR_PARAM:无效参数, 否则成功
 */
static int httpc_send_header(httpc_t *httpc)
{
    if ((NULL == httpc) || (NULL == httpc->header_node) || (NULL == httpc->header_node->buf))
        return HTTPC_ERR_PARAM;

    httpc_debug("http request header:\n%s", httpc->header_node->buf);
    return httpc_send(httpc, httpc->header_node->buf, httpc->header_node->buf_used);
}

/**
 * @brief  发送报文主体数据
 * @param  httpc HTTP Client 对象
 * @param  buf 发送数据的缓冲区
 * @param  size 缓冲区的大小
 * @return HTTPC_ERR_FAIL:失败, HTTPC_ERR_PARAM:无效参数, 否则成功
 */
static int httpc_send_data(httpc_t *httpc, char *buf, int size)
{
    if ((NULL == httpc) || (NULL == buf))
        return HTTPC_ERR_PARAM;

    httpc_debug("http request data:\n%s", buf);
    return httpc_send(httpc, buf, size);
}

/**
 * @brief HTTP Client 内存节点置零
 * @param node HTTP 内存节点
 */
static void httpc_mem_node_reset(httpc_mem_node_t *node)
{
    if (NULL == node)
        return;
    
    memset(node->buf, 0, node->buf_len);
    node->buf_used = 0;
}

/**
 * @brief  解析 uri
 * @param  httpc HTTP Client 对象
 * @param  uri 请求的资源地址
 * @param  port 连接的端口
 * @return 参考 HTTPC_ERR_XXX 状态码
 */
static int httpc_parse_uri(httpc_t *httpc, char *uri, char *port)
{
    int ret = HTTPC_ERR_OK;
    int len = 0;

    char *host_ptr = NULL;
    char *path_ptr = NULL;

    if ((NULL == httpc) || (NULL == uri)) {
        ret = HTTPC_ERR_PARAM;
        goto exit;
    }

    if (0 == strncmp(uri, "http://", 7)) {
        httpc->enable_mbedtls = false;
        host_ptr = uri + 7;
    } else if (0 == strncmp(uri, "https://", 8)) {
        httpc->enable_mbedtls = true;
        host_ptr = uri + 8;
    } else {
        httpc_error("unknown uri: %s.", uri);
        ret = HTTPC_ERR_PARAM;
        goto exit;
    }

    httpc->host = NULL;
    path_ptr = strstr(host_ptr, "/");
    /**
     * 两种情况:
     * 1. https://github.com
     * 2. https://github.com/zeepunt 
     */
    if (NULL == path_ptr) {
        len = strlen(host_ptr) + 1;
    } else {
        len = (path_ptr - host_ptr) + 1;
    }

    httpc->host = httpc_calloc(1, len);
    if (NULL == httpc->host) {
        httpc_error("host buffer malloc fail.");
        ret = HTTPC_ERR_MEM;
        goto exit;
    }
    snprintf(httpc->host, len, "%s", host_ptr);
    httpc_debug("host: %s.", httpc->host);

    httpc->path = NULL;
    /**
     * 两种情况:
     * 1. https://github.com
     * 2. https://github.com/zeepunt 
     */
    if (NULL == path_ptr) {
        len = 2;
    } else {
        len = strlen(path_ptr) + 1;
    }

    httpc->path = httpc_calloc(1, len);
    if (NULL == httpc->path) {
        httpc_error("path buffer malloc fail.");
        ret = HTTPC_ERR_MEM;
        goto exit;
    }
    snprintf(httpc->path, len, "%s", (NULL == path_ptr) ? "/" : path_ptr);
    httpc_debug("path: %s.", httpc->path);

    /* 通过 DNS 找到主机地址对应的 IP 地址并连接 */
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *cur = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    ret = getaddrinfo(httpc->host,
                      port,
                      &hints,
                      &result);
    if (ret < 0) {
        httpc_error("getaddrinfo fail: %s.", gai_strerror(ret));
        ret = HTTPC_ERR_FAIL;
        goto exit;
    }

    char ip_str[INET_ADDRSTRLEN] = {0};

    for (cur = result; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family == AF_INET) {
            ret = connect(httpc->socket, cur->ai_addr, sizeof(struct sockaddr));
            if (0 == ret) {
                inet_ntop(AF_INET, &((struct sockaddr_in *)cur->ai_addr)->sin_addr, ip_str, INET_ADDRSTRLEN);
                httpc_info("connected ip addr: %s.", ip_str);
                break;
            }
        }
    }

    freeaddrinfo(result);
    ret = HTTPC_ERR_OK;
    return ret;

exit:
    safe_free(httpc->path);
    safe_free(httpc->host);
    return ret;
}

/**
 * @brief  从响应的头部数据中读取一行数据
 * @param  httpc HTTP Client 对象
 * @param  buf 接收数据的缓冲区
 * @param  size 缓冲区大小
 * @return HTTPC_ERR_PARAM:无效参数, 否则返回数据大小
 */
static int httpc_read_one_line(httpc_t *httpc, char *buf, int size)
{
    int ret = HTTPC_ERR_OK;
    int count = 0;
    char byte = 0;
    char last_byte = 0;

    if ((NULL == httpc) || (NULL == buf)) {
        ret = HTTPC_ERR_PARAM;
        return ret;
    }

    while (1) {
        ret = httpc_recv(httpc, &byte, 1);
        if (ret <= 0)
            break;

        if ((byte == '\n') && (last_byte == '\r')) // "\r\n" 在 HTTP 中表示一行结束
            break;

        last_byte = byte;
        buf[count++] = byte;
        if (count >= size) {
            httpc_warn("buffer is full, maybe data is incomplete.[%d >= %d]", count, size);
            break;
        }
    }

    return count; // 这里返回的长度 = 数据 + '\r' + 1Byte , 所以调用函数需要做相应处理
}

/**
 * @brief  接收 chunk 模式的报文主体数据
 * @param  httpc HTTP Client 对象
 * @param  buf 接收数据的缓冲区
 * @param  size 缓冲区的大小
 * @return HTTPC_ERR_PARAM:无效参数, 否则返回数据大小
 */
static int httpc_chunked_recv(httpc_t *httpc, char *buf, int size)
{
    int ret = HTTPC_ERR_FAIL;
    int count = 0;
    int len = -1;
    char *tmp_buf = NULL;

    if ((NULL == httpc) || (NULL == buf)) {
        ret = HTTPC_ERR_PARAM;
        goto exit;
    }

    tmp_buf = httpc_calloc(1, 1024 * sizeof(char));
    if (NULL == tmp_buf) {
        ret = HTTPC_ERR_MEM;
        goto exit;
    }

    /**
     * chunk 格式:
     * ...
     * Length\r\n
     * chunk_data\r\n
     * ...
     * Length\r\n
     * chunk_data\r\n
     */
    count = httpc_read_one_line(httpc, tmp_buf, 1024);
    if (count < 0) {
        httpc_error("chunk length invalid.");
        goto exit;
    }
    tmp_buf[count - 1] = '\0';

    len = strtoul(tmp_buf, NULL, 16);
    if (len < 0) {
        httpc_error("chunk data invalid.");
        goto exit;
    } else if (len == 0) {
        httpc_debug("no more chunk data.");
        goto exit;
    }

    if (len > size) {
        httpc_error("buffer is smaller than chunk size");
        goto exit;
    }

    count = httpc_recv(httpc, buf, len);
    if (count < 0) {
        httpc_error("recv chunk data fail.");
        goto exit;
    }

    len = httpc_recv(httpc, tmp_buf, 2); // 对于 CRLF, 不做处理
    if (len < 0) {
        httpc_error("recv chunk CRLF fail.");
    }
    ret = count;

exit:
    safe_free(tmp_buf);
    return ret;
}

/**
 * @brief  HTTP Client SSL 销毁
 * @param  httpc HTTP Client 对象
 */
static void httpc_tls_deinit(httpc_t *httpc)
{
    if (NULL == httpc)
        return;
#if defined(HTTPC_CFG_ENABLE_MBEDTLS)
    if (NULL == httpc->mbedtls_ctx)
        return;

    mbedtls_ssl_close_notify(&(httpc->mbedtls_ctx->ssl_ctx));

    mbedtls_ctr_drbg_free(&(httpc->mbedtls_ctx->ctr_drbg_ctx));
    mbedtls_entropy_free(&(httpc->mbedtls_ctx->entropy_ctx));

#if defined(MBEDTLS_PK_PARSE_C)
    mbedtls_pk_free(&(httpc->mbedtls_ctx->pk_ctx));
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_free(&(httpc->mbedtls_ctx->x509_crt));
#endif

    mbedtls_ssl_config_free(&(httpc->mbedtls_ctx->ssl_cfg));
    mbedtls_ssl_free(&(httpc->mbedtls_ctx->ssl_ctx));
    mbedtls_net_free(&(httpc->mbedtls_ctx->net_ctx));

    safe_free(httpc->mbedtls_ctx);
#endif
}

static void httpc_tls_dbg(void *ctx, int level, const char *file, int line, const char *str)
{
    ((void)level);

    printf("%s:%04d: %s", file, line, str);
}

/**
 * @brief  HTTP Client TLS 初始化
 * @param  httpc HTTP Client 对象
 * @param  ssl_info 证书信息
 * @return HTTPC_ERR_OK:成功, 否则失败
 */
static int httpc_tls_init(httpc_t *httpc, httpc_ver_t version, httpc_tls_info_t *ssl_info)
{
    int ret = HTTPC_ERR_FAIL;

    if (NULL == httpc)
        goto exit;

#if defined(HTTPC_CFG_ENABLE_MBEDTLS)
    httpc->mbedtls_ctx = httpc_calloc(1, sizeof(httpc_mebdtls_ctx_t));
    if (NULL == httpc->mbedtls_ctx)
    {
        httpc_error("mbedtls context fail.");
        goto exit;
    }

    mbedtls_net_init(&(httpc->mbedtls_ctx->net_ctx));
    mbedtls_ssl_init(&(httpc->mbedtls_ctx->ssl_ctx));
    mbedtls_ssl_config_init(&(httpc->mbedtls_ctx->ssl_cfg));

    mbedtls_entropy_init(&(httpc->mbedtls_ctx->entropy_ctx));
    mbedtls_ctr_drbg_init(&(httpc->mbedtls_ctx->ctr_drbg_ctx));

    ret = mbedtls_ssl_set_hostname(&(httpc->mbedtls_ctx->ssl_ctx), httpc->host);
    ret = mbedtls_ssl_config_defaults(&(httpc->mbedtls_ctx->ssl_cfg), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_max_version(&(httpc->mbedtls_ctx->ssl_cfg), MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_authmode(&(httpc->mbedtls_ctx->ssl_cfg), MBEDTLS_SSL_VERIFY_NONE);

    ret = mbedtls_ctr_drbg_seed(&(httpc->mbedtls_ctx->ctr_drbg_ctx), mbedtls_entropy_func, &(httpc->mbedtls_ctx->entropy_ctx), NULL, 0);
    mbedtls_ssl_conf_rng(&(httpc->mbedtls_ctx->ssl_cfg), mbedtls_ctr_drbg_random, &(httpc->mbedtls_ctx->ctr_drbg_ctx));
    mbedtls_ssl_conf_dbg(&(httpc->mbedtls_ctx->ssl_cfg), httpc_tls_dbg, NULL);

#if defined(MBEDTLS_DEBUG_C)
    // mbedtls_debug_set_threshold(5);
#endif

    if (HTTP_VER_2_0 == version) {
#if defined(MBEDTLS_SSL_ALPN)
        mbedtls_ssl_conf_alpn_protocols(&(httpc->mbedtls_ctx->ssl_cfg), s_mebdtls_alpn_list);
#endif
    }

    ret = mbedtls_ssl_setup(&(httpc->mbedtls_ctx->ssl_ctx), &(httpc->mbedtls_ctx->ssl_cfg));

    mbedtls_ssl_set_bio(&(httpc->mbedtls_ctx->ssl_ctx), &(httpc->mbedtls_ctx->net_ctx), mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    if ((NULL != ssl_info) && (NULL != ssl_info->cert)) {
#if defined(MBEDTLS_X509_CRT_PARSE_C)
        mbedtls_x509_crt_init(&(httpc->mbedtls_ctx->x509_crt));
        ret = mbedtls_x509_crt_parse(&(httpc->mbedtls_ctx->x509_crt), (const unsigned char *)ssl_info->cert, ssl_info->cert_len);
        if (0 != ret) {
            httpc_error("x509 crt parse fail: %d.", ret);
            goto exit;
        }
        mbedtls_ssl_conf_authmode(&(httpc->mbedtls_ctx->ssl_cfg), MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_ca_chain(&(httpc->mbedtls_ctx->ssl_cfg), &(httpc->mbedtls_ctx->x509_crt), NULL);
#endif
    }

    if ((NULL != ssl_info) && (NULL != ssl_info->pk)) {
#if defined(MBEDTLS_PK_PARSE_C)
        mbedtls_pk_init(&(httpc->mbedtls_ctx->pk_ctx));
        ret = mbedtls_x509_crt_parse(&(httpc->mbedtls_ctx->pk_ctx), (const unsigned char *)ssl_info->pk, ssl_info->pk_len);
        if (0 != ret) {
            httpc_error("pk parse key fail: %d: %d.", ret);
            goto exit;
        }
#endif
    }

    httpc->mbedtls_ctx->net_ctx.fd = httpc->socket;

    ret = mbedtls_ssl_handshake(&(httpc->mbedtls_ctx->ssl_ctx));
    if (0 == ret) {
        httpc_info("handshake success, cipher_suite: %s.", mbedtls_ssl_get_ciphersuite(&(httpc->mbedtls_ctx->ssl_ctx)));
    } else {
        ret = mbedtls_ssl_get_verify_result(&(httpc->mbedtls_ctx->ssl_ctx));
        if (0 != ret) {
            char *buf = httpc_calloc(1, 4096);
            if (NULL != buf) {
                mbedtls_x509_crt_verify_info(buf, 4096, "! ", ret);
                httpc_error("verify result:\n%s.", buf);
                safe_free(buf);
            }
        }
        goto exit;
    }
#endif

    ret = HTTPC_ERR_OK;
    return ret;

exit:
    httpc_tls_deinit(httpc);
    return ret;
}

char* httpc_header_get(httpc_t *httpc, const char *fields)
{
    int count = 0;
    char *str_ptr = NULL;
    char *ptr = NULL;

    if ((NULL == httpc) || (NULL == httpc->header_node) || (NULL == httpc->header_node->buf) || (NULL == fields))
        return NULL;

    str_ptr = httpc->header_node->buf;

    /**
     * 由于接收的响应数据经过处理, 即用 '\0' 代替了 "\r\n"
     * 所以这里需要循环处理多个字符串
     */
    while (count <= httpc->header_node->buf_used) {
        ptr = strstr(str_ptr + count, fields);
        if (NULL == ptr) {
            ptr = strchr(str_ptr + count, '\0');
            if (NULL == ptr)
                break;
            count += (ptr - (str_ptr + count)) + 1;
            ptr = NULL;
        } else {
            break;
        }
    }

    if (NULL == ptr) {
        httpc_error("can not find field[%d]: %s.", count, fields);
        return NULL;
    }

    ptr += strlen(fields) + 1; // +1 的目的是跳过 ":"

    if (ptr[0] == ' ')
        ptr += 1;

    return ptr;
}

int httpc_header_set(httpc_t *httpc, const char *fmt, ...)
{
    int len = 0;

    va_list args;

    if ((NULL == httpc) || (NULL == httpc->header_node) || (NULL == httpc->header_node->buf))
        return HTTPC_ERR_PARAM;

    httpc_mem_node_t *node = httpc->header_node;

    va_start(args, fmt);
    len = vsnprintf(node->buf + node->buf_used,
                    node->buf_len - node->buf_used,
                    fmt,
                    args);
    va_end(args);

    node->buf_used += len;
    if (node->buf_used >= node->buf_len) {
        httpc_warn("buffer is full.");
        return HTTPC_ERR_FAIL;
    }

    return len;
}

/**
 * @brief 处理从服务器接收到的数据: 获得响应的报文头部, 但不包括响应的报文主体
 * @param  httpc HTTP Client 对象
 * @return 参考 HTTPC_ERR_XXX 状态码
 */
int httpc_handle_response(httpc_t *httpc)
{
    int ret = HTTPC_ERR_OK;
    int count = -1;
    char *buf = NULL;
    char *ptr = NULL;

    if ((NULL == httpc) || (NULL == httpc->header_node) || (NULL == httpc->header_node->buf)) {
        ret = HTTPC_ERR_PARAM;
        goto exit;
    }

    httpc_mem_node_t *node = httpc->header_node;
    httpc_mem_node_reset(node);

    httpc_debug("http response header:");
    while (1) {
        buf = node->buf + node->buf_used;

        count = httpc_read_one_line(httpc, buf, node->buf_len - node->buf_used);
        if (count <= 0)
            break;

        if ((buf[0] == '\r') && (1 == count)) { // 报文头部和报文主体之间存在 "\r\n" 换行符
            buf[0] = '\0';
            break;
        }

        buf[count - 1] = '\0';
        httpc_debug("%s.", buf);

        node->buf_used += count; // 这里不用 -1, 因为下次循环会用到
        if (node->buf_used >= node->buf_len) {
            httpc_warn("buffer is full, maybe data is incomplete.");
            break;
        }
    }

    ptr = strstr(node->buf, "HTTP/1.");
    if (NULL == ptr) {
        httpc_warn("can not find response code.");
        return -1;
    }
    ptr += strlen("HTTP/1.1");

    while ((NULL != ptr) && (*ptr == ' '))
        ptr++;
    httpc->response_code = strtoul(ptr, NULL, 10); // strtoul 遇到非数字时自动停止

    httpc->content_length = 0;
    ptr = NULL;
    ptr = httpc_header_get(httpc, "Content-Length");
    if (NULL != ptr)
        httpc->content_length = strtoul(ptr, NULL, 10);

    httpc->chunked = 0;
    if (0 != httpc->content_length) { // chunked 和 Content-Length 是互斥的
        ret = HTTPC_ERR_OK;
        goto exit;
    }

    ptr = NULL;
    ptr = httpc_header_get(httpc, "Transfer-Encoding");
    if (NULL != ptr) {
        if (0 == strcmp(ptr, "chunked")) {
            httpc_info("chunked mode.");
            httpc->chunked = 1;
        }
        ret = HTTPC_ERR_OK;
    } else {
        ret = HTTPC_ERR_FAIL;
    }

exit:
    return ret;
}

int httpc_send_request(httpc_t *httpc, char *send_buf, int send_size)
{
    int ret = HTTPC_ERR_FAIL;

    if (NULL == httpc) {
        ret = HTTPC_ERR_PARAM;
        goto exit;
    }

    ret = httpc_send_header(httpc);
    if (ret < HTTPC_ERR_OK) {
        httpc_error("http send header fail: %d.", ret);
        ret = HTTPC_ERR_SEND;
        goto exit;
    }

    if (NULL != send_buf) {
        ret = httpc_send_data(httpc, send_buf, send_size);
        if (ret < HTTPC_ERR_OK) {
            httpc_error("http send data fail: %d.", ret);
            ret = HTTPC_ERR_SEND;
            goto exit;
        }
    }

    ret = httpc_handle_response(httpc);
    if (HTTPC_ERR_OK != ret) {
        httpc_error("http handle response header fail: %d.", ret);
        goto exit;
    }

exit:
    return ret;
}

int httpc_recv_response(httpc_t *httpc, httpc_mode_t mode, char *buf, int size)
{
    int ret = HTTPC_ERR_FAIL;

    if (NULL == httpc) {
        ret = HTTPC_ERR_PARAM;
        goto exit;
    }

    switch (mode) {
    case HTTPC_MODE_NORMAL:
        ret = httpc_recv(httpc, buf, size);
        break;

    case HTTPC_MODE_CHUNK:
        ret = httpc_chunked_recv(httpc, buf, size);
        break;

    default:
        break;
    }

exit:
    return ret;
}

httpc_t *httpc_init(const char *uri, char *port, httpc_mem_node_t *node, httpc_ver_t version, httpc_tls_info_t *ssl_info)
{
    int ret = HTTPC_ERR_FAIL;
    int len = 0;
    httpc_t *httpc = NULL;

    httpc = httpc_calloc(1, sizeof(httpc_t));
    if (NULL == httpc) {
        httpc_error("http client malloc fail.");
        goto exit;
    }

    httpc->socket = -1;
    httpc->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (httpc->socket < 0) {
        httpc_error("socket create fail.");
        goto malloc_fail;
    }

    if (HTTP_VER_1_1 == version) {
        struct timeval timeout;
        timeout.tv_sec = HTTPC_CFG_SOCKET_TIMEOUT;
        timeout.tv_usec = 0;

        ret = setsockopt(httpc->socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, (socklen_t)(sizeof(timeout)));
        if (-1 == ret)
            httpc_warn("set socket send timeout fail.");

        ret = setsockopt(httpc->socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, (socklen_t)(sizeof(timeout)));
        if (-1 == ret)
            httpc_warn("set socket recv timeout fail.");
    }

    ret = httpc_parse_uri(httpc, uri, port);
    if (ret < HTTPC_ERR_OK) {
        httpc_error("http client parse uri fail.");
        goto parse_fail;
    }

    if (httpc->enable_mbedtls) {
        ret = httpc_tls_init(httpc, version, ssl_info);
        if (0 != ret) {
            goto parse_fail;
        }
    }

    httpc->header_node = node;
    return httpc;

parse_fail:
    if (httpc->socket >= 0)
        close(httpc->socket);
malloc_fail:
    safe_free(httpc);
exit:
    return httpc;
}

void httpc_deinit(httpc_t *httpc)
{
    if (NULL == httpc)
        return;

    if (httpc->enable_mbedtls)
        httpc_tls_deinit(httpc);

    close(httpc->socket);
    safe_free(httpc->path);
    safe_free(httpc->host);
    safe_free(httpc);
}

#if defined(HTTPC_CFG_ENABLE_NGHTTP2)
static ssize_t httpc2_send_callback(nghttp2_session *session,
                                    const uint8_t *data,
                                    size_t length,
                                    int flags,
                                    void *user_data)
{
    int ret = 0;
    httpc2_t *httpc2 = (httpc2_t *)user_data;
    if (NULL != httpc2) {
        ret = httpc_send(httpc2->httpc, data, length);
    }

    if (ret <= 0) {
        httpc_error("err code: %d.", ret);
        if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret)) {
            ret = NGHTTP2_ERR_WOULDBLOCK;
        } else {
            ret = NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    }
    return ret;
}

static ssize_t httpc2_recv_callback(nghttp2_session *session,
                                    uint8_t *buf,
                                    size_t length,
                                    int flags,
                                    void *user_data)
{
    int ret = 0;
    httpc2_t *httpc2 = (httpc2_t *)user_data;
    if (NULL != httpc2) {
        ret = httpc_recv(httpc2->httpc, buf, length);
    }

    if (ret < 0) {
        httpc_error("err code: %d.", ret);
        if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret)) {
            ret = NGHTTP2_ERR_WOULDBLOCK;
        } else {
            ret = NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    } else if (0 == ret) {
        ret = NGHTTP2_ERR_EOF;
    }
    return ret;
}

static int httpc2_on_frame_send_callback(nghttp2_session *session,
                                         const nghttp2_frame *frame,
                                         void *user_data)
{
    switch (frame->hd.type) {
    case NGHTTP2_HEADERS: {
        const nghttp2_nv *nva = frame->headers.nva;
        httpc_debug("send HEADER:");
        for (size_t i = 0; i < frame->headers.nvlen; i++) {
            httpc_debug("%s: %s.", nva[i].name, nva[i].value);
        }
        break;
    }

    case NGHTTP2_RST_STREAM:
        break;

    default:
        break;
    }

    return 0;
}

static int httpc2_on_frame_recv_callback(nghttp2_session *session,
                                         const nghttp2_frame *frame,
                                         void *user_data)
{
    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
        if (NGHTTP2_HCAT_RESPONSE == frame->headers.cat) {
            const nghttp2_nv *nva = frame->headers.nva;
            httpc_debug("recv HEADER:");
            for (size_t i = 0; i < frame->headers.nvlen; i++) {
                httpc_debug("%s: %s.", nva[i].name, nva[i].value);
            }
        }
        break;

    case NGHTTP2_RST_STREAM:
        httpc_debug("recv RST_STREAM frame.");
        break;

    default:
        break;
    }

    return 0;
}

static int httpc2_on_stream_close_callback(nghttp2_session *session,
                                           int32_t stream_id,
                                           uint32_t error_code,
                                           void *user_data)
{
    httpc_debug("stream %d close.", stream_id);
    return 0;
}

static int httpc2_on_data_chunk_recv_callback(nghttp2_session *session,
                                              uint8_t flags,
                                              int32_t stream_id,
                                              const uint8_t *data,
                                              size_t len,
                                              void *user_data)
{
    httpc2_t *httpc2 = (httpc2_t *)user_data;
    if ((NULL != httpc2) && (NULL != httpc2->write_callback)) {
        httpc2->write_callback(stream_id, data, len, flags);
    }
    return 0;
}

static ssize_t httpc2_data_source_read_callback(nghttp2_session *session,
                                                int32_t stream_id,
                                                uint8_t *buf,
                                                size_t length,
                                                uint32_t *data_flags,
                                                nghttp2_data_source *source,
                                                void *user_data)
{
    httpc2_read_from_user_t read_callback = (httpc2_read_from_user_t)source->ptr;
    if (NULL != read_callback) {
        return read_callback(stream_id, buf, length, data_flags);
    } else {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        return 0;
    }
}

int httpc2_submit_request(httpc2_t *httpc2, const nghttp2_nv *nva, size_t nva_len, httpc2_read_from_user_t read_callback, httpc2_write_to_user_t write_callback)
{
    int stream_id = -1;

    if ((NULL == httpc2) || (NULL == nva)) {
        httpc_error("invalid param.");
        goto exit;
    }

    httpc2->write_callback = write_callback;
    /**
     * 通过传进来的 nghttp2_nv 结构体, nghttp2 知道 HEADERS 帧的内容和大小
     * 但是如果要发送 DATA 帧的话, nghttp2 需要从外界获取相应的数据
     * nghttp2_data_provider 结构体就是告诉 nghttp2 如何从外界获取数据, 并保存到 nghttp2 内部缓冲区中
     */
    nghttp2_data_provider data_provider;
    data_provider.read_callback = httpc2_data_source_read_callback;
    data_provider.source.ptr = (void *)read_callback;

    stream_id = nghttp2_submit_request(httpc2->session,
                                       NULL,
                                       nva,
                                       nva_len,
                                       (NULL != read_callback) ? &data_provider : NULL,
                                       (void *)httpc2);

exit:
    return stream_id;
}

int httpc2_core_run(httpc2_t *httpc2)
{
    int ret = 0;

    if (NULL == httpc2) {
        return HTTPC_ERR_PARAM;
    }

    ret = nghttp2_session_send(httpc2->session);
    if (0 != ret) {
        httpc_error("session send fail: %d.", ret);
        return HTTPC_ERR_FAIL;
    }

    ret = nghttp2_session_recv(httpc2->session);
    if (0 != ret) {
        httpc_error("session recv fail: %d.", ret);
        return HTTPC_ERR_FAIL;
    }

    return HTTPC_ERR_OK;
}

httpc2_t *httpc2_init(const char *uri, char *port, httpc_tls_info_t *ssl_info)
{
    int ret = 0;
    httpc2_t *httpc2 = NULL;

    if ((NULL == uri) || (NULL == port)) {
        httpc_error("invalid param.");
        goto exit;
    }

    httpc2 = httpc_calloc(1, sizeof(httpc2_t));
    if (NULL == httpc2) {
        httpc_error("http2 client malloc fail.");
        goto malloc_fail;
    }

    nghttp2_session_callbacks *callbacks = NULL;
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_send_callback(callbacks, httpc2_send_callback);
    nghttp2_session_callbacks_set_recv_callback(callbacks, httpc2_recv_callback);
    nghttp2_session_callbacks_set_on_frame_send_callback(callbacks, httpc2_on_frame_send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, httpc2_on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, httpc2_on_stream_close_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, httpc2_on_data_chunk_recv_callback);

    ret = nghttp2_session_client_new(&(httpc2->session), callbacks, (void *)httpc2);
    nghttp2_session_callbacks_del(callbacks);
    if (0 != ret) {
        httpc_error("nhttp2 session new fail: %d.", ret);
        goto malloc_fail;
    }

    httpc2->httpc = httpc_init(uri, port, NULL, HTTP_VER_2_0, ssl_info);
    if (NULL == httpc2->httpc) {
        goto httpc_init_fail;
    }

    /* 连接成功后, 将 socket 改为不阻塞 */
    int flags = fcntl(httpc2->httpc->socket, F_GETFL, 0);
    if (-1 == flags) {
        httpc_error("fcntl cmd: F_GETFL fail.");
        goto httpc_init_fail;
    }

    ret = fcntl(httpc2->httpc->socket, F_SETFL, flags | O_NONBLOCK);
    if (-1 == ret) {
        httpc_error("fcntl cmd: F_SETFL fail.");
        goto httpc_init_fail;
    }

    ret = nghttp2_submit_settings(httpc2->session, NGHTTP2_FLAG_NONE, NULL, 0);
    return httpc2;

httpc_init_fail:
    httpc2_deinit(httpc2);
malloc_fail:
    safe_free(httpc2);
exit:
    return httpc2;
}

void httpc2_deinit(httpc2_t *httpc2)
{
    if (NULL == httpc2)
        return;

    nghttp2_session_del(httpc2->session);
    httpc_deinit(httpc2->httpc);
    safe_free(httpc2);
}
#endif /* HTTPC_CFG_ENABLE_NGHTTP2 */