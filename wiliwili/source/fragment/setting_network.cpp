//
// Created by fang on 2022/9/19.
//

#include "fragment/setting_network.hpp"
#include "bilibili.h"
#include "bilibili/result/home_result.h"
#include "bilibili/result/setting.h"
#include "utils/number_helper.hpp"
#include "utils/config_helper.hpp"

using namespace brls::literals;

SettingNetwork::SettingNetwork() {
    this->inflateFromXMLRes("xml/fragment/setting_network.xml");
    brls::Logger::debug("Fragment SettingNetwork: create");
    this->networkTest();
    this->getUnixTime();

    if (brls::Application::getPlatform()->hasWirelessConnection()) {
        labelWIFI->setTextColor(nvgRGB(72, 154, 83));
        labelWIFI->setText("hints/on"_i18n);
    } else {
        labelWIFI->setTextColor(nvgRGB(199, 84, 80));
        labelWIFI->setText("hints/off"_i18n);
    }
    labelIP->setText(brls::Application::getPlatform()->getIpAddress());
    labelDNS->setText(brls::Application::getPlatform()->getDnsServer());

    headerTest->setSubtitle(APPVersion::instance().git_tag);
}

void SettingNetwork::networkTest() {
    ASYNC_RETAIN
    bilibili::BilibiliClient::get_recommend(
        1, 1,
        [ASYNC_TOKEN](const auto& result) {
            brls::sync([ASYNC_TOKEN]() {
                ASYNC_RELEASE
                this->labelTest1->setTextColor(nvgRGB(72, 154, 83));
                this->labelTest1->setText("hints/success"_i18n);
            });
        },
        [ASYNC_TOKEN](const std::string& error) {
            brls::sync([ASYNC_TOKEN]() {
                ASYNC_RELEASE
                this->labelTest1->setTextColor(nvgRGB(199, 84, 80));
                this->labelTest1->setText("hints/failed"_i18n);
            });
        });
}

void SettingNetwork::getUnixTime() {
    // 设置系统时间
    this->labelSysTime->setText(
        wiliwili::sec2FullDate(wiliwili::getUnixTime()));

    // 获取网络时间
    ASYNC_RETAIN
    bilibili::BilibiliClient::get_unix_time(
        [ASYNC_TOKEN](const bilibili::UnixTimeResult& result) {
            brls::sync([ASYNC_TOKEN, result]() {
                ASYNC_RELEASE
                this->labelNetTime->setText(wiliwili::sec2FullDate(result.now));
            });
        },
        [ASYNC_TOKEN](const std::string& error) {
            brls::sync([ASYNC_TOKEN]() {
                ASYNC_RELEASE
                this->labelNetTime->setTextColor(nvgRGB(199, 84, 80));
                this->labelNetTime->setText("hints/failed"_i18n);
            });
        });
}

SettingNetwork::~SettingNetwork() {
    brls::Logger::debug("Fragment SettingNetwork: delete");
}

brls::View* SettingNetwork::create() { return new SettingNetwork(); }