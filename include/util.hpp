#if !defined(__UTIL__H)
#define __UTIL__H

#include <fmt/core.h>
#include <fstream>
#include <optional>
#include <regex>
#include <tuple>

static const char* file = "/etc/shadow";

template <typename Str, typename... Args>
void log(Str str, Args&&... args) {
    fmt::print(str, std::forward<Args>(args)...);
}

template <typename Str, typename... Args>
void logln(Str str, Args&&... args) {
    fmt::print(fmt::format("{}\n", str), std::forward<Args>(args)...);
}

auto get_secret(const std::string& username) -> std::optional<std::pair<std::string, std::string>> {
    std::ifstream infile(file);
    if (infile.good()) {

        std::regex regex("^(\\w*):\\$(.*)\\$([^:]*):(?:.*)");
        std::smatch matches;

        std::string line{};
        while (infile) {
            std::getline(infile, line);
            if (line.find(username) != std::string::npos) {
                std::regex_search(line, matches, regex);
                return {{fmt::format("${}$", matches[2].str()), matches[3].str()}};
            }
        }
        return std::nullopt;
    } else
        throw std::runtime_error("Failed to open file");
}

#endif // __UTIL__H
