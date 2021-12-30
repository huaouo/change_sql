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
        threads.emplace_back([&, i]() {
            set_thread_affinity(i);
            uv_loop_t loop;
            uv_loop_init(&loop);

            auto& tasks = distributed_tasks[i];
            for (auto &task: tasks) {
                auto completed_file = fmt::format("/dev/shm/{}.{}.completed", task.db, task.table);
                auto state_file = fmt::format("/dev/shm/{}.{}", task.db, task.table);
                if (access(completed_file.c_str(), F_OK) == 0) {
                    continue;
                }

                auto c = factory.create_client(&loop, task);
                uv_run(&loop, UV_RUN_DEFAULT);
                delete c;
                int fd = open(completed_file.c_str(), O_RDWR | O_CREAT, 0644);
                if (fd != -1) {
                    close(fd);
                }
                remove(state_file.c_str());
            }
            uv_loop_close(&loop);
        });
    }

    for (auto &t: threads) {
        if (t.joinable()) t.join();
    }
}
