
#pragma once

#include "util/str.hpp"

// #include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include <quickjs.h>
#include <spdlog/spdlog.h>

#include "macro.hpp"

namespace lany {
namespace js {

class Object {
protected:
    util::str_list pool;
    std::vector<JSCFunctionListEntry> entries;
    std::vector<
        std::tuple<std::string_view, std::shared_ptr<Object>, bool, uint8_t>>
        objects;

    virtual std::shared_ptr<Object> dup();
    std::string_view dup_str(const std::string_view &str);
    JSCFunctionListEntry
    dup_js_fn_list_entry(const JSCFunctionListEntry &entry);

public:
    Object();
    Object(const Object &other);
    Object(Object &&other);
    Object &operator=(const Object &other);
    Object &operator=(Object &&other);
    ~Object() = default;
    void add_fn(const std::string_view &name, JSCFunction *func,
                uint8_t length = 0);
    template <typename Type>
    void add_prop(const std::string_view &name, Type val,
                  uint8_t prop_flags = JS_PROP_C_W_E) {
        static_assert(std::is_same_v<Type, int32_t> ||
                          std::is_same_v<Type, int64_t> ||
                          std::is_same_v<Type, double> ||
                          std::is_same_v<Type, std::string_view> ||
                          std::is_same_v<Type, JSValue>,
                      "Unsupported type");
        auto _name = dup_str(name);
        if constexpr (std::is_same_v<Type, int32_t>) {
            entries.emplace_back(
                JS_PROP_INT32_DEF(_name.data(), val, prop_flags));
        } else if constexpr (std::is_same_v<Type, int64_t>) {
            entries.emplace_back(
                JS_PROP_INT64_DEF(_name.data(), val, prop_flags));
        } else if constexpr (std::is_same_v<Type, double>) {
            entries.emplace_back(
                JS_PROP_DOUBLE_DEF(_name.data(), val, prop_flags));
        } else if constexpr (std::is_same_v<Type, std::string_view>) {
            entries.emplace_back(JS_PROP_STRING_DEF(
                _name.data(), dup_str(val).data(), prop_flags));
        } else if constexpr (std::is_same_v<Type, JSValue>) {
            entries.emplace_back(
                JS_PROP_UNDEFINED_DEF(_name.data(), prop_flags));
        }
    }
    void add_obj(const std::string_view &name, const JSCFunctionListEntry *tab,
                 int len, uint8_t prop_flags = JS_PROP_C_W_E);
    void add_obj(const std::string_view &name,
                 const std::shared_ptr<Object> &obj, bool deep_copy = false,
                 uint8_t prop_flags = JS_PROP_C_W_E);
    void add_alias(const std::string_view &name, const std::string_view &from,
                   int base = -1);
    virtual JSValue to_js_value(JSContext *ctx);
};

class Class : public Object {
    JSClassID class_id = 0;

protected:
    JSClassCall *ctor;
    JSClassFinalizer *finalizer;
    JSClassGCMark *gc_marker;

    std::string class_name;
    virtual std::shared_ptr<Object> dup() override;

public:
    Class();
    Class(const Class &);
    Class(Class &&other);
    Class &operator=(const Class &);
    Class &operator=(Class &&other);
    virtual ~Class() = default;
    void set_ctor(JSClassCall *func);
    void set_finalizer(JSClassFinalizer *func);
    void set_gc_marker(JSClassGCMark *func);
    void add_getset(const std::string_view &name, JSClassGetter *getter,
                    JSClassSetter *setter);
    JSClassID get_class_id() const;
    virtual JSValue to_js_value(JSContext *ctx) override;
};

class Module : public Object {
public:
    Module();
    Module(const Module &);
    Module(Module &&other);
    Module &operator=(const Module &);
    Module &operator=(Module &&other);
    virtual ~Module() = default;
    void set_export(JSContext *ctx, JSModuleDef *m);
    virtual JSModuleDef *init_module(JSContext *ctx,
                                     const std::string_view &module_name);
};

bool is_buildin_module(const std::string &name);
JSModuleDef *load_module(JSContext *ctx, const std::string &name);
void register_module(const std::string &name, const Module &module);
void unregister_module(const std::string &name);

} // namespace js
} // namespace lany