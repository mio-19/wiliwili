//
// Created by fang on 2022/9/17.
//

#include "view/svg_image.hpp"
#include "borealis/core/cache_helper.hpp"

SVGImage::SVGImage() {
    this->registerFilePathXMLAttribute(
        "SVG", [this](std::string value) { this->setImageFromSVGFile(value); });

    // 交给缓存自动处理纹理的删除
    this->setFreeTexture(false);

    // 改变窗口大小时自动更新纹理
    subscription =
        brls::Application::getWindowSizeChangedEvent()->subscribe([this]() {
            if (!filePath.empty()) {
                brls::Visibility v = getVisibility();
                this->setVisibility(brls::Visibility::VISIBLE);
                setImageFromSVGFile(filePath);
                this->setVisibility(v);
            }
        });
}

void SVGImage::setImageFromSVGRes(std::string name) {
    this->setImageFromSVGFile(std::string(BRLS_RESOURCES) + name);
}

void SVGImage::setImageFromSVGFile(const std::string value) {
    filePath   = value;
    size_t tex = this->getTexture();
    if (tex > 0) brls::TextureCache::instance().removeCache(tex);

    tex = brls::TextureCache::instance().getCache(value);
    if (tex > 0) {
        brls::Logger::verbose("cache hit: {} {}", value, tex);
        this->innerSetImage(tex);
        return;
    }

    this->document = lunasvg::Document::loadFromFile(value.c_str());
    this->updateBitmap();

    tex = this->getTexture();
    if (tex > 0) {
        brls::Logger::verbose("cache svg: {} {}", value, tex);
        brls::TextureCache::instance().addCache(value, tex);
    }
}

void SVGImage::setImageFromSVGString(const std::string value) {
    this->document = lunasvg::Document::loadFromData(value);
    this->updateBitmap();
}

void SVGImage::updateBitmap() {
    float width  = this->getWidth() * brls::Application::windowScale;
    float height = this->getHeight() * brls::Application::windowScale;
    auto bitmap  = this->document->renderToBitmap(width, height);
    bitmap.convertToRGBA();
    NVGcontext* vg = brls::Application::getNVGContext();
    int tex        = nvgCreateImageRGBA(vg, bitmap.width(), bitmap.height(), 0,
                                        bitmap.data());
    this->innerSetImage(tex);
}

SVGImage::~SVGImage() {
    size_t tex = this->getTexture();
    if (tex > 0) brls::TextureCache::instance().removeCache(tex);
    brls::Application::getWindowSizeChangedEvent()->unsubscribe(subscription);
}

brls::View* SVGImage::create() { return new SVGImage(); }