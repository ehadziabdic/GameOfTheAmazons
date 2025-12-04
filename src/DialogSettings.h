#pragma once

#include <gui/Dialog.h>
#include "SettingsPopup.h"

class DialogSettings : public gui::Dialog {
protected:
    SettingsPopup _settingsView;
    std::function<void(BoardDimension)> _boardChangedHandler;
    std::function<void(Difficulty)> _difficultyChangedHandler;
    std::function<void(BoardStyle)> _boardStyleChangedHandler;

    bool onClick(gui::Dialog::Button::ID btnID, gui::Button* /*pButton*/) override {
        if (btnID == gui::Dialog::Button::ID::OK) {
            if (_boardChangedHandler) {
                _boardChangedHandler(_settingsView.currentBoardSelection());
            }
            if (_difficultyChangedHandler) {
                _difficultyChangedHandler(_settingsView.currentDifficultySelection());
            }
            if (_boardStyleChangedHandler) {
                _boardStyleChangedHandler(_settingsView.currentBoardStyleSelection());
            }
        }
        return true;
    }
public:
    DialogSettings(gui::Frame* pFrame, td::UINT4 wndID = 0)
    : gui::Dialog(pFrame,
                  { {gui::Dialog::Button::ID::OK, tr("Ok"), gui::Button::Type::Default},
                    {gui::Dialog::Button::ID::Cancel, tr("Cancel")} },
                  gui::Size(420, 160), wndID)
    {
        setTitle(tr("settings"));
        setCentralView(&_settingsView);
    }

    void setBoardChangedHandler(std::function<void(BoardDimension)> handler) { _boardChangedHandler = std::move(handler); }
    void setDifficultyChangedHandler(std::function<void(Difficulty)> handler) { _difficultyChangedHandler = std::move(handler); }
    void setBoardStyleChangedHandler(std::function<void(BoardStyle)> handler) { _boardStyleChangedHandler = std::move(handler); }

    void syncSelections(BoardDimension board, Difficulty difficulty, BoardStyle style) {
        _settingsView.syncSelections(board, difficulty, style);
    }
};
