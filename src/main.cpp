#include "uv.h"
#include "fmt/format.h"
#include "mysql.h"
#include <algorithm>


struct Config {
    const char *data_path, *dst_ip, *dst_user, *dst_password;
    int dst_port;
};

// change_sql --data_path /tmp/data --dst_ip 127.0.0.1 --dst_port 3306 --dst_user root --dst_password 123456789
inline Config parse_argv(char *argv[]) {
    return {
            .data_path = argv[2],
            .dst_ip = argv[4],
            .dst_user = argv[8],
            .dst_password = argv[10],
            .dst_port = atoi(argv[6])
    };
}

int main(int argc, char *argv[]) {
    auto cfg = parse_argv(argv);
    uv_loop_t loop;
    uv_loop_init(&loop);
    MySQLClient client(&loop, cfg.dst_user, cfg.dst_password);
    client.connect(cfg.dst_ip, cfg.dst_port);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
}
