#pragma once
#include <gui/Window.h>
#include "MainView.h"
#include "Algorithms.h"
#include "Rules.h"
#include "ToolBarMain.h"

class MainWindow : public gui::Window {
protected:
    MainView view;
    ToolBarMain _toolBar;
public:
    MainWindow() : gui::Window(gui::Geometry(50, 50, 1440, 800)) {
        setTitle(tr("appTitle"));
            setToolBar(_toolBar);
            // Wire MainView toolbar state notifications to the window toolbar
            view.setToolbarStateHandler([this](bool allowChanges, bool allowUndo){
                // New game item: menu 20 action 10
                gui::ToolBarItem* pNew = _toolBar.getItem(20, 0, 0, 10);
                if (pNew) pNew->enable(allowChanges);
                // Undo item: menu 20 action 11
                gui::ToolBarItem* pUndo = _toolBar.getItem(20, 0, 0, 11);
                if (pUndo) pUndo->enable(allowUndo);
                // Cancel item: menu 20 action 12 (enabled when AI is running -> !allowChanges)
                gui::ToolBarItem* pCancel = _toolBar.getItem(20, 0, 0, 12);
                if (pCancel) pCancel->enable(!allowChanges);
            });
        setCentralView(&view);
    }

    void onInitialAppearance() override {
        view.focusBoard();
    }
    
    bool onActionItem(gui::ActionItemDescriptor& aiDesc) override
    {
        auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
        if (menuID == 10 && actionID == 10) {
            gui::Sound::play(gui::Sound::Type::SelectionChanged);
            view.openSettingsDialog();
            return true;
        }
        if (menuID == 20 && firstSubMenuID == 0 && lastSubMenuID == 0) {
            switch (actionID) {
                case 10:
                    gui::Sound::play(gui::Sound::Type::SelectionChanged);
                    view.triggerNewGameFromToolbar();
                    return true;
                case 11:
                    gui::Sound::play(gui::Sound::Type::SelectionChanged);
                    view.triggerUndoFromToolbar();
                    return true;
                    case 12:
                        gui::Sound::play(gui::Sound::Type::SelectionChanged);
                        view.cancelAiFromToolbar();
                        return true;
                case 20:
                    gui::Sound::play(gui::Sound::Type::SelectionChanged);
                    view.setGameDifficulty(Difficulty::Easy);
                    return true;
                case 21:
                    gui::Sound::play(gui::Sound::Type::SelectionChanged);
                    view.setGameDifficulty(Difficulty::Medium);
                    return true;
                case 22:
                    gui::Sound::play(gui::Sound::Type::SelectionChanged);
                    view.setGameDifficulty(Difficulty::Hard);
                    return true;
            }
        }
        return false;
    }
};

