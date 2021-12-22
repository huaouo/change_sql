#include "uv.h"
#include "xxh3.h"
#include "mysql.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    auto cfg = parse_argv(argv);
    auto tasks = extract_table_tasks(cfg.data_path);

    uv_loop_t loop;
    uv_loop_init(&loop);
    MySQLClient *clients[128];
    for (auto &client: clients) {
        client = new MySQLClient(&loop, cfg.dst_ip, cfg.dst_port,
                                 cfg.dst_user, cfg.dst_password);
    }
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    for (auto c: clients) delete c;
}
