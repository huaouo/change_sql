//
// Created by huaouo on 2021/12/21.
//

#include <string.h>
#include <spdlog/spdlog.h>

#include "mysql.h"

MySQLClient::Factory::Factory(const char *ip, int port, const char *username, const char *password)
        : ip(ip), port(port), username(username), password(password) {}

MySQLClient *MySQLClient::Factory::create_client(uv_loop_t *loop, const TableTask &task) {
    return new MySQLClient(loop, task, ip, port, username, password);
}

ConnContext::ConnContext() {
    in_buf = new char[65536];
    write_buf_base = new char[65536];
    write_buf = uv_buf_init(write_buf_base, 0);
    hash_state = XXH64_createState();
}

ConnContext::~ConnContext() {
    delete[] in_buf;
    delete[] write_buf_base;
    for (auto p: csv_handles) {
        delete p;
    }
    XXH64_freeState(hash_state);
    delete shared_mgr;
}

void ConnContext::init_with_task(const TableTask &t) {
    task = t;
    for (auto &c: t.csvs) {
        csv_handles.push_back(new BufferedReader(c.c_str()));
    }
    auto mem_id = fmt::format("{}.{}", task.db, task.table);
    shared_mgr = new shared::segment(ipc::open_or_create, mem_id.c_str(), SHARED_MGR_SIZE);
    inserted = shared_mgr->find_or_construct<shared::hash_map<uint64_t, time_t>>
            ("inserted")(shared_mgr->get_segment_manager());
    stored_cur_handle = shared_mgr->find<int>("cur_handle").first;
    if (stored_cur_handle == nullptr) {
        stored_cur_handle = shared_mgr->construct<int>("cur_handle")();
        *stored_cur_handle = 0;
    }
    working_cur_handle = *stored_cur_handle;
    stored_read_offset = shared_mgr->find<size_t>("read_offset").first;
    if (stored_read_offset == nullptr) {
        stored_read_offset = shared_mgr->construct<size_t>("read_offset")();
        *stored_read_offset = 0;
    }
    csv_handles[working_cur_handle]->seek(*stored_read_offset);
}

void ConnContext::reuse_write_buf() {
    write_buf.len = 0;
}

Record ConnContext::next_record() {
    RecordType type = INSERT_REC;
    std::vector<std::string> values;
    size_t num_fields = task.ddl_info.field_names.size();
    values.reserve(num_fields);

    if (csv_handles[working_cur_handle]->peek() == EOF) {
        working_cur_handle++;
        if (working_cur_handle == csv_handles.size()) {
            return Record{.values=std::move(values), .field_names=&task.ddl_info.field_names,
                    .record_type=EOF_REC, .unique_mask=0};
        }
    }

    auto &h = *csv_handles[working_cur_handle];
    const auto unique_mask = task.ddl_info.unique_mask;
    XXH64_reset(hash_state, 0);
    std::string v;
    for (int i = 0; i < num_fields; i++) {
        v = h.get_value_unsafe();
        if (((1 << i) & unique_mask) != 0)
            XXH64_update(hash_state, v.c_str(), v.size());
        values.push_back(v);
    }
    uint64_t hash = XXH64_digest(hash_state);
    if (working_cur_handle == 0) { // only insert
        inserted->insert(std::make_pair(hash, serialize_datetime(v.c_str())));
    } else if (working_cur_handle == csv_handles.size() - 1) { // only probe
        auto it = inserted->find(hash);
        if (it != inserted->end()) {
            auto prev_time = it->second;
            auto cur_time = serialize_datetime(v.c_str());
            if (cur_time > prev_time) {
                type = UPDATE_REC;
            } else {
                values.clear();
                type = NOP_REC;
            }
        }
    } else { // probe and insert, not implemented for now
        fmt::print("ERROR: probe and insert branch entered !!");
        exit(-1);
    }

    prepare_cur_handle = working_cur_handle;
    prepare_read_offset = h.offset();
    return Record{.values=std::move(values), .field_names=&task.ddl_info.field_names,
            .record_type=type, .unique_mask=unique_mask};
}

MySQLClient::MySQLClient(uv_loop_t *loop, const TableTask &task,
                         const char *ip, int port, const char *username, const char *password) {
    uv_tcp_init(loop, &ctx.tcp_client);
    strncpy(ctx.username, username, sizeof(ConnContext::username) - 1);
    strncpy(ctx.password, password, sizeof(ConnContext::password) - 1);
    ctx.init_with_task(task);
    connect(ip, port);
}

