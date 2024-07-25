
#include "module.hpp"

#include <map>

#include <spdlog/spdlog.h>

static std::map<std::string_view,
                JSModuleDef *(*)(JSContext *ctx, const char *module_name)>
    module_map;

namespace lany {
namespace js {
bool is_buildin_module(const std::string_view &name) {
    return module_map.find(name) != module_map.end();
}
JSModuleDef *load_module(JSContext *ctx, const std::string_view &name) {
    if (!is_buildin_module(name)) {
        JS_ThrowReferenceError(ctx, "buildin module %s not found", name.data());
        return nullptr;
    }
    return module_map[name](ctx, name.data());
}

void register_module(const std::string_view &name,
                     JSModuleDef *(*func)(JSContext *ctx,
                                          const char *module_name)) {
    spdlog::info("register module {}", name);
    if (is_buildin_module(name)) {
        spdlog::warn("buildin module {} already registered", name);
        return;
    }
    module_map[name] = func;
}
void unregister_module(const std::string_view &name) {
    spdlog::info("unregister module {}", name);
    module_map.erase(name);
}
} // namespace js
} // namespace lany