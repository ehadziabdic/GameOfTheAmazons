#pragma once

#include <gui/ToolBar.h>
#include <gui/Image.h>

class ToolBarMain : public gui::ToolBar {
    gui::Image _imgNew;
    gui::Image _imgUndo;
    gui::Image _imgSettings;
    gui::Image _imgCancel;
    gui::Image _imgEasy;
    gui::Image _imgMedium;
    gui::Image _imgHard;
public:
    ToolBarMain()
    : gui::ToolBar("mainTB", 7)
    , _imgNew(":reset")
    , _imgUndo(":undo")
    , _imgSettings(":settings")
    , _imgCancel(":cancle")
    , _imgEasy(":easy")
    , _imgMedium(":medium")
    , _imgHard(":hard")
    {
        addItem(tr("settings"), &_imgSettings, tr("settingsTT"), 10, 0, 0, 10);
        addSpaceItem();
        addItem(tr("Easy"), &_imgEasy, tr("EasyTT"), 20, 0, 0, 20);
        addItem(tr("Medium"), &_imgMedium, tr("MediumTT"), 20, 0, 0, 21);
        addItem(tr("Hard"), &_imgHard, tr("HardTT"), 20, 0, 0, 22);
        addSpaceItem();
        addItem(tr("newGame"), &_imgNew, tr("newGameTT"), 20, 0, 0, 10);
        addItem(tr("undo"), &_imgUndo, tr("undoTT"), 20, 0, 0, 11);
        addItem(tr("cancel"), &_imgCancel, tr("cancelTT"), 20, 0, 0, 12);
    }
};
