

#include "jsc.hpp"
#include "module.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>

#include <quickjs-libc.h>
#include <quickjs.h>
#include <spdlog/spdlog.h>
#if defined(_WIN32) || defined(_WIN64)
#include <spdlog/sinks/wincolor_sink.h>
using console_sink_mt = spdlog::sinks::wincolor_stderr_sink_mt;
#else
#include <spdlog/sinks/stdout_color_sinks.h>
using console_sink_mt = spdlog::sinks::stderr_color_sink_mt;
#endif

static auto jsc_cs = std::make_shared<console_sink_mt>();
static spdlog::logger dp_logger("JS Dump Error Logger", {jsc_cs});

static std::string read_file(const std::string_view &filename) {
    std::ifstream file(filename.data(), std::ios::in | std::ios::binary);
    if (!file) {
        spdlog::error("Could not open file: {}", filename);
        return "";
    }

    auto ret = std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    spdlog::info("Read file: {}", filename);
    spdlog::debug("Read file size: {}", ret.size());
    spdlog::debug("Read file content: {}", ret);
    return ret;
}

static int js_set_import_meta(JSContext *ctx, JSValueConst func_val,
                              bool is_main) noexcept {
    using namespace std::filesystem;

    JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func_val));

    JSAtom module_name_atom = JS_GetModuleName(ctx, m);
    std::string_view module_name = JS_AtomToCString(ctx, module_name_atom);
    JS_FreeAtom(ctx, module_name_atom);

    if (module_name.empty())
        return -1;

    std::string url;
    if (module_name.find(':') == std::string::npos) {
        url = "file://";
        try {
            // Handle file paths depending on whether realpath should be used
            if (exists(module_name)) {
                url += canonical(module_name).string();
            } else {
                url += absolute(module_name).string();
            }
        } catch (const filesystem_error &e) {
            JS_ThrowTypeError(ctx, "Path resolution failure");
            JS_FreeCString(ctx, module_name.data());
            return -1;
        }
    } else {
        url = module_name;
    }
    spdlog::debug("Set import.meta.url: {}", url);
    JS_FreeCString(ctx, module_name.data());

    JSValue meta_obj = JS_GetImportMeta(ctx, m);
    if (JS_IsException(meta_obj))
        return -1;

    JS_DefinePropertyValueStr(ctx, meta_obj, "url",
                              JS_NewString(ctx, url.c_str()), JS_PROP_C_W_E);
    JS_DefinePropertyValueStr(ctx, meta_obj, "main", JS_NewBool(ctx, is_main),
                              JS_PROP_C_W_E);
    JS_FreeValue(ctx, meta_obj);
    return 0;
}

static JSModuleDef *jsc_module_loader(JSContext *ctx, const char *module_name,
                                      void *opaque) {

    using namespace lany::js;
    JSModuleDef *m = nullptr;
    EntryPoint *ep = static_cast<EntryPoint *>(opaque);

    if (is_buildin_module(module_name)) {
        m = load_module(ctx, module_name);
        return m;
    }

    std::string code = read_file(module_name);
    if (code == "")
        return nullptr;

    JSValue func_val = JS_Eval(ctx, code.c_str(), code.size(), module_name,
                               JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(func_val))
        return nullptr;

    if (js_set_import_meta(ctx, func_val, false) < 0) {
        JS_FreeValue(ctx, func_val);
        return nullptr;
    }

    // ep->_add_obj(func_val);

    m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func_val));
    JS_FreeValue(ctx, func_val);
    return m;
}

static JSContext *JS_NewCustomContext(JSRuntime *rt) {
    JSContext *ctx = JS_NewContextRaw(rt);
    if (!ctx) {
        return nullptr;
    }
    JS_AddIntrinsicBaseObjects(ctx);
    JS_AddIntrinsicDate(ctx);
    JS_AddIntrinsicEval(ctx);
    JS_AddIntrinsicStringNormalize(ctx);
    JS_AddIntrinsicRegExp(ctx);
    JS_AddIntrinsicJSON(ctx);
    JS_AddIntrinsicProxy(ctx);
    JS_AddIntrinsicMapSet(ctx);
    JS_AddIntrinsicTypedArrays(ctx);
    JS_AddIntrinsicPromise(ctx);
    return ctx;
}

