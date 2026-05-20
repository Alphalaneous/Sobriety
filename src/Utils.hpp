#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace sobriety::utils {

    template<class T>
    struct Singleton {
        static T* get() {
            static T instance;
            return &instance;
        }
    };

    inline geode::Severity fromString(std::string_view severity) {
        if (severity == "debug") return geode::Severity::Debug;
        if (severity == "info") return geode::Severity::Info;
        if (severity == "warning") return geode::Severity::Warning;
        if (severity == "error") return geode::Severity::Error;
        return geode::Severity::Info;
    }

    inline geode::Severity getConsoleLogLevel() {
        static auto geode = geode::Loader::get()->getLoadedMod("geode.loader");
        static auto setting = fromString(geode->getSettingValue<std::string>("console-log-level"));
        static auto listener = listenForSettingChanges<std::string>("console-log-level", [](std::string value) {
            setting = fromString(value);
        }, geode);

        return setting;
    }

    inline bool shouldLogMillisconds() {
        static auto geode = geode::Loader::get()->getLoadedMod("geode.loader");
        static auto setting = geode->getSettingValue<bool>("log-milliseconds");
        static auto listener = listenForSettingChanges<bool>("log-milliseconds", [](bool value) {
            setting = value;
        }, geode);

        return setting;
    }

    inline int getFontSize() {
        static auto setting = geode::Mod::get()->getSettingValue<int>("console-font-size");
        return setting;
    }

    inline cocos2d::ccColor3B getConsoleForegroundColor(std::function<void()> callback) {
        static auto setting = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("console-foreground-color");
        static auto listener = geode::listenForSettingChanges<cocos2d::ccColor3B>("console-foreground-color", [callback = std::move(callback)](ccColor3B value) {
            setting = value;
            if (callback) callback();
        });
        return setting;
    }

    inline cocos2d::ccColor3B getConsoleBackgroundColor(std::function<void()> callback) {
        static auto setting = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("console-background-color");
        static auto listener = geode::listenForSettingChanges<cocos2d::ccColor3B>("console-background-color", [callback = std::move(callback)](ccColor3B value) {
            setting = value;
            if (callback) callback();
        });
        return setting;
    }

    inline cocos2d::ccColor3B getLogInfoColor(std::function<void()> callback) {
        static auto setting = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("console-log-info-color");
        static auto listener = geode::listenForSettingChanges<ccColor3B>("console-log-info-color", [callback = std::move(callback)](ccColor3B value) {
            setting = value;
            if (callback) callback();
        });
        return setting;
    }

    inline cocos2d::ccColor3B getLogWarnColor(std::function<void()> callback) {
        static auto setting = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("console-log-warn-color");
        static auto listener = geode::listenForSettingChanges<cocos2d::ccColor3B>("console-log-warn-color", [callback = std::move(callback)](cocos2d::ccColor3B value) {
            setting = value;
            if (callback) callback();
        });
        return setting;
    }

    inline cocos2d::ccColor3B getLogErrorColor(std::function<void()> callback) {
        static auto setting = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("console-log-error-color");
        static auto listener = geode::listenForSettingChanges<cocos2d::ccColor3B>("console-log-error-color", [callback = std::move(callback)](cocos2d::ccColor3B value) {
            setting = value;
            if (callback) callback();
        });
        return setting;
    }

    inline cocos2d::ccColor3B getLogDebugColor(std::function<void()> callback) {
        static auto setting = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("console-log-debug-color");
        static auto listener = geode::listenForSettingChanges<cocos2d::ccColor3B>("console-log-debug-color", [callback = std::move(callback)](cocos2d::ccColor3B value) {
            setting = value;
            if (callback) callback();
        });
        return setting;
    }

    inline bool hasConsole() {
        static auto geode = geode::Loader::get()->getLoadedMod("geode.loader");
        static bool setting = geode->getSettingValue<bool>("show-platform-console");
        return setting;
    }
}