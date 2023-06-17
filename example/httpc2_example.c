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
#define TEST_GET_URL  "https://nghttp2.org/httpbin/get"
#define TEST_POST_URL "https://nghttp2.org/httpbin/post"

/* 自定义缓冲区大小 */
#define BUF_SIZE        4096

int httpc2_save_recv_data(int32_t stream_id, uint8_t *buf, size_t length, uint32_t data_flags)
{
    /* data_flags 为 DATA 帧的标志位, 具体参考 nghttp2_data_flag */
    printf("stream_id: %d.\n", stream_id);
    printf("flags: %d.\n", data_flags);
    printf("buf: %s.\n", buf);
}

int httpc2_send_user_data(int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags)
{
    printf("stream_id: %d.\n", stream_id);
    printf("buf length: %d.\n", length);

    char *data = "post_data";
    int data_len = strlen("post_data");
    memcpy(buf, data, data_len);

    /**
     * 这里要注意两点:
     * 1. 从这里返回后, 就说明拿到的数据是一个 DATA 帧
     * 2. 如果标志位中 NGHTTP2_DATA_FLAG_EOF 置位, 则表示这是最后一个 DATA 帧 
     */
    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    return data_len;
}

/**
 * @brief  发送一个 HTTP/2 GET 请求
 * @param  httpc2: http2 client 对象
 * @return 0:成功, -1:失败
 */
int httpc2_get(httpc2_t *httpc2)
{
    int ret = -1;

    if (NULL == httpc2)
        return -1;

    const nghttp2_nv nva[] = {
        MAKE_HTTP2_HEADER_NV(":method", "GET"),
        MAKE_HTTP2_HEADER_NV(":authority", httpc2->httpc->host),
        MAKE_HTTP2_HEADER_NV(":path", httpc2->httpc->path),
        MAKE_HTTP2_HEADER_NV(":scheme", "https"),
        MAKE_HTTP2_HEADER_NV("accept", "*/*"),
        MAKE_HTTP2_HEADER_NV("user-agent", "tiny http client whit nghttp2/" NGHTTP2_VERSION),
    };

    ret = httpc2_submit_request(httpc2, nva, sizeof(nva) / sizeof(nva[0]), NULL, httpc2_save_recv_data);
    printf("stream_id: %d.\n", ret);

    httpc2_core_run(httpc2);

    return 0;
}

/**
 * @brief  发送一个 HTTP/2 POST 请求
 * @param  httpc2: http2 client 对象
 * @return 0:成功, -1:失败
 */
int httpc2_post(httpc2_t *httpc2)
{
    int ret = -1;

    if (NULL == httpc2)
        return -1;

    const nghttp2_nv nva[] = {
        MAKE_HTTP2_HEADER_NV(":method", "POST"),
        MAKE_HTTP2_HEADER_NV(":authority", httpc2->httpc->host),
        MAKE_HTTP2_HEADER_NV(":path", httpc2->httpc->path),
        MAKE_HTTP2_HEADER_NV(":scheme", "https"),
        MAKE_HTTP2_HEADER_NV("accept", "*/*"),
        MAKE_HTTP2_HEADER_NV("user-agent", "tiny http client whit nghttp2/" NGHTTP2_VERSION),
    };

    ret = httpc2_submit_request(httpc2, nva, sizeof(nva) / sizeof(nva[0]), httpc2_send_user_data, httpc2_save_recv_data);
    printf("stream_id: %d.\n", ret);

    httpc2_core_run(httpc2);
}

void http2_get_request(void)
{
    char *recv_buf = NULL;
    
    httpc2_t *httpc2 = NULL;
    
    httpc2 = httpc2_init(TEST_GET_URL, "443", NULL);

    httpc2_get(httpc2);

    sleep(1);

    httpc2_deinit(httpc2);
}

void http2_post_request(void)
{
    char *recv_buf = NULL;
    
    httpc2_t *httpc2 = NULL;
    
    httpc2 = httpc2_init(TEST_POST_URL, "443", NULL);

    httpc2_post(httpc2);

    sleep(1);

    httpc2_deinit(httpc2);
}

void print_usage(void)
{
    printf("usage: httpc2_example get.\n");
    printf("       httpc2_example post.\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "get")) {
        http2_get_request();
        return 0;
    } else if (0 == strcmp(argv[1], "post")) {
        http2_post_request();
        return 0;
    } else {
        print_usage();
        return -1;
    }
}