//
// Created by huaouo on 2021/12/21.
//

#include "mysql.h"
#include <spdlog/spdlog.h>

MySQLClient::MySQLClient(uv_loop_t *loop, const char *ip, int port, const char *username, const char *password) {
    uv_tcp_init(loop, &ctx.tcp_client);
    strncpy(ctx.username, username, ConnContext::USERNAME_LEN - 1);
    strncpy(ctx.password, password, ConnContext::PASSWORD_LEN - 1);
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
    ctx->seq = ctx->in_buf[3];
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
            break;
        case 0x00:
        case 0xfe: // OK_Packet
            fmt::print("OK_Packet received\n");
            fflush(stdout);
            break;
        case 0x0a: { // Initial_Handshake_Packet
            fmt::print("Initial_Handshake_Packet received\n");
            fflush(stdout);
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
    auto buf = uv_buf_init(new char[256], 0);
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
    buf.base[0] = static_cast<uint8_t>(buf.len - 4);

    auto w = new uv_write_t;
    w->data = buf.base;
    uv_write(w, reinterpret_cast<uv_stream_t *>(&ctx->tcp_client),
             &buf, 1, on_write_completed);
}

void MySQLClient::on_write_completed(uv_write_t *req, int status) {
    delete[] reinterpret_cast<char *>(req->data);
    delete req;
}

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