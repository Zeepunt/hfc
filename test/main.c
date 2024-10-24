/*
 * Copyright (c) 2023 - 2024 by Zeepunt, All Rights Reserved. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <httpc/httpc.h>

/**
 * 根证书 Amazon Root CA 1
 * 因为测试网址 httpbin.org 的证书是 Amazon Root CA 1 颁发的
 */
static char s_root_cert[] = 
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsFADA5MQswCQYD\r\n"
"VQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6b24gUm9vdCBDQSAxMB4XDTE1\r\n"
"MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTELMAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpv\r\n"
"bjEZMBcGA1UEAxMQQW1hem9uIFJvb3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC\r\n"
"ggEBALJ4gHHKeNXjca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgH\r\n"
"FzZM9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qwIFAGbHrQ\r\n"
"gLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6VOujw5H5SNz/0egwLX0t\r\n"
"dHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L93FcXmn/6pUCyziKrlA4b9v7LWIbxcce\r\n"
"VOF34GfID5yHI9Y/QCB/IIDEgEw+OyQmjgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB\r\n"
"/zAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3\r\n"
"DQEBCwUAA4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDIU5PM\r\n"
"CCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy\r\n"
"8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vvo/ufQJVtMVT8QtPHRh8jrdkPSHCa\r\n"
"2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2\r\n"
"xJNDd2ZhwLnoQdeXeGADbkpyrqXRfboQnoZsG4q5WTP468SQvvG5\r\n"
"-----END CERTIFICATE-----\r\n";

/* 测试链接 */
#define TEST_HTTP_POST_URL  "http://httpbin.org/post"
#define TEST_HTTPS_POST_URL "https://httpbin.org/post"

#define TEST_HTTP_GET_URL   "http://httpbin.org/get"
#define TEST_HTTPS_GET_URL  "https://httpbin.org/get"

/* 头部缓存区大小 */
#define HEADER_BUF_SIZE 4096

/* 自定义缓冲区大小 */
#define BUF_SIZE        4096

static int test_http(char *method)
{
    int ret = 0;

    char *recv_buf = NULL;
    char *send_buf = NULL;

    httpc_t *httpc = NULL;
    httpc_mem_node_t node;

    /* 分配接收缓冲区 */
    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf) {
        printf("malloc fail.\n");
        goto exit;
    }

    /* 分配 HTTP Body 缓冲区 */
    send_buf = calloc(1, BUF_SIZE);
    if (NULL == send_buf) {
        printf("malloc fail.\n");
        goto exit;
    }

    /* 分配 HTTP Header 缓冲区 */
    node.buf_len = HEADER_BUF_SIZE;
    node.buf = calloc(1, BUF_SIZE);
    if (NULL == node.buf) {
        printf("malloc fail.\n");
        goto exit;
    }
    node.buf_used = 0;

    printf("http start: %lds.\n", time(NULL));

    if (0 == strcmp(method, "get")) {
        httpc = httpc_init(TEST_HTTP_GET_URL, "80", &node, HTTP_VER_1_1, NULL);
        if (NULL == httpc) {
            printf("malloc fail.\n");
            goto exit;
        }

        httpc_header_set(httpc, "GET %s HTTP/1.1\r\n", httpc->path);
        httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
        httpc_header_set(httpc, "Accept: */*\r\n");
        httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
        httpc_header_set(httpc, "Connection: close\r\n");
        httpc_header_set(httpc, "\r\n");

        httpc_send_request(httpc, NULL, 0);

        httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
        printf("get recv: %s\n", recv_buf);
    } else if (0 == strcmp(method, "post")) {
        httpc = httpc_init(TEST_HTTP_POST_URL, "80", &node, HTTP_VER_1_1, NULL);
        if (NULL == httpc) {
            printf("malloc fail.\n");
            goto exit;
        }

        sprintf(send_buf, "just for post data test.");

        httpc_header_set(httpc, "POST %s HTTP/1.1\r\n", httpc->path);
        httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
        httpc_header_set(httpc, "Content-Length: %d\r\n", strlen(send_buf));
        httpc_header_set(httpc, "Accept: */*\r\n");
        httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
        httpc_header_set(httpc, "Connection: close\r\n");
        httpc_header_set(httpc, "\r\n");

        httpc_send_request(httpc, send_buf, strlen(send_buf));

        httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
        printf("post recv: %s\n", recv_buf);
    } else {
        ret = -1;
    }

    printf("http end: %lds.\n", time(NULL));

exit:
    if (NULL != node.buf) {
        free(node.buf);
    }

    if (NULL != send_buf) {
        free(send_buf);
    }

    if (NULL != recv_buf) {
        free(recv_buf);
    }

    if (NULL != httpc) {
        httpc_deinit(httpc);
    }

    return ret;
}

