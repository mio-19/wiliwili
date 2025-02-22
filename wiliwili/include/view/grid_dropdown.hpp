//
// Created by fang on 2022/7/9.
//

// register this view in main.cpp
//#include "view/grid_dropdown.hpp"
//    brls::Application::registerXMLView("GridDropdown", GridDropdown::create);
// <brls:View xml=@res/xml/views/grid_dropdown.xml

#pragma once

#include <cmath>
#include <borealis.hpp>
#include <borealis/views/cells/cell_radio.hpp>
#include <utility>
#include "view/recycling_grid.hpp"

typedef brls::Event<int> ValueSelectedEvent;

//class GridDropdown;
class BaseDropdown;

class GridRadioCell : public RecyclingGridItem {
public:
    GridRadioCell();

    void setSelected(bool selected);

    bool getSelected();

    BRLS_BIND(brls::Label, title, "brls/rediocell/title");
    BRLS_BIND(brls::CheckBox, checkbox, "brls/rediocell/checkbox");

    static RecyclingGridItem* create();

private:
    bool selected = false;
};

class DataSourceDropdown : public RecyclingGridDataSource {
public:
    DataSourceDropdown(BaseDropdown* view) : dropdown(view) {}

    void onItemSelected(RecyclingGrid* recycler, size_t index) override;

protected:
    BaseDropdown* dropdown;
};

class TextDataSourceDropdown : public DataSourceDropdown {
public:
    TextDataSourceDropdown(std::vector<std::string> result, BaseDropdown* view)
        : DataSourceDropdown(view), data(std::move(result)) {}

    RecyclingGridItem* cellForRow(RecyclingGrid* recycler,
                                  size_t index) override;

    size_t getItemCount() override;

    void clearData() override;

private:
    std::vector<std::string> data;
};

class BaseDropdown : public brls::Box {
public:
    BaseDropdown(const std::string& title, ValueSelectedEvent::Callback cb,
                 int selected = 0);

    RecyclingGrid* getRecyclingList();

    void setDataSource(DataSourceDropdown* dataSource);

    void show(std::function<void(void)> cb, bool animate,
              float animationDuration) override;

    void hide(std::function<void(void)> cb, bool animated,
              float animationDuration) override;

    virtual View* getParentNavigationDecision(
        View* from, View* newFocus, brls::FocusDirection direction) override;

    brls::Event<RecyclingGridItem*>* getCellFocusDidChangeEvent();

    bool isTranslucent() override;

    size_t getSelected() const;

    ValueSelectedEvent::Callback getSelectCallback();

    static void text(const std::string& title,
                     const std::vector<std::string>& values,
                     ValueSelectedEvent::Callback cb, int selected = 0);

protected:
    BRLS_BIND(RecyclingGrid, recycler, "grid_dropdown/recycler");
    BRLS_BIND(brls::Box, header, "grid_dropdown/header");
    BRLS_BIND(brls::Label, title, "grid_dropdown/title_label");
    BRLS_BIND(brls::Box, content, "grid_dropdown/content");
    BRLS_BIND(brls::AppletFrame, applet, "grid_dropdown/applet");

    ValueSelectedEvent::Callback cb;
    size_t selected;
    brls::Animatable showOffset = 0;
    brls::Event<RecyclingGridItem*> cellFocusDidChangeEvent;

    void offsetTick() { content->setTranslationY(showOffset); }
};