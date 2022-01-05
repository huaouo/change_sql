//
// Created by huaouo on 2022/1/3.
//

#ifndef CHANGE_SQL_BUFFERED_READER_H
#define CHANGE_SQL_BUFFERED_READER_H

#include <string>

class BufferedReader {
public:
    explicit BufferedReader(const char *path);

    ~BufferedReader();

    char peek();

    void seek(long offset);

    long offset() const;

    std::string get_value();

private:
    FILE *fd;
    char *read_buffer;
    const long BUFFER_SIZE = 4 * 1024 * 1024;
    long buf_end = BUFFER_SIZE;
    long read_idx = BUFFER_SIZE;
    long offset_ = 0;
};


#endif //CHANGE_SQL_BUFFERED_READER_H
