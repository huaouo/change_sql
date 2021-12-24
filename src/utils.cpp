//
// Created by huaouo on 2021/12/22.
//

#include <unordered_map>

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

uint16_t parse_ddl_key_position(const char *ddl) {
    uint16_t ret = 0;
    int pos = 0;
    std::unordered_map<std::string, int> m;

    // Skip after the first '('
    while (ddl[pos] != '(') pos++;
    pos++;

    while (true) {
        while (ddl[pos] == ' ') pos++;
        if (ddl[pos] == '`') {

        } else {
            while (ddl[pos] != '(') pos++;
            pos++;
        }
    }
}

std::vector<TableTask> extract_table_tasks(const char *data_path) {
    std::vector<TableTask> ret;
    auto srcs = list_dir(data_path);
    std::vector<std::string> srcs_path_;
    srcs_path_.reserve(srcs.size());
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

std::string normalize_float(const std::string &number_str) {
    std::string n = number_str;
    int digit_cnt = 0, i = 0, dot_pos = n.size();
    bool leading_zero = true;
    while (digit_cnt < 6 && i < n.size()) {
        if (n[i] == '0') {
            if (!leading_zero) digit_cnt++;
        } else if (isdigit(n[i])) {
            digit_cnt++;
            leading_zero = false;
        } else if (n[i] == '.') dot_pos = i;
        i++;
    }
    if (dot_pos == n.size()) {
        for (int j = i; j < n.size(); j++) {
            if (n[j] == '.') {
                dot_pos = j;
                break;
            }
        }
    }

    int end = i;
    bool carry = false;
    if (i < n.size() && n[i] >= '5' || i + 1 < n.size() && n[i] == '.' && n[i + 1] >= '5') {
        carry = true;
        int end_iter = i - 1;
        while (carry && end_iter >= 0) {
            if (n[end_iter] == '.') {
                if (end == end_iter + 1) end--;
            } else if (n[end_iter] == '9') {
                if (end_iter > dot_pos) end--;
                else n[end_iter] = '0';
            } else {
                n[end_iter] += 1;
                carry = false;
            }
            end_iter--;
        }
    }

    if (end > dot_pos) {
        n = n.substr(0, end);
    } else {
        for (int t = end; t < dot_pos; t++) n[t] = '0';
        n = n.substr(0, dot_pos);
    }
    return carry ? std::string("1") + n : n;
}
