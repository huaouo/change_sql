//
// Created by huaouo on 2022/1/3.
//

#include "query_builder.h"

QueryBuilder::QueryBuilder(const TableTask &t) {
    hash_state = XXH3_createState();
    field_names = t.ddl_info.field_names;
    unique_mask = t.ddl_info.unique_mask;
    float_mask = t.ddl_info.float_mask;
    table = t.table;
    for (auto &c: t.csvs) {
        csv_handles.push_back(new BufferedReader(c.c_str()));
    }

    auto mem_id = fmt::format("{}.{}", t.db, table);
    shm_segment = new shared::segment(ipc::open_or_create, mem_id.c_str(), SHARED_MGR_SIZE);
    inserted = shm_segment->find_or_construct<shared::hash_set<XXH64_hash_t>>
            ("inserted")(shm_segment->get_segment_manager());
    inserted->reserve(INSERTED_TABLE_RESERVED_SIZE);
    committed_handle_idx = shm_segment->find<unsigned char>("committed_handle_idx").first;
    if (committed_handle_idx == nullptr) {
        committed_handle_idx = shm_segment->construct<unsigned char>("committed_handle_idx")();
        *committed_handle_idx = 0;
    }
    handle_idx = *committed_handle_idx;
    committed_offset = shm_segment->find<long>("committed_offset").first;
    if (committed_offset == nullptr) {
        committed_offset = shm_segment->construct<long>("committed_offset")();
        *committed_offset = 0;
    }
    csv_handles[handle_idx]->seek(*committed_offset);
}

std::string QueryBuilder::next_query() {
    std::string ret;
    if (!insert_if_not_exists_stmt.empty()) {
        ret = std::move(insert_if_not_exists_stmt);
    } else if (tmp_record.empty()) {
        std::vector<std::vector<std::string>> records;
        while (records.size() <= INSERT_NUM_TUPLES) {
            auto rec_handle_idx = handle_idx;
            auto rec_offset = csv_handles[handle_idx]->offset();
            auto rec = next_record();
            if (rec.empty()) {
                return ""; // no more queries
            }

            auto hash = hash_record(rec);
            if (inserted->find(hash) == inserted->end()) { // not need update
                inserted->insert(hash);
                records.push_back(std::move(rec));
            } else { // may need insert or update
                tmp_record = std::move(rec);
                tmp_handle_idx = rec_handle_idx;
                tmp_offset = rec_offset;
                break;
            }
        }
        if (records.empty()) {
            goto elze;
        } else {
            ret = make_insert_statement(records);
        }
    } else {
        elze:
        ret = make_update_statement(tmp_record);
        insert_if_not_exists_stmt = make_insert_where_not_exists_statement(tmp_record);
        tmp_record.clear();
    }
    return ret;
}

void QueryBuilder::commit() {
    if (!insert_if_not_exists_stmt.empty()) {
        return;
    }

    if (tmp_record.empty()) {
        *committed_handle_idx = handle_idx;
        *committed_offset = csv_handles[handle_idx]->offset();
    } else {
        *committed_handle_idx = tmp_handle_idx;
        *committed_offset = tmp_offset;
    }
}

std::vector<std::string> QueryBuilder::next_record() {
    std::vector<std::string> values;
    values.reserve(field_names.size());

    if (csv_handles[handle_idx]->peek() == EOF) {
        handle_idx++;
        if (handle_idx == csv_handles.size()) {
            handle_idx--;
            return values;
        }
    }

    auto &h = *csv_handles[handle_idx];
    std::string v;
    for (int i = 0; i < field_names.size(); i++) {
        v = h.get_value();
        if (((1 << i) & float_mask) != 0)
            v = normalize_float(v);
        values.push_back(std::move(v));
    }
    return values;
}

XXH64_hash_t QueryBuilder::hash_record(const std::vector<std::string> &rec) {
    XXH3_64bits_reset(hash_state);
    for (int i = 0; i < rec.size(); i++) {
        if (((1 << i) & unique_mask) != 0)
            XXH3_64bits_update(hash_state, rec[i].c_str(), rec[i].size());
    }
    return XXH3_64bits_digest(hash_state);
}

std::string QueryBuilder::make_where_clause(const std::vector<std::string> &rec) {
    std::string where;
    where.reserve(128);
    where += "where ";
    for (int i = 0; i < rec.size(); i++) {
        if ((unique_mask & (1 << i)) != 0) { // not unique, need update
            where += field_names[i];
            where += "='";
            where += rec[i];
            where.push_back('\'');
            where += " and";
        }
    }
    where.pop_back();
    where.pop_back();
    where.pop_back();
    where.pop_back();
    return where;
}

std::string QueryBuilder::make_update_statement(const std::vector<std::string> &rec) {
    std::string ret;
    ret.reserve(256);
    ret += "update `";
    ret += table;
    ret += "` set ";
    for (int i = 0; i < rec.size(); i++) {
        if ((unique_mask & (1 << i)) == 0) { // not unique, need update
            ret += field_names[i];
            ret += "='";
            ret += rec[i];
            ret += "',";
        }
    }
    ret.pop_back();

    auto where = make_where_clause(rec);
    where += " and `updated_at`<'";
    where += rec[rec.size() - 1];
    where += "'";
    ret.push_back(' ');
    ret += where;
    return ret;
}

std::string QueryBuilder::make_insert_statement(const std::vector<std::vector<std::string>> &records) {
    std::string ret;
    ret += "insert into `";
    ret += table;
    ret += "` values";
    for (auto &rec: records) {
        ret.push_back('(');
        for (int i = 0; i < rec.size(); i++) {
            ret.push_back('\'');
            ret += rec[i];
            ret.push_back('\'');
            if (i != rec.size() - 1) {
                ret.push_back(',');
            }
        }
        ret += "),";
    }
    ret.pop_back();
    return ret;
}

std::string QueryBuilder::make_insert_where_not_exists_statement(const std::vector<std::string> &rec) {
    std::string ret;
    ret += "insert into `";
    ret += table;
    ret += "` select ";
    for (int i = 0; i < rec.size(); i++) {
        ret.push_back('\'');
        ret += rec[i];
        ret.push_back('\'');
        if (i != rec.size() - 1) {
            ret.push_back(',');
        }
    }
    ret += " where not exists(select 1 from `";
    ret += table;
    ret += "` ";
    ret += make_where_clause(rec);
    ret.push_back(')');
    return ret;
}
