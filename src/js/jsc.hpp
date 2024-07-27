
#pragma once

#include <string_view>
#include <vector>

#include <quickjs.h>

namespace lany {
namespace js {
class EntryPoint {
    JSContext *ctx;
    void init();

public:
    EntryPoint(JSRuntime *rt);
    EntryPoint(JSContext *ctx);
    EntryPoint(const EntryPoint &) = delete;
    EntryPoint(EntryPoint &&other);
    ~EntryPoint();

    inline JSContext *get_ctx() noexcept { return ctx; }

    int eval_file(const std::string_view &filename) noexcept;
    int loop() noexcept;
    void dump_error(JSContext *error_ctx = nullptr) noexcept;
};

class Core {
    std::vector<EntryPoint> ep_list;
    JSRuntime *rt;

public:
    Core();
    Core(JSRuntime *rt);
    Core(const Core &) = delete;
    Core(Core &&other);
    ~Core();

    int add_file(const std::string_view &filename) noexcept;
    int loop_all() noexcept;
};
} // namespace js
} // namespace lany
