//
// Created by huaouo on 2022/1/3.
//

#include "buffered_reader.h"

BufferedReader::BufferedReader(const char *path) {
    fd = fopen(path, "rb");
    read_buffer = new char[BUFFER_SIZE];
}

BufferedReader::~BufferedReader() {
    fclose(fd);
    delete[] read_buffer;
}

char BufferedReader::peek() {
    if (read_idx == buf_end) {
        if (buf_end == BUFFER_SIZE) {
            buf_end = static_cast<long>(fread(read_buffer, 1, BUFFER_SIZE, fd));
            read_idx = 0;
            if (buf_end == 0) return EOF;
        } else {
            return EOF;
        }
    }
    return read_buffer[read_idx];
}

void BufferedReader::seek(long offset) {
    fseek(fd, offset, SEEK_SET);
    offset_ = offset;
    read_idx = buf_end = BUFFER_SIZE;
}

long BufferedReader::offset() const {
    return offset_;
}

std::string BufferedReader::get_value() {
    std::string ret;
    ret.reserve(48);

    while (true) {
        if (read_idx == buf_end) {
            peek();
        }
        char c = read_buffer[read_idx++];
        offset_++;
        if (c == ',' || c == '\n') break;
        ret.push_back(c);
    }
    return ret;
}