namespace lany {
namespace js {

#undef JS_MKVAL
#define JS_MKVAL(tag, val)                                                     \
    JSValue { JSValueUnion{val}, tag }

void EntryPoint::init() {
    JS_SetModuleLoaderFunc(JS_GetRuntime(ctx), NULL, jsc_module_loader, this);
    js_std_add_helpers(ctx, 0, nullptr);
}

EntryPoint::EntryPoint(JSRuntime *rt) {
    ctx = JS_NewCustomContext(rt);
    init();
}
EntryPoint::EntryPoint(JSContext *ctx) : ctx(ctx) { init(); }
EntryPoint::EntryPoint(EntryPoint &&other) {
    ctx = other.ctx;
    other.ctx = nullptr;
    obj_list.swap(other.obj_list);
    obj_ep = other.obj_ep;
    other.obj_ep = JS_NULL;
}
EntryPoint::~EntryPoint() { JS_FreeContext(ctx); }

int EntryPoint::eval_file(const std::string_view &filename) noexcept {
    std::string code = read_file(filename);
    if (code == "")
        return -1;

    int eval_flags = JS_EVAL_TYPE_MODULE;
    // int eval_flags = JS_EVAL_FLAG_COMPILE_ONLY;
    // if (JS_DetectModule(code.c_str(), code.size()))
    //     eval_flags |= JS_EVAL_TYPE_MODULE;
    // else
    //     eval_flags |= JS_EVAL_TYPE_GLOBAL;

    JSValue obj =
        JS_Eval(ctx, code.c_str(), code.size(), filename.data(), eval_flags);
    if (JS_IsException(obj)) {
        JS_FreeValue(ctx, obj);
        dump_error();
        return -1;
    }

    // if (js_set_import_meta(ctx, obj, true) < 0) {
    //     JS_FreeValue(ctx, obj);
    //     dump_error();
    //     return -1;
    // }

    obj_ep = obj;
    return 0;
}

int EntryPoint::run() noexcept {
    // if (JS_IsNull(obj_ep)) {
    //     spdlog::error("EntryPoint::run() can't be run twice");
    //     return -1;
    // }
    // for (auto &obj : obj_list) {
    //     JSValue ret = JS_EvalFunction(ctx, obj);
    //     if (JS_IsException(ret)) {
    //         dump_error();
    //         return -1;
    //     }
    // }
    // JSValue obj = JS_EvalFunction(ctx, obj_ep);
    // if (JS_IsException(obj)) {
    //     dump_error();
    //     return -1;
    // }
    // obj_ep = JS_NULL;
    // JS_FreeValue(ctx, obj);
    js_std_loop(ctx);
    return 0;
}

void EntryPoint::dump_error() noexcept {
    JSValue exn = JS_GetException(ctx);
    std::string_view err = JS_ToCString(ctx, exn);
    if (err.empty())
        dp_logger.error("unknown exception");
    else
        dp_logger.error("{}", err);
    if (JS_IsError(ctx, exn)) {
        JSValue val = JS_GetPropertyStr(ctx, exn, "stack");
        if (!JS_IsUndefined(val)) {
            std::string_view stack = JS_ToCString(ctx, val);
            if (!stack.empty())
                dp_logger.error("JS stack: {}", stack);
            JS_FreeCString(ctx, stack.data());
        }
        JS_FreeValue(ctx, val);
    }
    JS_FreeCString(ctx, err.data());
    JS_FreeValue(ctx, exn);
}

void EntryPoint::_add_obj(JSValue obj) { obj_list.emplace_back(obj); }

Core::Core() {
    rt = JS_NewRuntime();
    js_std_init_handlers(rt);
}
Core::Core(JSRuntime *rt) : rt(rt) {}
Core::~Core() { JS_FreeRuntime(rt); }

int Core::add_file(const std::string_view &filename) noexcept {
    auto ep = EntryPoint(rt);
    if (!ep.eval_file(filename)) {
        return -1;
    }
    ep_list.emplace_back(std::move(ep));
    return 0;
}
int Core::run_all_files() noexcept {
    int ret = 0;
    for (auto &ep : ep_list) {
        if (!ep.run())
            ret = -1;
    }
    return ret;
}
} // namespace js
} // namespace lany