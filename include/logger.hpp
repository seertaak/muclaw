#pragma once

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

#include <chrono>
#include <source_location>

namespace nclaw::log {

inline auto current_time() { return std::chrono::system_clock::now(); }

template <typename... Args>
auto info(fmt::format_string<Args...> fmt, Args&&... args) -> void {
    fmt::print(fg(fmt::color::cyan), "[{:%Y-%m-%d %H:%M:%S}] [INFO] ", current_time());
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
}

template <typename... Args>
auto warn(fmt::format_string<Args...> fmt, Args&&... args) -> void {
    fmt::print(fg(fmt::color::yellow), "[{:%Y-%m-%d %H:%M:%S}] [WARN] ", current_time());
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
}

template <typename... Args>
auto error(fmt::format_string<Args...> fmt, Args&&... args) -> void {
    fmt::print(fg(fmt::color::red), "[{:%Y-%m-%d %H:%M:%S}] [ERROR] ", current_time());
    fmt::print(fmt, std::forward<Args>(args)...);
    fmt::print("\n");
}

} // namespace nclaw::log