static int test_https(char *method)
{
    int ret = 0;

    char *recv_buf = NULL;
    char *send_buf = NULL;

    httpc_t *httpc = NULL;
    httpc_mem_node_t node;

    /* 分配接收缓冲区 */
    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf) {
        printf("malloc fail.\n");
        goto exit;
    }

    /* 分配 HTTP Body 缓冲区 */
    send_buf = calloc(1, BUF_SIZE);
    if (NULL == send_buf) {
        printf("malloc fail.\n");
        goto exit;
    }

    /* 分配 HTTP Header 缓冲区 */
    node.buf_len = HEADER_BUF_SIZE;
    node.buf = calloc(1, BUF_SIZE);
    if (NULL == node.buf) {
        printf("malloc fail.\n");
        goto exit;
    }
    node.buf_used = 0;

    if (0 == strcmp(method, "get")) {
        httpc = httpc_init(TEST_HTTPS_GET_URL, "443", &node, HTTP_VER_1_1, NULL);
        if (NULL == httpc) {
            printf("malloc fail.\n");
            goto exit;
        }

        httpc_header_set(httpc, "GET %s HTTP/1.1\r\n", httpc->path);
        httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
        httpc_header_set(httpc, "Accept: */*\r\n");
        httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
        httpc_header_set(httpc, "\r\n");

        httpc_send_request(httpc, NULL, 0);

        httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
        printf("get recv: %s\n.", recv_buf);
    } else if (0 == strcmp(method, "post")) {
        httpc = httpc_init(TEST_HTTPS_POST_URL, "443", &node, HTTP_VER_1_1, NULL);
        if (NULL == httpc) {
            printf("malloc fail.\n");
            goto exit;
        }

        sprintf(send_buf, "just for post data test.");

        httpc_header_set(httpc, "POST %s HTTP/1.1\r\n", httpc->path);
        httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
        httpc_header_set(httpc, "Content-Length: %d\r\n", strlen(send_buf));
        httpc_header_set(httpc, "Accept: */*\r\n");
        httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
        httpc_header_set(httpc, "\r\n");

        httpc_send_request(httpc, send_buf, strlen(send_buf));

        httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
        printf("post recv: %s\n.", recv_buf);
    } else if (0 == strcmp(method, "cert-post")) {
        httpc_tls_info_t info;
        memset(&info, 0, sizeof(httpc_tls_info_t));

        info.cert = s_root_cert;
        info.cert_len = strlen(s_root_cert) + 1; /* 这里记得 +1, 也就是包含了 '\0' 结束符, 因为 MbedTLS 解析证书时会用到 */

        httpc = httpc_init(TEST_HTTPS_POST_URL, "443", &node, HTTP_VER_1_1, &info);
        if (NULL == httpc) {
            printf("malloc fail.\n");
            goto exit;
        }

        sprintf(send_buf, "just for cert-post data test.");

        httpc_header_set(httpc, "POST %s HTTP/1.1\r\n", httpc->path);
        httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
        httpc_header_set(httpc, "Content-Length: %d\r\n", strlen(send_buf));
        httpc_header_set(httpc, "Accept: */*\r\n");
        httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
        httpc_header_set(httpc, "Connection: close\r\n");
        httpc_header_set(httpc, "\r\n");

        httpc_send_request(httpc, send_buf, strlen(send_buf));

        httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
        printf("post recv: %s\n", recv_buf);
    } else {
        ret = -1;
    }

exit:
    if (NULL != node.buf) {
        free(node.buf);
    }

    if (NULL != send_buf) {
        free(send_buf);
    }

    if (NULL != recv_buf) {
        free(recv_buf);
    }

    if (NULL != httpc) {
        httpc_deinit(httpc);
    }

    return ret;
}

static void print_usage(char *cmd)
{
    printf("Usage:\n");
    printf("%s http <get/post>.\n", cmd);
    printf("%s https <get/post>.\n", cmd);
}

int main(int argc, char *argv[])
{
    int ret = 0;

    if (argc < 2) {
        ret = -1;
        goto exit;
    }

    if (0 == strcmp(argv[1], "http")) {
        if (argc != 3) {
            ret = -1;
            goto exit;
        }

        ret = test_http(argv[2]);
    } else if (0 == strcmp(argv[1], "https")) {
        if (argc != 3) {
            ret = -1;
            goto exit;
        }

        ret = test_https(argv[2]);
    }

    ret = 0;
exit:
    if (ret < 0) {
        print_usage(argv[0]);
    }
    return ret;
}