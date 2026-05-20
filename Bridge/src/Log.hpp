#pragma once

#include <fmt/format.h>
#include <iostream>
#include <matjson.hpp>
#include "Client.hpp"

namespace bridge::log {

    template <typename... Args>
    inline void logImpl(std::string_view type, fmt::format_string<Args...> str, Args&&... args) {
        matjson::Value json;
        json["type"] = type;
        json["message"] = fmt::vformat(str, fmt::make_format_args(args...));
        Client::get()->sendMessage(json.dump());
        std::cout << fmt::format("[{}] {}", type, json["message"].asString().unwrapOrDefault()) << std::endl;
    }

    template <typename... Args>
    inline void info(fmt::format_string<Args...> str, Args&&... args) {
        logImpl("info", str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void warn(fmt::format_string<Args...> str, Args&&... args) {
        logImpl("warn", str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void error(fmt::format_string<Args...> str, Args&&... args) {
        logImpl("error", str, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void debug(fmt::format_string<Args...> str, Args&&... args) {
        logImpl("debug", str, std::forward<Args>(args)...);
    }
}