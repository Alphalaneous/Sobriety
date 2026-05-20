#include "WaitingPopup.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

WaitingPopup* WaitingPopup::create(const std::string& text) {
    auto ret = new WaitingPopup();
    if (ret->init(text)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool WaitingPopup::init(const std::string& text) {
    if (!Popup::init(300, 100)) return false;
    m_bgSprite->setVisible(false);
    m_noElasticity = true;
    m_closeBtn->removeFromParent();
    setOpacity(180);
    
    setKeyboardEnabled(true);
    setKeypadEnabled(true);
    setTouchEnabled(true);

    auto container = CCNode::create();
    container->setAnchorPoint({0.5f, 0.5f});
    container->setContentSize({m_mainLayer->getContentWidth(), 50});
    container->setPosition(m_mainLayer->getContentSize()/2);

    auto layout = RowLayout::create();
    layout->setGap(15);
    layout->setAutoScale(false);
    layout->setAxisReverse(true);
    container->setLayout(layout);

    m_mainLayer->addChild(container);

    auto label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    label->setScale(0.7f);
    container->addChild(label);

    auto spinner = LoadingSpinner::create(30);
    spinner->setAnchorPoint({1.f, 0.5f});

    container->addChild(spinner);

    container->updateLayout();

    return true;
}

void WaitingPopup::keyBackClicked() {}

void WaitingPopup::keyDown(cocos2d::enumKeyCodes key, double p1) {}

void WaitingPopup::onClose(CCObject*) {}