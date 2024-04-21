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
#define TEST_URL          "http://httpbin.org/get"
#define TEST_URL_WITH_SSL "https://httpbin.org/get"
/* chunk 测试链接 */
#define CHUNK_URL "http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx"
/* range 测试链接 */
#define RANGE_URL "http://httpbin.org/range/%d"

/* 保存的文件名 */
#define GET_CHUNK_FILE_PATH     "tmp_file.aspx"

/* 头部缓存区大小 */
#define HEADER_BUF_SIZE 4096
/* 自定义缓冲区大小 */
#define BUF_SIZE        4096

/* AMAZON ROOT CA */
char test_cert[] = 
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\r\n" \
"-----END CERTIFICATE-----\r\n" \
;
 
/**
 * @brief  发送一个 HTTP GET 请求
 * @param  httpc: http client 对象
 * @return 0:成功, -1:失败
 */
int httpc_get(httpc_t *httpc)
{
    if (NULL == httpc)
        return -1;

    httpc_header_set(httpc, "GET %s HTTP/1.1\r\n", httpc->path);
    httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
    httpc_header_set(httpc, "Accept: */*\r\n");
    httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(httpc, "\r\n");

    return httpc_send_request(httpc, NULL, 0);
}

/**
 * @brief  发送一个带范围请求的 HTTP GET 请求
 * @param  httpc: http client 对象
 * @param  start: 请求的开始位置, >=0
 * @param  end: 请求的结束位置, >=0
 * @return 0:成功, -1:失败
 */
int httpc_get_position(httpc_t *httpc, int start, int end)
{
    int ret = -1;
    char *ptr = NULL;

    if (NULL == httpc)
        return -1;

    ret = httpc_get(httpc);
    if (ret < 0)
        return -1;

    ptr = httpc_header_get(httpc, "Accept-Ranges: bytes");
    if (NULL == ptr) {
        printf("not support range request.\n");
        return -1;
    }

    memset(httpc->header_node->buf, 0, httpc->header_node->buf_len);
    httpc->header_node->buf_used = 0;

    httpc_header_set(httpc, "GET %s HTTP/1.1\r\n", httpc->path);
    httpc_header_set(httpc, "Host: %s\r\n", httpc->host);

    if ((start >= 0) && (end >= 0)) {
        httpc_header_set(httpc, "Range: bytes=%d-%d\r\n", start, end);
    } else if ((start >= 0) && (end < 0)) {
        httpc_header_set(httpc, "Range: bytes=%d-\r\n", start);
    } else if ((start < 0) && (end >= 0)) {
        httpc_header_set(httpc, "Range: bytes=-%d\r\n", end);
    } else {
        printf("invalid range.\n");
        return -1;
    }

    httpc_header_set(httpc, "Accept: */*\r\n");
    httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(httpc, "\r\n");
}

void normal_get_request_with_ca(void)
{
    char *recv_buf = NULL;
    
    httpc_t *httpc = NULL;
    
    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;   

    httpc_tls_info_t info;
    info.cert = test_cert;
    info.cert_len = sizeof(test_cert);

    httpc_mem_node_t node;
    node.buf_len = HEADER_BUF_SIZE;
    node.buf = calloc(1, BUF_SIZE);
    if (NULL == node.buf)
        goto end;
    node.buf_used = 0;

    httpc = httpc_init(TEST_URL_WITH_SSL, "443", &node, HTTP_VER_1_1, NULL);

    httpc_get(httpc);

    httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
    printf("recv : %s\n.", recv_buf);

end:
    if (NULL != node.buf)
        free(node.buf);
    if (NULL != recv_buf)
        free(recv_buf);
    httpc_deinit(httpc);
}

void chunk_get_request(void)
{
    int count = 0;
    int fd = -1;

    char *buf = NULL;

    httpc_t *httpc = NULL;

    httpc_mem_node_t node;
    node.buf_len = HEADER_BUF_SIZE;
    node.buf = calloc(1, BUF_SIZE);
    if (NULL == node.buf)
        goto end;
    node.buf_used = 0;

    httpc = httpc_init(CHUNK_URL, "80", &node, HTTP_VER_1_1, NULL);

    httpc_get(httpc);

    buf = calloc(1, BUF_SIZE);
    if (NULL == buf)
        goto end;

    /* 因为只有该demo使用这个文件，所以这里是表示删除 */
    unlink(GET_CHUNK_FILE_PATH);

    fd = open(GET_CHUNK_FILE_PATH, O_RDWR | O_CREAT, 0777);
    if (fd < 0)
    {
        printf("open %s fail, [%s].\n", GET_CHUNK_FILE_PATH, strerror(errno));
        goto end;
    } 

    printf("start download...\n");
    while (1)
    {
        count = httpc_recv_response(httpc, HTTPC_MODE_CHUNK, buf, BUF_SIZE);
        if (count < 0)
            break;
        write(fd, buf, count);
        memset(buf, 0, BUF_SIZE);
    }
    close(fd);
    printf("done.\n");

end:
    if (NULL != node.buf)
        free(node.buf);
    if (NULL != buf)
        free(buf);
    httpc_deinit(httpc);
}

void range_get_request(void)
{
    int ret = -1;
    int offset = 10;
    char tmp_url[32] = {0};
    char *recv_buf = NULL;
    
    httpc_t *httpc = NULL;
    
    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;   

    sprintf(tmp_url, RANGE_URL, offset);

    httpc_mem_node_t node;
    node.buf_len = HEADER_BUF_SIZE;
    node.buf = calloc(1, BUF_SIZE);
    if (NULL == node.buf)
        goto end;
    node.buf_used = 0;

    httpc = httpc_init(tmp_url, "80", &node, HTTP_VER_1_1, NULL);

    ret = httpc_get_position(httpc, 0, offset);
    if (ret < 0)
        goto end;

    httpc_recv_response(httpc, HTTPC_MODE_NORMAL, recv_buf, BUF_SIZE);
    printf("recv : %s.\n", recv_buf);

end:
    if (NULL != node.buf)
        free(node.buf);
    if (NULL != recv_buf)
        free(recv_buf);
    httpc_deinit(httpc);
}

void print_usage(void)
{
    printf("usage:\n");
    printf("get_example chunk.\n");
    printf("get_example range.\n");
    printf("get_example normal_with_ca.\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "chunk")) {
        chunk_get_request();
        return 0;
    } else if (0 == strcmp(argv[1], "range")) {
        range_get_request();
        return 0;
    } else if (0 == strcmp(argv[1], "normal_with_ca")) {
        normal_get_request_with_ca();
        return 0;
    } else {
        print_usage();
        return -1;
    }
}