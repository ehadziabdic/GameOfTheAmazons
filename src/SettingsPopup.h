#pragma once

#include <gui/Application.h>
#include <gui/Alert.h>
#include <gui/ComboBox.h>
#include <gui/GridComposer.h>
#include <gui/GridLayout.h>
#include <gui/Image.h>
#include <gui/PopupView.h>
#include <gui/Label.h>
#include <td/String.h>

#include "GameState.h"

#include <algorithm>
#include <array>
#include <functional>
#include <utility>

class SettingsPopup : public gui::PopupView {
public:
    SettingsPopup();

    void setBoardChangedHandler(std::function<void(BoardDimension)> handler);
    void setBoardStyleChangedHandler(std::function<void(BoardStyle)> handler);
    void setDifficultyChangedHandler(std::function<void(Difficulty)> handler);
    void syncSelections(BoardDimension board, Difficulty difficulty, BoardStyle style);
    // Accessors for dialog integration
    BoardDimension currentBoardSelection() const;
    Difficulty currentDifficultySelection() const;
    BoardStyle currentBoardStyleSelection() const;

private:
    struct LanguageOption {
        const char* extension;
        const char* translationKey;
    };

    gui::Image _imgButton;
    gui::Label _lblSelect;
    gui::ComboBox _cmbLanguages;
    gui::Label _lblBoard;
    gui::ComboBox _cmbBoardSizes;
    gui::Label _lblBoardStyle;
    gui::ComboBox _cmbBoardStyles;
    gui::Label _lblDifficulty;
    gui::ComboBox _cmbDifficulty;
    gui::GridLayout _layout;

    inline static const std::array<LanguageOption, 2> kLanguages{{
        LanguageOption{"EN", "langEnglish"},
        LanguageOption{"BA", "langBosnian"}
    }};

    int _initialSelection = 0;
    bool _suppressSettingsSignals = false;
    std::function<void(BoardDimension)> _boardChangedHandler;
    std::function<void(BoardStyle)> _boardStyleChangedHandler;
    std::function<void(Difficulty)> _difficultyChangedHandler;

    void populateLanguages();
    void handleSelectionChanged();
    void populateBoardSizes();
    void populateBoardStyles();
    void populateDifficulties();
    [[nodiscard]] int indexForBoard(BoardDimension dimension) const;
};

inline SettingsPopup::SettingsPopup()
: _imgButton(":settings")
, _lblSelect(tr("language"))
, _lblBoard(tr("boardSize"))
, _lblBoardStyle(tr("boardStyle"))
, _lblDifficulty(tr("difficulty"))
, _layout(4, 2)
{
    setPopoverButtonImage(&_imgButton);

    gui::GridComposer gc(_layout);
    gc.appendRow(_lblSelect) << _cmbLanguages;
    gc.appendRow(_lblBoard) << _cmbBoardSizes;
    gc.appendRow(_lblBoardStyle) << _cmbBoardStyles;
    gc.appendRow(_lblDifficulty) << _cmbDifficulty;
    setLayout(&_layout);

    populateLanguages();
    populateBoardSizes();
    populateBoardStyles();
    populateDifficulties();
    _cmbLanguages.onChangedSelection([this]() { handleSelectionChanged(); });
    _cmbBoardSizes.onChangedSelection([this]() {
        if (_suppressSettingsSignals) {
            return;
        }
        int index = _cmbBoardSizes.getSelectedIndex();
        if (index < 0 || index >= static_cast<int>(kBoardSizeConfigs.size())) {
            return;
        }
        if (_boardChangedHandler) {
            _boardChangedHandler(kBoardSizeConfigs[static_cast<std::size_t>(index)].id);
        }
    });
    _cmbDifficulty.onChangedSelection([this]() {
        if (_suppressSettingsSignals) {
            return;
        }
        int index = _cmbDifficulty.getSelectedIndex();
        if (index < 0 || index > static_cast<int>(Difficulty::Hard)) {
            return;
        }
        if (_difficultyChangedHandler) {
            _difficultyChangedHandler(static_cast<Difficulty>(index));
        }
    });
    _cmbBoardStyles.onChangedSelection([this]() {
        if (_suppressSettingsSignals) {
            return;
        }
        int index = _cmbBoardStyles.getSelectedIndex();
        if (index < 0) {
            return;
        }
        if (_boardStyleChangedHandler) {
            _boardStyleChangedHandler(static_cast<BoardStyle>(index));
        }
    });
}

