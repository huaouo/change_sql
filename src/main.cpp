#include <unistd.h>
#include <thread>
#include <vector>

#include "mysql.h"
#include "utils.h"

// change_sql --data_path /tmp/data --dst_ip 127.0.0.1 --dst_port 3306 --dst_user root --dst_password 123456789
int main(int argc, char *argv[]) {
    auto cfg = parse_argv(argv);
    auto distributed_tasks = distribute_tasks(extract_table_tasks(cfg.data_path), NUM_CPU_CORES);
    auto factory = MySQLClient::Factory(
            cfg.dst_ip, cfg.dst_port, cfg.dst_user, cfg.dst_password);

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_CPU_CORES; i++) {
//    for (int i = 0; i < 1; i++) {
        threads.emplace_back([&, i]() {
            set_thread_affinity(i);
            uv_loop_t loop;
            uv_loop_init(&loop);

            auto tasks = distributed_tasks[i];
            std::vector<MySQLClient *> clients;
            for (auto &task: tasks) {
                clients.push_back(factory.create_client(&loop, task));
            }
//            clients.push_back(factory.create_client(&loop, tasks[0]));
            uv_run(&loop, UV_RUN_DEFAULT);
            uv_loop_close(&loop);
            for (auto c: clients) delete c;
        });
    }

    for (auto &t: threads) {
        if (t.joinable()) t.join();
    }
}
