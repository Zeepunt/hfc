/*
 * @Author       : Zeepunt
 * @Date         : 2023-03-12 13:13:46
 * @LastEditTime : 2023-05-28 20:32:35
 *  
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/zeepunt
 *  
 * Copyright (c) 2023 by ${git_name}, All Rights Reserved. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "httpc.h"

/* 测试链接 */
#define TEST_URL          "http://httpbin.org/post"
#define TEST_URL_WITH_SSL "https://httpbin.org/post"

/* 头部缓存区大小 */
#define HEADER_BUF_SIZE 4096

/* 自定义缓冲区大小 */
#define BUF_SIZE        4096

/**
 * @brief  发送一个 HTTP POST 请求
 * @param  httpc: http client 对象
 * @param  buf: 发送缓冲区
 * @param  size: 缓冲区大小
 * @return 0:成功, -1:失败
 */
int httpc_post(httpc_t *httpc, char *buf, int size)
{
    if (NULL == httpc)
        return -1;

    httpc_header_set(httpc, "POST %s HTTP/1.1\r\n", httpc->path);
    httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
    httpc_header_set(httpc, "Content-Length: %d\r\n", size);
    httpc_header_set(httpc, "Accept: */*\r\n");
    httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(httpc, "\r\n");

    if (NULL != buf)
        return httpc_send_request(httpc, buf, size);
    else
        return httpc_send_request(httpc, NULL, 0);
}

void normal_post_request(void)
{
    int count = 0;

    char *recv_buf = NULL;
    char *send_buf = NULL;

    httpc_t *httpc = NULL;

    httpc_mem_node_t node;
    node.buf_len = HEADER_BUF_SIZE;
    node.buf = calloc(1, BUF_SIZE);
    if (NULL == node.buf)
        goto end;
    node.buf_used = 0;

    httpc = httpc_init(TEST_URL, "80", &node, HTTP_VER_1_1, NULL);

    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;

    send_buf = calloc(1, BUF_SIZE);
    if (NULL == send_buf)
        goto end;

    sprintf(send_buf, "just for post data test.");

    httpc_post(httpc, send_buf, strlen(send_buf));

    count = httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
    printf("recv : %s\n.", recv_buf);

end:
    if (NULL != node.buf)
        free(node.buf);
    if (NULL != send_buf)
        free(send_buf);
    if (NULL != recv_buf)
        free(recv_buf);
    httpc_deinit(httpc);
}

void normal_post_request_with_ssl(void)
{
    int count = 0;

    char *recv_buf = NULL;
    char *send_buf = NULL;

    httpc_t *httpc = NULL;

    httpc_mem_node_t node;
    node.buf_len = HEADER_BUF_SIZE;
    node.buf = calloc(1, BUF_SIZE);
    if (NULL == node.buf)
        goto end;
    node.buf_used = 0;

    httpc = httpc_init(TEST_URL_WITH_SSL, "443", &node, HTTP_VER_1_1, NULL);

    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;

    send_buf = calloc(1, BUF_SIZE);
    if (NULL == send_buf)
        goto end;

    sprintf(send_buf, "just for post data test.");

    httpc_post(httpc, send_buf, strlen(send_buf));

    count = httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
    printf("recv : %s\n.", recv_buf);

end:
    if (NULL != node.buf)
        free(node.buf);
    if (NULL != send_buf)
        free(send_buf);
    if (NULL != recv_buf)
        free(recv_buf);
    httpc_deinit(httpc);
}

void print_usage(void)
{
    printf("usage: post_example normal.\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "normal")) {
        normal_post_request();
        return 0;
    } if (0 == strcmp(argv[1], "normal_with_ssl")) {
        normal_post_request_with_ssl();
        return 0;
    } else {
        print_usage();
        return -1;
    }
}