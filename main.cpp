#include <fmt/format.h>

struct Config {
    const char *data_path, *dst_ip, *dst_port, *dst_user, *dst_password;
};

// change_sql --data_path /tmp/data --dst_ip 127.0.0.1 --dst_port 3306 --dst_user root --dst_password 123456789
inline Config parse_argv(char *argv[]) {
    return {
            .data_path = argv[2],
            .dst_ip = argv[4],
            .dst_port = argv[6],
            .dst_user = argv[8],
            .dst_password = argv[10]
    };
}

int main(int argc, char *argv[]) {
    Config cfg = parse_argv(argv);
    fmt::print("{} {} {} {} {}", cfg.data_path, cfg.dst_ip, cfg.dst_port, cfg.dst_user, cfg.dst_password);
    return 0;
}
