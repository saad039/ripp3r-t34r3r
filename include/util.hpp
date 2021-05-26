#if !defined(__UTIL__H)
#define __UTIL__H

#include <fmt/core.h>

template <typename Str, typename... Args>
void log(Str str, Args&&... args) {
    fmt::print(str, std::forward<Args>(args)...);
}

template <typename Str, typename... Args>
void logln(Str str, Args&&... args) {
    fmt::print(fmt::format("{}\n", str), std::forward<Args>(args)...);
}

#endif // __UTIL__H
