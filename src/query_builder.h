//
// Created by huaouo on 2022/1/3.
//

#ifndef CHANGE_SQL_QUERY_BUILDER_H
#define CHANGE_SQL_QUERY_BUILDER_H

#include <vector>
#include "buffered_reader.h"
#include "utils.h"
#include "config.h"

class QueryBuilder {
public:
    explicit QueryBuilder(const TableTask &t);

    ~QueryBuilder() {
        XXH3_freeState(hash_state);
        delete shm_segment;
        for (auto p: csv_handles) {
            delete p;
        }
    }

    std::string next_query();

    void commit();

private:
    XXH3_state_t *hash_state;
    std::vector<std::string> field_names;
    uint16_t unique_mask, float_mask;
    std::string table;

    unsigned char handle_idx;
    shared::segment *shm_segment;
    std::vector<BufferedReader *> csv_handles;
    shared::hash_set<XXH64_hash_t> *inserted;

    std::vector<std::string> tmp_record;
    unsigned char tmp_handle_idx;
    long tmp_offset;

    unsigned char *committed_handle_idx;
    long *committed_offset;

    std::string insert_if_not_exists_stmt;

    std::vector<std::string> next_record();

    XXH64_hash_t hash_record(const std::vector<std::string> &rec);

    std::string make_where_clause(const std::vector<std::string> &rec);

    std::string make_update_statement(const std::vector<std::string> &rec);

    std::string make_insert_statement(const std::vector<std::vector<std::string>> &records);

    std::string make_insert_where_not_exists_statement(const std::vector<std::string> &rec);
};

#endif //CHANGE_SQL_QUERY_BUILDER_H
