#include <stdint.h>

#define elementsof(arr) (sizeof(arr) / sizeof(arr[0]))

#define _MACRO_CAT(arg1, arg2) arg1 ## arg2
#define MACRO_CAT(arg1, arg2) _MACRO_CAT(arg1, arg2)
#define MACRO_CAT2(arg1, arg2, ...) _MACRO_CAT(arg1, arg2) __VA_ARGS__
#define MACRO_CATW_RAW(arg1, arg2, arg3) arg1 ## arg2 ## arg3
#define MACRO_CATW(arg1, arg2, arg3) MACRO_CATW_RAW(arg1, arg2, arg3)
#define _MACRO_STR(arg) #arg
#define MACRO_STR(arg) _MACRO_STR(arg)
#define MACRO_EVAL(...) __VA_ARGS__

#define unknown_fields(size)		unsigned char unknown_name[size]
#define ValidateFieldOffset(offset, struct_type, member_name) \
static_assert((offset) == __builtin_offsetof(struct_type, member_name), "Incorrect struct offset! Offset of " MACRO_STR(struct_type) "."#member_name" is not "#offset)

enum CallType {
    Cdecl, // cdecl and CDECL are both predefined. :P
    Stdcall,
    Fastcall,
    Thiscall,
    Vectorcall
};

template <uintptr_t addr, CallType type, typename R = void, typename... Args>
static inline R asm_call(Args... args) {
    if constexpr (type == Cdecl) {
        auto* func = (R(__cdecl*)(Args...))addr;
        return func(args...);
    }
    else if constexpr (type == Stdcall) {
        auto* func = (R(__stdcall*)(Args...))addr;
        return func(args...);
    }
    else if constexpr (type == Fastcall) {
        auto* func = (R(__fastcall*)(Args...))addr;
        return func(args...);
    }
    else if constexpr (type == Vectorcall) {
        auto* func = (R(__vectorcall*)(Args...))addr;
        return func(args...);
    }
    else if constexpr (type == Thiscall) {
        auto* func = (R(__thiscall*)(Args...))addr;
        return func(args...);
    }
}
