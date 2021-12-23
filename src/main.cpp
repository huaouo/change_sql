#include <phmap/phmap.h>
#include "uv.h"
#include "mysql.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    phmap::flat_hash_map<int, int> m;
    m.reserve(3500000);
    m[0] = 0;
    m[1] = 1;
    getchar();

//    auto cfg = parse_argv(argv);
//    auto tasks = extract_table_tasks(cfg.data_path);
//
//    uv_loop_t loop;
//    uv_loop_init(&loop);
//    MySQLClient *clients[128];
//    for (auto &client: clients) {
//        client = new MySQLClient(&loop, cfg.dst_ip, cfg.dst_port,
//                                 cfg.dst_user, cfg.dst_password);
//    }
//    uv_run(&loop, UV_RUN_DEFAULT);
//    uv_loop_close(uv_default_loop());
//    for (auto c: clients) delete c;
}
