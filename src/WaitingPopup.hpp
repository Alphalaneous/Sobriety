#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class WaitingPopup : public geode::Popup {
public:
    static WaitingPopup* create(const std::string& text);
protected:
    bool init(const std::string& text);

    void keyBackClicked() override;
    void keyDown(cocos2d::enumKeyCodes key, double p1) override;
    void onClose(CCObject*) override;
};