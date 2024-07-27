#pragma once

#include <cstdint>
#include <list>
#include <string_view>

namespace lany {

namespace util {

class _str {
    char *_data;
    uint64_t _size;

public:
    _str(const std::string_view &str);
    _str(const _str &other);
    _str(_str &&other);
    _str &operator=(const _str &other);
    _str &operator=(_str &&other);
    ~_str();

    char *data();
    uint64_t size();
    char *begin();
    char *end();
    std::string_view to_view();
};

class str_list {
    std::list<_str> pool;

public:
    str_list();
    str_list(const str_list &other);
    str_list(str_list &&other);
    ~str_list() = default;

    str_list &operator=(const str_list &other);
    str_list &operator=(str_list &&other);

    std::string_view dup_safe_str(const std::string_view &str);
};

std::string quote(const std::string &s);

} // namespace util

} // namespace lany