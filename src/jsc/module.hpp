
#pragma once

#include <string_view>

#include <quickjs.h>

namespace lany {
namespace js {
bool is_buildin_module(const std::string_view &name);
JSModuleDef *load_module(JSContext *ctx, const std::string_view &name);
void register_module(const std::string_view &name,
                     JSModuleDef *(*func)(JSContext *ctx,
                                          const char *module_name));
void unregister_module(const std::string_view &name);
} // namespace js
} // namespace lany