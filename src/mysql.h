//
// Created by huaouo on 2021/12/17.
//

#ifndef CHANGE_SQL_MYSQL_H
#define CHANGE_SQL_MYSQL_H

#include <openssl/sha.h>
#include <uv.h>
#include <string.h>
#include <fmt/format.h>

class MySQLClient {
public:
    MySQLClient(uv_loop_t *loop, const char *ip, int port, const char *username, const char *password);

private:
    void connect(const char *ip, int port);

    struct ConnContext {
        static const size_t USERNAME_LEN = 33, PASSWORD_LEN = 17, BUF_LEN = 128;

        char username[USERNAME_LEN]{};
        char password[PASSWORD_LEN]{};
        char in_buf[BUF_LEN]{};
        size_t in_idx = 0;
        unsigned char seq = 0;

        uv_tcp_t tcp_client{};
    };

    static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

    static void on_connect(uv_connect_t *req, int status);

    static void on_data_ready(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    static void handle_in_packet(ConnContext *ctx);

    static void send_handshake_resp(ConnContext *ctx, char *challenge);

    static void on_write_completed(uv_write_t *req, int status);

    static unsigned char auth_block_1[36], auth_block_2[2], auth_block_3[154];

    ConnContext ctx;
};

#endif //CHANGE_SQL_MYSQL_H
