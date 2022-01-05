//
// Created by huaouo on 2021/12/17.
//

#ifndef CHANGE_SQL_MYSQL_H
#define CHANGE_SQL_MYSQL_H

#include <string.h>

#include <uv.h>
#include <xxhash.h>
#include <flat_hash_map.hpp>
#include <openssl/sha.h>

#include "config.h"
#include "utils.h"
#include "query_builder.h"

enum ConnState {
    PRE_CREATE_DATABASE = 0,
    PRE_SWITCH_DB,
    PRE_CREATE_TABLE,
    // TODO: use prepared statements
    INSERT
};

struct ConnContext {
    char username[33]{};
    char password[17]{};
    char *in_buf;
    size_t in_idx = 0;

    uv_tcp_t tcp_client{};
//    TableTask task;
    std::string db, table, table_ddl;
    ConnState state = PRE_CREATE_DATABASE;

    char *write_buf_base;
    uv_write_t write_req;
    uv_buf_t write_buf;

    QueryBuilder *builder;

    ConnContext();

    ~ConnContext();
};

class MySQLClient {
public:
    class Factory {
    public:
        Factory(const char *ip, int port, const char *username, const char *password);

        MySQLClient *create_client(uv_loop_t *loop, const TableTask &task);

    private:
        const char *ip, *username, *password;
        int port;
    };

private:
    MySQLClient(uv_loop_t *loop, const TableTask &task,
                const char *ip, int port, const char *username, const char *password);

    void connect(const char *ip, int port);

    static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

    static void on_connect(uv_connect_t *req, int status);

    static void on_data_ready(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    static void handle_in_packet(ConnContext *ctx);

    static void send_handshake_resp(ConnContext *ctx, char *challenge);

    static void on_write_completed(uv_write_t *req, int status);

    static void send_request(ConnContext *ctx, char type, const char *data, size_t len_data);

    static void send_create_database(ConnContext *ctx);

    static void send_switch_database(ConnContext *ctx);

    static void send_create_table(ConnContext *ctx);

    static void on_shutdown(uv_shutdown_t *req, int status);

    static void send_insert(ConnContext *ctx);

    static unsigned char empty_header[4], auth_block_1[36], auth_block_2[2], auth_block_3[154];

    ConnContext ctx;
};

#endif //CHANGE_SQL_MYSQL_H
