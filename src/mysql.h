//
// Created by huaouo on 2021/12/17.
//

#ifndef CHANGE_SQL_MYSQL_H
#define CHANGE_SQL_MYSQL_H

#include <string.h>

#include <uv.h>
#include <phmap/phmap.h>
#include <openssl/sha.h>

#include "config.h"
#include "utils.h"
#include "xxhash.h"

enum ConnState {
    PRE_CREATE_DATABASE = 0,
    PRE_SWITCH_DB,
    PRE_CREATE_TABLE,
    // TODO: use prepared statements
    INSERT
};

enum RecordType {
    NOP_REC = 0,
    INSERT_REC,
    UPDATE_REC,
    EOF_REC
};

struct Record {
    std::vector<std::string> values;
    const std::vector<std::string> *field_names;
    RecordType record_type;
    uint16_t unique_mask;
};

struct ConnContext {
    char username[33]{};
    char password[17]{};
    char* in_buf;
    size_t in_idx = 0;
    unsigned char seq = 0;

    uv_tcp_t tcp_client{};
    TableTask task;
    ConnState state = PRE_CREATE_DATABASE;

    char* write_buf_base;
    uv_write_t write_req;
    uv_buf_t write_buf;

    ConnContext();

    ~ConnContext();

    void set_task(const TableTask &task);

    void reuse_write_buf();

    std::vector<BufferedReader*> csv_handles;
    phmap::flat_hash_map<uint64_t, time_t> inserted;
    int cur_handle = 0;
    XXH64_state_t *hash_state;

    Record next_record();
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

    static void send_create_database(ConnContext *ctx);

    static void send_switch_database(ConnContext *ctx);

    static void send_create_table(ConnContext *ctx);

    static std::string assemble_insert_statement(const Record &rec);

    static std::string assemble_update_statement(const Record &rec);

    static void on_shutdown(uv_shutdown_t *req, int status);

    static void send_insert(ConnContext *ctx);

    static unsigned char empty_header[4], auth_block_1[36], auth_block_2[2], auth_block_3[154];

    ConnContext ctx;
};

#endif //CHANGE_SQL_MYSQL_H
