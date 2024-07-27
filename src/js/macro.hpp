#pragma once

#include <quickjs.h>

// #undef JS_MKVAL
// #define JS_MKVAL(tag, val) \
//     JSValue { JSValueUnion{val}, tag }

// inline JSValue JS_MKVAL(uint8_t tag, int32_t val) {
//     return JSValue{JSValueUnion{.int32 = val}, tag};
// }

// Undefine the existing macros
#undef JS_CFUNC_DEF
#undef JS_CFUNC_MAGIC_DEF
#undef JS_CFUNC_SPECIAL_DEF
#undef JS_ITERATOR_NEXT_DEF
#undef JS_CGETSET_DEF
#undef JS_CGETSET_MAGIC_DEF
#undef JS_PROP_STRING_DEF
#undef JS_PROP_INT32_DEF
#undef JS_PROP_INT64_DEF
#undef JS_PROP_DOUBLE_DEF
#undef JS_PROP_UNDEFINED_DEF
#undef JS_OBJECT_DEF
#undef JS_ALIAS_DEF
#undef JS_ALIAS_BASE_DEF

#undef JS_MKVAL

inline JSValue JS_MKVAL(uint8_t tag, int32_t val) {
    return {{.int32 = val}, tag};
}

// Define the same functionality using C++ constructs
inline JSCFunctionListEntry JS_CFUNC_DEF(const char *name, uint8_t length,
                                         JSCFunction *func1) {
    return {name,
            JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,
            JS_DEF_CFUNC,
            0,
            {.func = {length, JS_CFUNC_generic, {.generic = func1}}}};
}

inline JSCFunctionListEntry JS_CFUNC_MAGIC_DEF(const char *name, uint8_t length,
                                               JSCFunctionMagic *func1,
                                               int16_t magic) {
    return {
        name,
        JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,
        JS_DEF_CFUNC,
        magic,
        {.func = {length, JS_CFUNC_generic_magic, {.generic_magic = func1}}}};
}

inline JSCFunctionListEntry JS_ITERATOR_NEXT_DEF(const char *name,
                                                 uint8_t length,
                                                 JSCFunctionType func1,
                                                 int16_t magic) {
    return {name,
            JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,
            JS_DEF_CFUNC,
            magic,
            {.func = {length, JS_CFUNC_iterator_next, func1}}};
}

using JSClassGetter = JSValue(JSContext *ctx, JSValueConst this_val);
using JSClassSetter = JSValue(JSContext *ctx, JSValueConst this_val,
                              JSValueConst val);

inline JSCFunctionListEntry JS_CGETSET_DEF(const char *name,
                                           JSClassGetter *fgetter,
                                           JSClassSetter *fsetter) {
    return {
        name,
        JS_PROP_CONFIGURABLE,
        JS_DEF_CGETSET,
        0,
        {.getset = {.get = {.getter = fgetter}, .set = {.setter = fsetter}}}};
}

inline JSCFunctionListEntry
JS_PROP_STRING_DEF(const char *name, const char *cstr, uint8_t prop_flags) {
    return {name, prop_flags, JS_DEF_PROP_STRING, 0, {.str = cstr}};
}

inline JSCFunctionListEntry JS_PROP_INT32_DEF(const char *name, int32_t val,
                                              uint8_t prop_flags) {
    return {name, prop_flags, JS_DEF_PROP_INT32, 0, {.i32 = val}};
}

inline JSCFunctionListEntry JS_PROP_INT64_DEF(const char *name, int64_t val,
                                              uint8_t prop_flags) {
    return {name, prop_flags, JS_DEF_PROP_INT64, 0, {.i64 = val}};
}

inline JSCFunctionListEntry JS_PROP_DOUBLE_DEF(const char *name, double val,
                                               uint8_t prop_flags) {
    return {name, prop_flags, JS_DEF_PROP_DOUBLE, 0, {.f64 = val}};
}

inline JSCFunctionListEntry JS_PROP_UNDEFINED_DEF(const char *name,
                                                  uint8_t prop_flags) {
    return {name, prop_flags, JS_DEF_PROP_UNDEFINED, 0, {.i32 = 0}};
}

inline JSCFunctionListEntry JS_OBJECT_DEF(const char *name,
                                          const JSCFunctionListEntry *tab,
                                          int len, uint8_t prop_flags) {
    return {name, prop_flags, JS_DEF_OBJECT, 0, {.prop_list = {tab, len}}};
}

inline JSCFunctionListEntry JS_ALIAS_DEF(const char *name, const char *from,
                                         int base = -1) {
    return {name,
            JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE,
            JS_DEF_ALIAS,
            0,
            {.alias = {from, base}}};
}
