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

#include <xxhash.h>
#include <flat_hash_map.hpp>
#include <spdlog/spdlog.h>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

struct Config {
    const char *data_path, *dst_ip, *dst_user, *dst_password;
    int dst_port;
};

// change_sql --data_path /tmp/data --dst_ip 127.0.0.1 --dst_port 3306 --dst_user root --dst_password 123456789
Config parse_argv(char *argv[]);

struct DDLInfo {
    std::vector<std::string> field_names;
    uint16_t unique_mask, float_mask, double_mask;
};

struct TableTask {
    std::string db, table;
    std::string table_ddl;
    DDLInfo ddl_info;
    std::vector<std::string> csvs;
};

std::vector<std::string> list_dir(const char *p);

std::string read_file(const char *path);

DDLInfo parse_ddl(const char *ddl);

std::vector<TableTask> extract_table_tasks(const char *data_path);

std::string represent_float(const std::string &number_str);

std::string normalize_float(const std::string &number_str);

std::vector<std::vector<TableTask>> distribute_tasks(const std::vector<TableTask> &tasks, int num_cores);

void set_thread_affinity(int i);

time_t serialize_datetime(const char *input_str);

namespace ipc = boost::interprocess;

namespace shared {
    using segment = ipc::managed_shared_memory;
    using manager = segment::segment_manager;
    template<typename T> using alloc = ipc::allocator<T, manager>;
    template<typename K, typename KH = std::hash<K>, typename KEq = std::equal_to<K>>
    using hash_set = ska::flat_hash_set<K, KH, KEq, alloc<K>>;
}

#endif //CHANGE_SQL_UTILS_H
