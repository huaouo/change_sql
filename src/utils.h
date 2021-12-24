//
// Created by huaouo on 2021/12/22.
//

#ifndef CHANGE_SQL_UTILS_H
#define CHANGE_SQL_UTILS_H

#include <dirent.h>
#include <string.h>

#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <algorithm>
#include <spdlog/spdlog.h>

struct Config {
    const char *data_path, *dst_ip, *dst_user, *dst_password;
    int dst_port;
};

// change_sql --data_path /tmp/data --dst_ip 127.0.0.1 --dst_port 3306 --dst_user root --dst_password 123456789
Config parse_argv(char *argv[]);

struct TableTask {
    std::string db, table;
    std::string table_ddl;
    std::vector<std::string> csvs;
};

std::vector<std::string> list_dir(const char *p);

std::string read_file(const char *path);

uint16_t parse_ddl_key_position(const char *ddl);

std::vector<TableTask> extract_table_tasks(const char *data_path);

std::string normalize_float(const std::string &number_str);

#endif //CHANGE_SQL_UTILS_H
