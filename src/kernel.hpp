#pragma once

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long long;
using uptr = u64;

template <typename T> auto inline ptr(void* p) -> T* {
    return reinterpret_cast<T*>(p);
}

template <typename T> auto inline ptr(void const* p) -> T const* {
    return reinterpret_cast<T const*>(p);
}

template <typename T> auto inline ptr(uptr p) -> T* {
    return reinterpret_cast<T*>(p);
}

auto constexpr CACHE_LINE_SIZE = 64u;
// note: almost all modern x86_64 processors (intel and amd)

auto constexpr CORE_STACK_SIZE_PAGES = 2 * 1024 * 1024 / 4096u;

template <typename T>
auto inline atomic_compare_exchange_acquire_relaxed(T* target, T& expected,
                                                    T desired, bool weak)
    -> bool {
    return __atomic_compare_exchange_n(
        target,           // pointer to the object to modify
        &expected,        // pointer to the value we expect to find
        desired,          // the value we want to write if expected matches
        weak,             // 'weak' = false (use strong version/lock prefix)
        __ATOMIC_ACQUIRE, // success memory order
        __ATOMIC_RELAXED  // failure memory order
    );
}

template <typename T> auto inline atomic_add(T* target, T delta) -> void {
    __atomic_fetch_add(target, delta, __ATOMIC_SEQ_CST);
}

template <typename T>
auto inline atomic_add_release(T* target, T delta) -> void {
    __atomic_fetch_add(target, delta, __ATOMIC_RELEASE);
}

template <typename T>
auto inline atomic_add_relaxed(T* target, T delta) -> void {
    __atomic_fetch_add(target, delta, __ATOMIC_RELAXED);
}

template <typename T> auto inline atomic_load_acquire(T const* target) -> T {
    return __atomic_load_n(target, __ATOMIC_ACQUIRE);
}

template <typename T> auto inline atomic_load_relaxed(T const* target) -> T {
    return __atomic_load_n(target, __ATOMIC_RELAXED);
}

template <typename T>
auto inline atomic_store_release(T* target, T val) -> void {
    __atomic_store_n(target, val, __ATOMIC_RELEASE);
}

template <typename T>
auto inline atomic_store_relaxed(T* target, T val) -> void {
    __atomic_store_n(target, val, __ATOMIC_RELAXED);
}

auto inline interrupts_enable() -> void { asm volatile("sti"); }

[[noreturn]] auto kernel_start() -> void;

namespace osca {

[[noreturn]] auto start() -> void;
[[noreturn]] auto run_core(u32 core_index) -> void;
auto on_keyboard(u8 scancode) -> void;
auto on_timer() -> void;

} // namespace osca
