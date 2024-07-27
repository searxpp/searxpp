
#include "str.hpp"

#include <iomanip>
#include <sstream>

namespace lany {
namespace util {
_str::_str(const std::string_view &str) {
    _data = new char[str.size() + 1];
    std::memcpy(_data, str.data(), str.size());
    _data[str.size()] = '\0';
    _size = str.size();
}

_str::_str(const _str &other) {
    _data = new char[other._size + 1];
    std::memcpy(_data, other._data, other._size);
    _data[other._size] = '\0';
    _size = other._size;
}

_str::_str(_str &&other) {
    _data = other._data;
    _size = other._size;
    other._data = nullptr;
    other._size = 0;
}

_str &_str::operator=(const _str &other) {
    if (this == &other) {
        return *this;
    }
    _data = new char[other._size + 1];
    std::memcpy(_data, other._data, other._size);
    _data[other._size] = '\0';
    _size = other._size;
    return *this;
}

_str &_str::operator=(_str &&other) {
    if (this == &other) {
        return *this;
    }
    _data = other._data;
    _size = other._size;
    other._data = nullptr;
    other._size = 0;
    return *this;
}

_str::~_str() { delete[] _data; }

char *_str::data() { return _data; }

uint64_t _str::size() { return _size; }

char *_str::begin() { return _data; }

char *_str::end() { return _data + _size; }

std::string_view _str::to_view() { return std::string_view(_data, _size); }

str_list::str_list() = default;

str_list::str_list(const str_list &other) : pool(other.pool) {}

str_list::str_list(str_list &&other) : pool(std::move(other.pool)) {}

str_list &str_list::operator=(const str_list &other) {
    if (this == &other) {
        return *this;
    }
    pool = other.pool;
    return *this;
}

str_list &str_list::operator=(str_list &&other) {
    if (this == &other) {
        return *this;
    }
    pool = std::move(other.pool);
    return *this;
}

std::string_view str_list::dup_safe_str(const std::string_view &str) {
    return pool.emplace_back(str).to_view();
}

std::string quote(const std::string &s) {
    std::ostringstream ss;
    ss << std::quoted(s);
    return ss.str();
}

} // namespace util

} // namespace lany