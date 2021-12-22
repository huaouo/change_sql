//
// Created by huaouo on 2021/12/22.
//

#include "utils.h"

// change_sql --data_path /tmp/data --dst_ip 127.0.0.1 --dst_port 3306 --dst_user root --dst_password 123456789
Config parse_argv(char *argv[]) {
    return {
            .data_path = argv[2],
            .dst_ip = argv[4],
            .dst_user = argv[8],
            .dst_password = argv[10],
            .dst_port = atoi(argv[6])
    };
}

std::vector<std::string> list_dir(const char *p) {
    std::vector<std::string> items;
    DIR *d = opendir(p);
    if (d) {
        struct dirent *src_dir;
        while ((src_dir = readdir(d)) != nullptr) {
            if (strcmp(".", src_dir->d_name) == 0
                || strcmp("..", src_dir->d_name) == 0)
                continue;
            items.emplace_back(src_dir->d_name);
        }
        closedir(d);
    }
    return items;
}

std::string read_file(const char *path) {
    std::ifstream file(path);
    std::string str;
    std::string file_contents;
    while (std::getline(file, str)) {
        file_contents += str;
        file_contents.push_back(' ');
    }
    return file_contents;
}

std::vector<TableTask> extract_table_tasks(const char *data_path) {
    std::vector<TableTask> ret;
    auto srcs = list_dir(data_path);
    std::vector<std::string> srcs_path_;
    for (auto &src: srcs) srcs_path_.push_back(std::string(data_path) + "/" + src + "/");
    auto dbs = list_dir(srcs_path_[0].c_str());
    for (auto &db: dbs) {
        auto db_path_ = srcs_path_[0] + db + "/";
        auto db_items = list_dir(db_path_.c_str());
        std::vector<std::string> table_names, table_ddls;
        for (auto &item: db_items) {
            if (item.substr(item.size() - 4) == ".sql") {
                table_names.push_back(item.substr(0, item.size() - 4));
                table_ddls.push_back(read_file((db_path_ + item).c_str()));
            }
        }
        for (int i = 0; i < table_names.size(); i++) {
            std::vector<std::string> csvs;
            csvs.reserve(srcs_path_.size());
            for (auto &s: srcs_path_) {
                csvs.push_back(s + db + "/" + table_names[i] + ".csv");
            }
            ret.push_back(TableTask{
                    .db = db,
                    .table = table_names[i],
                    .table_ddl = table_ddls[i],
                    .csvs = csvs
            });
        }
    }
    return ret;
}