void MySQLClient::connect(const char *ip, int port) {
    sockaddr_in dest{};
    uv_ip4_addr(ip, port, &dest);
    auto c = new uv_connect_t;
    uv_tcp_connect(c, &ctx.tcp_client,
                   (const struct sockaddr *) &dest, on_connect);
    ctx.tcp_client.data = &ctx;
}

// TODO: buffer pool optimization
void MySQLClient::alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
}

void MySQLClient::on_connect(uv_connect_t *req, int status) {
    uv_read_start((uv_stream_t *) req->handle, alloc_buffer, on_data_ready);
    delete req;
}

void MySQLClient::on_data_ready(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        return;
    }
    auto ctx = reinterpret_cast<ConnContext *>(client->data);
    memcpy(ctx->in_buf, buf->base, nread);
    ctx->in_idx += nread;
    delete[] buf->base;
    if (ctx->in_idx >= 4 &&
        (ctx->in_buf[0] | ctx->in_buf[1] << 8 | ctx->in_buf[2] << 16) + 4 == ctx->in_idx) {
        handle_in_packet(ctx);
    }
}

void MySQLClient::handle_in_packet(ConnContext *ctx) {
    unsigned char packet_type = ctx->in_buf[4];
    char login_challenge[20];
    switch (packet_type) {
        case 0xff: // ERR_Packet
            fmt::print("ERR_Packet received: ");
            for (int i = 0; i < ctx->in_idx; i++) {
                fmt::print("{:x}", ctx->in_buf[i]);
            }
            fmt::print("\n");
            fflush(stdout);
            exit(-1);
            break;
        case 0x00:
        case 0xfe: // OK_Packet
            switch (ctx->state) {
                case PRE_CREATE_DATABASE:
                    send_create_database(ctx);
                    break;
                case PRE_SWITCH_DB:
                    send_switch_database(ctx);
                    break;
                case PRE_CREATE_TABLE:
                    send_create_table(ctx);
                    break;
                case INSERT:
                    send_insert(ctx);
                    break;
            }
            if (ctx->state != INSERT) {
                ctx->state = static_cast<ConnState>(ctx->state + 1);
            }
            break;
        case 0x0a: { // Initial_Handshake_Packet
            char *it = ctx->in_buf + 5;
            // server version
            while (*it != 0) it++;
            it++;
            // connection id
            it += 4;
            // auth-plugin-data-part-1
            memcpy(login_challenge, it, 8);
            it += 8;
            // other data ...
            it += 19;
            // auth-plugin-data-part-2
            memcpy(login_challenge + 8, it, 12);
        }
            break;
    }
    ctx->in_idx = 0;
    if (packet_type == 0x0a) { // Initial_Handshake_Packet
        send_handshake_resp(ctx, login_challenge);
    }
}

void MySQLClient::send_handshake_resp(ConnContext *ctx, char *challenge) {
    unsigned char sha_password[20], shasha_password[20];
    SHA1(reinterpret_cast<const unsigned char *>(ctx->password),
         strlen(ctx->password), sha_password);
    SHA1(sha_password, 20, shasha_password);
    unsigned char right_part[40], sha_right_part[20];
    memcpy(right_part, challenge, 20);
    memcpy(right_part + 20, shasha_password, 20);
    SHA1(right_part, 40, sha_right_part);
    for (int i = 0; i < 20; i++) {
        // sha_password := SHA1(password) XOR SHA1(challenge <concat> SHA1(SHA1(password)))
        sha_password[i] ^= sha_right_part[i];
    }

    // buf := auth_block_1 <concat> username <concat> auth_block_2 <concat> sha_password <concat> auth_block_3
    ctx->reuse_write_buf();
    auto buf = ctx->write_buf;
    memcpy(buf.base + buf.len, auth_block_1, sizeof(auth_block_1));
    buf.len += sizeof(auth_block_1);
    size_t len_username = strlen(ctx->username);
    memcpy(buf.base + buf.len, ctx->username, len_username);
    buf.len += len_username;
    memcpy(buf.base + buf.len, auth_block_2, sizeof(auth_block_2));
    buf.len += sizeof(auth_block_2);
    memcpy(buf.base + buf.len, sha_password, 20);
    buf.len += 20;
    memcpy(buf.base + buf.len, auth_block_3, sizeof(auth_block_3));
    buf.len += sizeof(auth_block_3);
    size_t header_len = buf.len - 4;
    memcpy(buf.base, &header_len, 3);

    uv_write(&ctx->write_req, reinterpret_cast<uv_stream_t *>(&ctx->tcp_client),
             &buf, 1, on_write_completed);
}

