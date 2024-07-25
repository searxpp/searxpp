
#pragma once

#include <string_view>
#include <vector>

#include <quickjs.h>

namespace lany {
namespace js {
class EntryPoint {
    JSContext *ctx;
    JSValue obj_ep;
    std::vector<JSValue> obj_list;
    void init();

public:
    EntryPoint(JSRuntime *rt);
    EntryPoint(JSContext *ctx);
    EntryPoint(const EntryPoint &) = delete;
    EntryPoint(EntryPoint &&other);
    ~EntryPoint();

    inline JSContext *get_ctx() noexcept { return ctx; }

    int eval_file(const std::string_view &filename) noexcept;
    int run() noexcept;
    void dump_error() noexcept;
    // internal use
    void _add_obj(JSValue obj);
};

class Core {
    std::vector<EntryPoint> ep_list;
    JSRuntime *rt;

public:
    Core();
    Core(JSRuntime *rt);
    ~Core();

    int add_file(const std::string_view &filename) noexcept;
    int run_all_files() noexcept;
};
} // namespace js
} // namespace lany
