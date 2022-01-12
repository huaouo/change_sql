#include <unistd.h>
#include <thread>
#include <vector>
#include <mysql/mysql.h>

#include "utils.h"
#include "config.h"

// run --data_path /home/huaouo/data --dst_ip 127.0.0.1 --dst_port 3306 --dst_user root --dst_password 123
int main2(int argc, char *argv[]) {
    auto cfg = parse_argv(argv);
    auto distributed_tasks = distribute_tasks(extract_table_tasks(cfg.data_path), NUM_CPU_CORES);
//    auto factory = MySQLClient::Factory(
//            cfg.dst_ip, cfg.dst_port, cfg.dst_user, cfg.dst_password);
//
//    std::vector<std::thread> threads;
//    for (int i = 0; i < NUM_CPU_CORES; i++) {
////    for (int i = 0; i < 1; i++) {
//        threads.emplace_back([&, i]() {
//            set_thread_affinity(i);
//            uv_loop_t loop;
//            uv_loop_init(&loop);
//
//            auto &tasks = distributed_tasks[i];
////            while (tasks.size() != 1) {
////                tasks.pop_back();
////            }
//            for (auto &task: tasks) {
//                auto c = factory.create_client(&loop, task);
//                uv_run(&loop, UV_RUN_DEFAULT);
//                delete c;
//            }
//            uv_loop_close(&loop);
//        });
//    }
//
//    for (auto &t: threads) {
//        if (t.joinable()) t.join();
//    }
}

int main(int argc, char *argv[]) {
    auto cfg = parse_argv(argv);
    MYSQL *client;
    if (nullptr == (client = mysql_init(nullptr))) {
        fprintf(stderr, "%s\n", mysql_error(client));
        exit(-1);
    }
    if (mysql_real_connect(client, "219.228.148.87", "root", "@sqlAdmin2021",
                           nullptr, 0, nullptr, 0) == nullptr) {
        fprintf(stderr, "%s\n", mysql_error(client));
        mysql_close(client);
        exit(-1);
    }

    if (mysql_query(client, "create database if not exists demo")) {
        fprintf(stderr, "%s\n", mysql_error(client));
        mysql_close(client);
        exit(1);
    }

    if (mysql_query(client, "use demo")) {
        fprintf(stderr, "%s\n", mysql_error(client));
        mysql_close(client);
        exit(1);
    }

    if (mysql_query(client, "create table t (a int)")) {
        fprintf(stderr, "%s\n", mysql_error(client));
        mysql_close(client);
        exit(1);
    }

    if (mysql_query(client, "insert into t values (1)")) {
        fprintf(stderr, "%s\n", mysql_error(client));
        mysql_close(client);
        exit(1);
    }

    mysql_close(client);
}