void MySQLClient::on_write_completed(uv_write_t *req, int status) {
    auto ctx = reinterpret_cast<ConnContext *>(req->handle->data);
    // TODO: The following two lines must be executed once the server
    //  has received our request completely, but how to ensure this?
    //  Aslo, any error shouldn't occur for the insert request.
    *(ctx->stored_cur_handle) = ctx->prepare_cur_handle;
    *(ctx->stored_read_offset) = ctx->prepare_read_offset;
}

void MySQLClient::send_create_database(ConnContext *ctx) {
    ctx->reuse_write_buf();
    auto buf = ctx->write_buf;

    memcpy(buf.base + buf.len, empty_header, sizeof(empty_header));
    buf.len += sizeof(empty_header);
    *(buf.base + buf.len) = 0x03; // COM_QUERY
    buf.len += 1;
    const char *create_db_str = "create database if not exists ";
    size_t len_create_db_str = strlen(create_db_str);
    memcpy(buf.base + buf.len, create_db_str, len_create_db_str);
    buf.len += len_create_db_str;
    memcpy(buf.base + buf.len, ctx->task.db.c_str(), ctx->task.db.size());
    buf.len += ctx->task.db.size();

    size_t header_len = buf.len - 4;
    memcpy(buf.base, &header_len, 3);

    uv_write(&ctx->write_req, reinterpret_cast<uv_stream_t *>(&ctx->tcp_client),
             &buf, 1, on_write_completed);
}

void MySQLClient::send_switch_database(ConnContext *ctx) {
    ctx->reuse_write_buf();
    auto buf = ctx->write_buf;

    memcpy(buf.base + buf.len, empty_header, sizeof(empty_header));
    buf.len += sizeof(empty_header);
    *(buf.base + buf.len) = 0x02; // COM_INIT_DB
    buf.len += 1;
    memcpy(buf.base + buf.len, ctx->task.db.c_str(), ctx->task.db.size());
    buf.len += ctx->task.db.size();

    size_t header_len = buf.len - 4;
    memcpy(buf.base, &header_len, 3);

    uv_write(&ctx->write_req, reinterpret_cast<uv_stream_t *>(&ctx->tcp_client),
             &buf, 1, on_write_completed);
}

void MySQLClient::send_create_table(ConnContext *ctx) {
    ctx->reuse_write_buf();
    auto buf = ctx->write_buf;

    memcpy(buf.base + buf.len, empty_header, sizeof(empty_header));
    buf.len += sizeof(empty_header);
    *(buf.base + buf.len) = 0x03; // COM_QUERY
    buf.len += 1;
    memcpy(buf.base + buf.len, ctx->task.table_ddl.c_str(), ctx->task.table_ddl.size());
    buf.len += ctx->task.table_ddl.size();

    size_t header_len = buf.len - 4;
    memcpy(buf.base, &header_len, 3);

    uv_write(&ctx->write_req, reinterpret_cast<uv_stream_t *>(&ctx->tcp_client),
             &buf, 1, on_write_completed);
}

std::string MySQLClient::assemble_insert_statement(const std::string &table, const Record &rec) {
    std::string ret;
    ret.reserve(128);
    ret += "insert into `";
    ret += table;
    ret += "` values(";
    for (int i = 0; i < rec.values.size(); i++) {
        ret.push_back('\'');
        ret += rec.values[i];
        ret.push_back('\'');
        if (i != rec.values.size() - 1) {
            ret.push_back(',');
        }
    }
    ret.push_back(')');
    fmt::print("{}\n", ret);
    return ret;
}

std::string MySQLClient::assemble_update_statement(const std::string &table, const Record &rec) {
    std::string ret, where;
    ret.reserve(256);
    where.reserve(128);
    ret += "update `";
    ret += table;
    ret += "` set ";
    where += "where ";
    for (int i = 0; i < rec.values.size(); i++) {
        if ((rec.unique_mask & (1 << i)) == 0) { // not unique, need update
            ret.push_back('`');
            ret += (*rec.field_names)[i];
            ret += "`='";
            ret += rec.values[i];
            ret += "',";
        } else { // unique, in the where clause
            ret.push_back('`');
            ret += (*rec.field_names)[i];
            ret += "`='";
            ret += rec.values[i];
            ret += "' and";
        }
    }
    ret.pop_back();
    where.pop_back();
    where.pop_back();
    where.pop_back();
    where.pop_back();
    ret += where;
    fmt::print("{}\n", ret);
    return ret;
}