inline void SettingsPopup::populateLanguages() {
    auto* app = gui::getApplication();
    auto* props = app->getProperties();
    td::String storedExt = props ? props->getValue("translation", td::String("EN")) : td::String("EN");

    for (std::size_t i = 0; i < kLanguages.size(); ++i) {
        _cmbLanguages.addItem(tr(kLanguages[i].translationKey));
        td::String optionExt(kLanguages[i].extension);
        if (storedExt == optionExt) {
            _initialSelection = static_cast<int>(i);
        }
    }
    _cmbLanguages.selectIndex(_initialSelection);
}

inline void SettingsPopup::handleSelectionChanged() {
    int selection = _cmbLanguages.getSelectedIndex();
    if (selection < 0 || selection >= static_cast<int>(kLanguages.size())) {
        return;
    }

    auto* app = gui::getApplication();
    auto* props = app->getProperties();
    if (props) {
        props->setValue("translation", td::String(kLanguages[selection].extension));
    }

    if (selection != _initialSelection) {
        gui::Alert::showYesNoQuestion(tr("RestartRequired"), tr("RestartRequiredInfo"), tr("Restart"), tr("DoNoRestart"), [app](gui::Alert::Answer answer) {
            if (answer == gui::Alert::Answer::Yes && app) {
                app->restart();
            }
        });
    }
}

inline void SettingsPopup::populateBoardSizes() {
    for (const auto& cfg : kBoardSizeConfigs) {
        _cmbBoardSizes.addItem(cfg.name);
    }
}

inline void SettingsPopup::populateBoardStyles() {
    _cmbBoardStyles.addItem(tr("woodenTheme"));
    _cmbBoardStyles.addItem(tr("blackWhiteTheme"));
    _cmbBoardStyles.addItem(tr("greenTheme"));
    _cmbBoardStyles.addItem(tr("blueTheme"));
    _cmbBoardStyles.addItem(tr("roseTheme"));
}

inline void SettingsPopup::populateDifficulties() {
    _cmbDifficulty.addItem(tr("Easy"));
    _cmbDifficulty.addItem(tr("Medium"));
    _cmbDifficulty.addItem(tr("Hard"));
}

inline void SettingsPopup::setBoardChangedHandler(std::function<void(BoardDimension)> handler) {
    _boardChangedHandler = std::move(handler);
}

inline void SettingsPopup::setBoardStyleChangedHandler(std::function<void(BoardStyle)> handler) {
    _boardStyleChangedHandler = std::move(handler);
}

inline void SettingsPopup::setDifficultyChangedHandler(std::function<void(Difficulty)> handler) {
    _difficultyChangedHandler = std::move(handler);
}

inline int SettingsPopup::indexForBoard(BoardDimension dimension) const {
    for (std::size_t i = 0; i < kBoardSizeConfigs.size(); ++i) {
        if (kBoardSizeConfigs[i].id == dimension) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

inline void SettingsPopup::syncSelections(BoardDimension board, Difficulty difficulty, BoardStyle style) {
    _suppressSettingsSignals = true;
    int boardIndex = indexForBoard(board);
    if (boardIndex >= 0) {
        _cmbBoardSizes.selectIndex(boardIndex);
    }
    int diffIndex = std::clamp(static_cast<int>(difficulty), 0, 2);
    _cmbDifficulty.selectIndex(diffIndex);
    int styleIndex = std::clamp(static_cast<int>(style), 0, 4);
    _cmbBoardStyles.selectIndex(styleIndex);
    _suppressSettingsSignals = false;
}

inline BoardDimension SettingsPopup::currentBoardSelection() const {
    int index = _cmbBoardSizes.getSelectedIndex();
    if (index < 0 || index >= static_cast<int>(kBoardSizeConfigs.size())) {
        return kBoardSizeConfigs[0].id;
    }
    return kBoardSizeConfigs[static_cast<std::size_t>(index)].id;
}

inline Difficulty SettingsPopup::currentDifficultySelection() const {
    int index = _cmbDifficulty.getSelectedIndex();
    if (index < 0) {
        return Difficulty::Medium;
    }
    return static_cast<Difficulty>(std::clamp(index, 0, static_cast<int>(Difficulty::Hard)));
}

inline BoardStyle SettingsPopup::currentBoardStyleSelection() const {
    int index = _cmbBoardStyles.getSelectedIndex();
    if (index < 0) {
        return BoardStyle::Wooden;
    }
    return static_cast<BoardStyle>(std::clamp(index, 0, 4));
}
