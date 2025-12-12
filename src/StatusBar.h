#pragma once
#include <gui/StatusBar.h>
#include <gui/Label.h>
#include <gui/HorizontalLayout.h>

class StatusBar : public gui::StatusBar {
protected:
    gui::Label _lblStatus;

public:
    StatusBar()
        : gui::StatusBar(3)
        , _lblStatus(tr("statusSelectQueen"))
    {
        _lblStatus.setFont(gui::Font::ID::SystemBold);
        
        setMargins(4, 0, 4, 4);
        _layout.setSpaceBetweenCells(0);
        _layout << _lblStatus;
        _layout.appendSpacer();
        
        setLayout(&_layout);
    }

    void setStatusText(const td::String& text) {
        _lblStatus.setTitle(text);
    }
};