void MySQLClient::on_shutdown(uv_shutdown_t *req, int status) {
    uv_close((uv_handle_t *) req->handle, NULL);
    delete req;
}

void MySQLClient::send_insert(ConnContext *ctx) {
    Record rec = ctx->next_record();
    while (rec.record_type == NOP_REC) {
        rec = ctx->next_record();
    }
    std::string stmt;
    switch (rec.record_type) {
        case INSERT_REC:
            stmt = assemble_insert_statement(ctx->task.table, rec);
            break;
        case UPDATE_REC:
            stmt = assemble_update_statement(ctx->task.table, rec);
            break;
        case EOF_REC:
            // TODO: close connection
            auto shutdown_req = new uv_shutdown_t;
            uv_shutdown(shutdown_req, (uv_stream_t *) &ctx->tcp_client, on_shutdown);
            return;
    }

    ctx->reuse_write_buf();
    auto buf = ctx->write_buf;

    memcpy(buf.base + buf.len, empty_header, sizeof(empty_header));
    buf.len += sizeof(empty_header);
    *(buf.base + buf.len) = 0x03; // COM_QUERY
    buf.len += 1;
    memcpy(buf.base + buf.len, stmt.c_str(), stmt.size());
    buf.len += stmt.size();

    size_t header_len = buf.len - 4;
    memcpy(buf.base, &header_len, 3);

    uv_write(&ctx->write_req, reinterpret_cast<uv_stream_t *>(&ctx->tcp_client),
             &buf, 1, on_write_completed);
}

unsigned char MySQLClient::empty_header[4] = {0x00, 0x00, 0x00, 0x00};

unsigned char MySQLClient::auth_block_1[36] =
        {0x00, 0x00, 0x00, 0x01,
         0x05, 0xA6, 0xDF, 0x01,
         0x00, 0x00, 0x00, 0x01,
         0x1C,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00};

unsigned char MySQLClient::auth_block_2[2] =
        {0x00, 0x14};

unsigned char MySQLClient::auth_block_3[154] =
        {0x6D, 0x79, 0x73, 0x71, 0x6C, 0x5F, 0x6E, 0x61,
         0x74, 0x69, 0x76, 0x65, 0x5F, 0x70, 0x61, 0x73,
         0x73, 0x77, 0x6F, 0x72, 0x64, 0x00, 0x83, 0x04,
         0x5F, 0x70, 0x69, 0x64, 0x05, 0x32, 0x32, 0x32,
         0x34, 0x30, 0x0C, 0x70, 0x72, 0x6F, 0x67, 0x72,
         0x61, 0x6D, 0x5F, 0x6E, 0x61, 0x6D, 0x65, 0x05,
         0x6D, 0x79, 0x73, 0x71, 0x6C, 0x07, 0x6F, 0x73,
         0x5F, 0x75, 0x73, 0x65, 0x72, 0x06, 0x68, 0x75,
         0x61, 0x6F, 0x75, 0x6F, 0x0C, 0x5F, 0x63, 0x6C,
         0x69, 0x65, 0x6E, 0x74, 0x5F, 0x6E, 0x61, 0x6D,
         0x65, 0x08, 0x6C, 0x69, 0x62, 0x6D, 0x79, 0x73,
         0x71, 0x6C, 0x07, 0x5F, 0x74, 0x68, 0x72, 0x65,
         0x61, 0x64, 0x05, 0x32, 0x33, 0x33, 0x36, 0x38,
         0x0F, 0x5F, 0x63, 0x6C, 0x69, 0x65, 0x6E, 0x74,
         0x5F, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E,
         0x06, 0x38, 0x2E, 0x30, 0x2E, 0x31, 0x38, 0x03,
         0x5F, 0x6F, 0x73, 0x05, 0x57, 0x69, 0x6E, 0x36,
         0x34, 0x09, 0x5F, 0x70, 0x6C, 0x61, 0x74, 0x66,
         0x6F, 0x72, 0x6D, 0x06, 0x78, 0x38, 0x36, 0x5F,
         0x36, 0x34};
