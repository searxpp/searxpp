
#include "module.hpp"
#include "macro.hpp"
#include "quickjs.h"

#include <map>

#include <spdlog/spdlog.h>
#include <string_view>

static std::map<std::string, lany::js::Module> module_map;

static int module_init_helper(JSContext *ctx, JSModuleDef *m) {
    using namespace lany::js;
    Module *_m = static_cast<Module *>(JS_GetContextOpaque(ctx));
    if (!_m) {
        spdlog::error("_m is null");
        return -1;
    }
    _m->set_export(ctx, m);
    return 0;
};

namespace lany {
namespace js {

Object::Object() {}
Object::Object(const Object &other) {
    pool = other.pool;
    entries.reserve(other.entries.size());
    for (const auto &entry : other.entries) {
        entries.emplace_back(dup_js_fn_list_entry(entry));
    }
    objects.reserve(other.objects.size());
    for (const auto &[name, obj, dcopy, pflags] : other.objects) {
        if (dcopy) {
            objects.emplace_back(dup_str(name), obj->dup(), true, pflags);
        } else {
            objects.emplace_back(dup_str(name), obj, false, pflags);
        }
    }
}
Object::Object(Object &&other) {
    pool = std::move(other.pool);
    entries = std::move(other.entries);
    objects = std::move(other.objects);
}

Object &Object::operator=(const Object &other) {
    if (this == &other) {
        return *this;
    }
    pool = other.pool;
    entries.reserve(other.entries.size());
    for (const auto &entry : other.entries) {
        entries.emplace_back(dup_js_fn_list_entry(entry));
    }
    objects.reserve(other.objects.size());
    for (const auto &[name, obj, dcopy, pflags] : other.objects) {
        if (dcopy) {
            objects.emplace_back(dup_str(name), obj->dup(), true, pflags);
        } else {
            objects.emplace_back(dup_str(name), obj, false, pflags);
        }
    }
    return *this;
}

Object &Object::operator=(Object &&other) {
    if (this == &other) {
        return *this;
    }
    pool = std::move(other.pool);
    entries = std::move(other.entries);
    objects = std::move(other.objects);
    return *this;
}

std::shared_ptr<Object> Object::dup() {
    return std::make_shared<Object>(*this);
}

std::string_view Object::dup_str(const std::string_view &str) {
    return pool.dup_safe_str(str);
}

JSCFunctionListEntry
Object::dup_js_fn_list_entry(const JSCFunctionListEntry &entry) {
    JSCFunctionListEntry dest = entry;
    dest.name = dup_str(entry.name).data();
    if (entry.def_type == JS_DEF_ALIAS) {
        dest.u.alias.name = dup_str(entry.u.alias.name).data();
    } else if (entry.def_type == JS_DEF_PROP_STRING) {
        dest.u.str = dup_str(entry.u.str).data();
    }
    // Handle other deep copy scenarios if necessary
    return dest;
}

void Object::add_fn(const std::string_view &name, JSCFunction *func,
                    uint8_t length) {
    entries.emplace_back(JS_CFUNC_DEF(dup_str(name).data(), length, func));
}

void Object::add_obj(const std::string_view &name,
                     const JSCFunctionListEntry *tab, int len,
                     uint8_t prop_flags) {
    entries.emplace_back(
        JS_OBJECT_DEF(dup_str(name).data(), tab, len, prop_flags));
}

void Object::add_obj(const std::string_view &name,
                     const std::shared_ptr<Object> &obj, bool deep_copy,
                     uint8_t prop_flags) {
    objects.emplace_back(dup_str(name), obj, deep_copy, prop_flags);
}

void Object::add_alias(const std::string_view &name,
                       const std::string_view &from, int base) {
    entries.emplace_back(
        JS_ALIAS_DEF(dup_str(name).data(), dup_str(from).data(), base));
}

JSValue Object::to_js_value(JSContext *ctx) {
    JSValue ret_obj = JS_NewObject(ctx);
    if (!entries.empty())
        JS_SetPropertyFunctionList(ctx, ret_obj, entries.data(),
                                   entries.size());
    for (const auto &[name, obj, dcopy, pflags] : objects) {
        JS_DefinePropertyValueStr(ctx, ret_obj, name.data(),
                                  obj->to_js_value(ctx), pflags);
    }
    return ret_obj;
}

Class::Class() { spdlog::debug("create class"); }
Class::Class(const Class &other) : Object(other) {
    ctor = other.ctor;
    finalizer = other.finalizer;
    gc_marker = other.gc_marker;
    class_name = other.class_name;
}
Class::Class(Class &&other) : Object(std::move(other)) {
    class_name = std::move(other.class_name);
    ctor = other.ctor;
    finalizer = other.finalizer;
    gc_marker = other.gc_marker;
    other.ctor = nullptr;
    other.finalizer = nullptr;
    other.gc_marker = nullptr;
}

Class &Class::operator=(const Class &other) {
    if (this == &other) {
        return *this;
    }
    Object::operator=(other);
    ctor = other.ctor;
    finalizer = other.finalizer;
    gc_marker = other.gc_marker;
    class_name = other.class_name;
    return *this;
}

Class &Class::operator=(Class &&other) {
    if (this == &other) {
        return *this;
    }
    Object::operator=(std::move(other));
    class_name = std::move(other.class_name);
    ctor = other.ctor;
    finalizer = other.finalizer;
    gc_marker = other.gc_marker;
    other.ctor = nullptr;
    other.finalizer = nullptr;
    other.gc_marker = nullptr;
    return *this;
}

std::shared_ptr<Object> Class::dup() { return std::make_shared<Class>(*this); }

void Class::set_ctor(JSClassCall *func) { ctor = func; }
void Class::set_finalizer(JSClassFinalizer *func) { finalizer = func; }
void Class::set_gc_marker(JSClassGCMark *func) { gc_marker = func; }

void Class::add_getset(const std::string_view &name, JSClassGetter *getter,
                       JSClassSetter *setter) {
    entries.emplace_back(JS_CGETSET_DEF(dup_str(name).data(), getter, setter));
}

JSClassID Class::get_class_id() const { return class_id; }

JSValue Class::to_js_value(JSContext *ctx) {
    if (class_id == 0) {
        JS_NewClassID(&class_id);
        auto class_def =
            JSClassDef{class_name.c_str(), finalizer, gc_marker, ctor, nullptr};
        if (JS_NewClass(JS_GetRuntime(ctx), class_id, &class_def) < 0) {
            spdlog::error("failed to register class");
            return JS_EXCEPTION;
        }
    }
    JSValue ret_obj = Object::to_js_value(ctx);
    JS_SetClassProto(ctx, get_class_id(), ret_obj);
    if (ctor != nullptr)
        JS_SetConstructorBit(ctx, ret_obj, true);
    return ret_obj;
}

Module::Module() {}
Module::Module(const Module &other) : Object(other) {}
Module::Module(Module &&other) : Object(std::move(other)) {}

Module &Module::operator=(const Module &other) {
    if (this == &other) {
        return *this;
    }
    Object::operator=(other);
    return *this;
}
Module &Module::operator=(Module &&other) {
    if (this == &other) {
        return *this;
    }
    Object::operator=(std::move(other));
    return *this;
}

void Module::set_export(JSContext *ctx, JSModuleDef *m) {
    for (const auto &entry : entries) {
        spdlog::debug("export: {}", entry.name);
    }
    if (!entries.empty()) {
        JS_SetModuleExportList(ctx, m, entries.data(), entries.size());
    }
    for (const auto &[name, obj, dcopy, pflags] : objects) {
        spdlog::debug("export: {}", name);
        JS_SetModuleExport(ctx, m, name.data(), obj->to_js_value(ctx));
    }
    spdlog::debug("export count: {}", entries.size() + objects.size());
}

JSModuleDef *Module::init_module(JSContext *ctx,
                                 const std::string_view &module_name) {
    spdlog::debug("init c module name: \"{}\"", module_name);
    JS_SetContextOpaque(ctx, this);
    JSModuleDef *m = JS_NewCModule(ctx, module_name.data(), module_init_helper);
    if (m == nullptr) {
        spdlog::error("failed to create module");
        return nullptr;
    }
    // add export
    if (!entries.empty())
        JS_AddModuleExportList(ctx, m, entries.data(), entries.size());
    for (const auto &[name, obj, dcopy, pflags] : objects) {
        JS_AddModuleExport(ctx, m, name.data());
    }
    return m;
}

bool is_buildin_module(const std::string &name) {
    return module_map.find(name) != module_map.end();
}
JSModuleDef *load_module(JSContext *ctx, const std::string &name) {
    if (!is_buildin_module(name)) {
        JS_ThrowReferenceError(ctx, "buildin c module %s not found",
                               name.c_str());
        return nullptr;
    }
    return module_map[name].init_module(ctx, name);
}

void register_module(const std::string &name, const Module &module) {
    spdlog::info("register c module: {}", util::quote(name));
    if (is_buildin_module(name)) {
        spdlog::warn("buildin module {} already registered", name);
        return;
    }
    module_map.emplace(name, module);
}
void unregister_module(const std::string &name) {
    spdlog::info("unregister c module: {}", util::quote(name));
    module_map.erase(name);
}
} // namespace js
} // namespace lany