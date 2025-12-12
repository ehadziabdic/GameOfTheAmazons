#pragma once

#include "AmazonsBoardCanvas.h"
#include "Algorithms.h"
#include "SettingsPopup.h"
#include "DialogSettings.h"

#include <gui/Alert.h>
#include <gui/Button.h>
#include <gui/GridComposer.h>
#include <gui/GridLayout.h>
#include <gui/Image.h>
#include <gui/Label.h>
#include <gui/Sound.h>
#include <gui/View.h>
#include <gui/Thread.h>

#include <thread>
#include <atomic>

class MainView : public gui::View {
public:
	MainView();
	~MainView() override;

	// Helpers for external UI actions (toolbar)
	void openSettingsDialog();
	void triggerNewGameFromToolbar();
	void triggerUndoFromToolbar();
	// Allow toolbar to cancel an ongoing AI search
	void cancelAiFromToolbar();

	// Fast difficulty setter used by toolbar
	bool setGameDifficulty(Difficulty difficulty);

	// Allow the window to receive toolbar state updates (enable/disable items)
	void setToolbarStateHandler(std::function<void(bool,bool)> handler);
	// Allow the window to update status bar text
	void setStatusBarHandler(std::function<void(const td::String&)> handler);

	void focusBoard();

private:
	AmazonsBoardCanvas _boardCanvas;
	gui::GridLayout _layout;
	gui::Label _lblWhite;
	gui::ComboBox _cmbWhitePlayerType;
	gui::Label _lblBlack;
	gui::ComboBox _cmbBlackPlayerType;
	gui::Image _imgNewGame;
	gui::Image _imgUndo;
	gui::Image _imgSettings;
	gui::Button _btnNewGame;
	gui::Button _btnUndo;
	gui::Button _btnSettings;
	gui::Sound _soundMove;
	gui::Sound _soundVictory;
	gui::Sound _soundLoss;
    
	// settings dialog is created on demand

	BoardDimension _selectedBoardDimension = BoardDimension::Ten;
	Difficulty _selectedDifficulty = Difficulty::Medium;
	BoardStyle _selectedBoardStyle = BoardStyle::Wooden;

	GameState _state;
	PlayerType _whitePlayerType = PlayerType::Human;
	PlayerType _blackPlayerType = PlayerType::AI;
	bool _gameOverDialogShown = false;

	std::thread _aiThread;
	bool _aiThinking = false;
	std::atomic_bool _cancelAi{false};
	std::function<void(bool,bool)> _toolbarStateHandler;
	std::function<void(const td::String&)> _statusBarHandler;

	void buildLayout();
	void populateControls();
	void wireCallbacks();
	void startNewGame();
	void handleHumanMove(const Move& move);
	void handleAiMove(const Move& move);
	void requestAiMove();
	void updateStatusForPhase(AmazonsBoardCanvas::SelectionPhase phase);
	void setStatusText(const td::String& text);
	void updateControlsState();
	void tryHandleGameEnd();
	void showWinnerDialog(Player winner);
	BoardDimension selectedBoardDimension() const;
	Difficulty selectedDifficulty() const;
	void finalizeAiThread();
	bool guardAgainstAiBusy() const;
	bool isCurrentPlayerAI() const;
};

inline MainView::MainView()
: _layout(3, 1)
	, _imgNewGame(":reset")
	, _imgUndo(":undo")
	, _imgSettings(":settings")
	, _btnNewGame(&_imgNewGame, tr("newGame"))
	, _btnUndo(&_imgUndo, tr("undo"))
	, _btnSettings(&_imgSettings, tr("settings"))
	, _soundMove(":move")
	, _soundVictory(":victory")
	, _soundLoss(":loss")
{
	_boardCanvas.setGameState(&_state);
	_boardCanvas.setBoardStyle(_selectedBoardStyle);
	_boardCanvas.setMoveHandler([this](const Move& move) { handleHumanMove(move); });
	_boardCanvas.setPhaseChangedHandler([this](AmazonsBoardCanvas::SelectionPhase phase) {
		updateStatusForPhase(phase);
		updateControlsState();
	});

	buildLayout();
	populateControls();
	wireCallbacks();
	setLayout(&_layout);
	startNewGame();
}

inline MainView::~MainView() {
	finalizeAiThread();
}

inline void MainView::focusBoard() {
	_boardCanvas.setFocus(true);
}

inline void MainView::buildLayout() {
	gui::GridComposer composer(_layout);
	// Row 1: Black player combo
	composer.appendRow(_cmbBlackPlayerType);
	// Row 2: Board
	composer.appendRow(_boardCanvas);
	// Row 3: White player combo
	composer.appendRow(_cmbWhitePlayerType);
}

inline void MainView::populateControls() {
	// Player type combos (labels not needed - will use combo title/tooltip)
	_cmbWhitePlayerType.setToolTip(tr("whitePlayer"));
	_cmbWhitePlayerType.addItem(tr("human"));
	_cmbWhitePlayerType.addItem(tr("ai"));
	_cmbWhitePlayerType.selectIndex(0); // Default to Human
	
	_cmbBlackPlayerType.setToolTip(tr("blackPlayer"));
	_cmbBlackPlayerType.addItem(tr("human"));
	_cmbBlackPlayerType.addItem(tr("ai"));
	_cmbBlackPlayerType.selectIndex(1); // Default to AI
	
	_btnNewGame.setToolTip(tr("newGame"));
	_btnNewGame.setFlat();
	_btnNewGame.setToMinSize();
	_btnUndo.setToolTip(tr("undo"));
	_btnUndo.setFlat();
	_btnUndo.setToMinSize();
	_btnSettings.setToolTip(tr("settings"));
	_btnSettings.setFlat();
	_btnSettings.setToMinSize();
}

inline void MainView::wireCallbacks() {
	// Player type combo boxes
	_cmbWhitePlayerType.onChangedSelection([this]() {
		int selectedIndex = _cmbWhitePlayerType.getSelectedIndex();
		_whitePlayerType = static_cast<PlayerType>(selectedIndex);
		// If it's White's turn, update interaction and possibly trigger AI
		if (_state.currentPlayer() == Player::White && !_state.isFinished()) {
			if (_whitePlayerType == PlayerType::AI) {
				_boardCanvas.setInteractionEnabled(false);
				requestAiMove();
			} else {
				_boardCanvas.setInteractionEnabled(true);
				updateStatusForPhase(_boardCanvas.currentPhase());
			}
		}
	});
	
	_cmbBlackPlayerType.onChangedSelection([this]() {
		int selectedIndex = _cmbBlackPlayerType.getSelectedIndex();
		_blackPlayerType = static_cast<PlayerType>(selectedIndex);
		// If it's Black's turn, update interaction and possibly trigger AI
		if (_state.currentPlayer() == Player::Black && !_state.isFinished()) {
			if (_blackPlayerType == PlayerType::AI) {
				_boardCanvas.setInteractionEnabled(false);
				requestAiMove();
			} else {
				_boardCanvas.setInteractionEnabled(true);
				updateStatusForPhase(_boardCanvas.currentPhase());
			}
		}
	});
	
	_btnNewGame.onClick([this]() {
		if (guardAgainstAiBusy()) {
			return;
		}
		startNewGame();
		// Play click sound to indicate reset/new-game completed
		gui::Sound::play(gui::Sound::Type::SelectionChanged);
	});

	// Settings button opens a dialog containing settings controls.
	_btnSettings.onClick([this]() { openSettingsDialog(); });

	_btnUndo.onClick([this]() {
		if (guardAgainstAiBusy()) {
			return;
		}
		// Only meaningful when user has selected a destination/arrow.
		if (_boardCanvas.currentPhase() == AmazonsBoardCanvas::SelectionPhase::SelectQueen) {
			return;
		}
		_boardCanvas.resetSelections();
		_boardCanvas.setInteractionEnabled(true);
		updateStatusForPhase(_boardCanvas.currentPhase());
		updateControlsState();
		// Play click sound to indicate undo action
		gui::Sound::play(gui::Sound::Type::SelectionChanged);
	});
}

inline void MainView::startNewGame() {
	_state.startNewGame(selectedBoardDimension(), selectedDifficulty());
	_gameOverDialogShown = false;
	_boardCanvas.setGameState(&_state);
	_boardCanvas.setInteractionEnabled(true);
	_boardCanvas.setAiThinking(false);
	_boardCanvas.resetSelections();
	updateStatusForPhase(_boardCanvas.currentPhase());
	updateControlsState();

	if (isCurrentPlayerAI()) {
		_boardCanvas.setInteractionEnabled(false);
		requestAiMove();
	}
}

inline void MainView::openSettingsDialog()
{
	if (guardAgainstAiBusy()) {
		return;
	}
	auto dlg = new DialogSettings(this);
	dlg->syncSelections(_selectedBoardDimension, _selectedDifficulty, _selectedBoardStyle);
	dlg->setBoardChangedHandler([this](BoardDimension dimension) {
		if (dimension == _selectedBoardDimension) {
			return;
		}
		if (guardAgainstAiBusy()) {
			return;
		}
		_selectedBoardDimension = dimension;
		startNewGame();
		_boardCanvas.setBoardStyle(_selectedBoardStyle);
	});
	dlg->setDifficultyChangedHandler([this](Difficulty difficulty) {
		if (difficulty == _selectedDifficulty) {
			return;
		}
		if (guardAgainstAiBusy()) {
			return;
		}
		_selectedDifficulty = difficulty;
		startNewGame();
		_boardCanvas.setBoardStyle(_selectedBoardStyle);
	});
	dlg->setBoardStyleChangedHandler([this, dlg](BoardStyle style) {
		if (guardAgainstAiBusy()) {
			return;
		}
		_selectedBoardStyle = style;
		_boardCanvas.setBoardStyle(_selectedBoardStyle);
		// If custom theme, also update the custom colors
		if (style == BoardStyle::CustomTheme) {
			td::ColorID lightColor = dlg->getSettingsView().getLightTileColor();
			td::ColorID darkColor = dlg->getSettingsView().getDarkTileColor();
			_boardCanvas.setCustomColors(lightColor, darkColor);
		}
		updateControlsState();
	});
	dlg->openNonModal();
}

inline void MainView::triggerNewGameFromToolbar()
{
	if (guardAgainstAiBusy()) return;
	startNewGame();
	gui::Sound::play(gui::Sound::Type::SelectionChanged);
}

inline void MainView::triggerUndoFromToolbar()
{
	if (guardAgainstAiBusy()) return;
	if (_boardCanvas.currentPhase() == AmazonsBoardCanvas::SelectionPhase::SelectQueen) return;
	_boardCanvas.resetSelections();
	_boardCanvas.setInteractionEnabled(true);
	updateStatusForPhase(_boardCanvas.currentPhase());
	updateControlsState();
	gui::Sound::play(gui::Sound::Type::SelectionChanged);
}

inline bool MainView::setGameDifficulty(Difficulty difficulty)
{
	if (difficulty == _selectedDifficulty)
		return false;
	if (guardAgainstAiBusy())
		return false;
	_selectedDifficulty = difficulty;
	startNewGame();
	_boardCanvas.setBoardStyle(_selectedBoardStyle);
	return true;
}

inline void MainView::handleHumanMove(const Move& move) {
	if (_aiThinking || _state.isFinished()) {
		gui::Sound::play(gui::Sound::Type::Beep);
		return;
	}
	if (!isMoveLegal(_state, move)) {
		gui::Sound::play(gui::Sound::Type::Beep);
		return;
	}

	_boardCanvas.setInteractionEnabled(false);
	applyMove(_state, move);
	_boardCanvas.notifyMoveApplied(move);
	// Play the configured move sound.
	_soundMove.play();
	updateStatusForPhase(_boardCanvas.currentPhase());
	tryHandleGameEnd();

	if (_state.isFinished()) {
		return;
	}

	if (isCurrentPlayerAI()) {
		// If board is animating (arrow in flight / impact pending), defer AI until animations complete
		if (_boardCanvas.isAnimating()) {
			_boardCanvas.setAnimationFinishedHandler([this]() {
				// ensure this runs on the main thread and that AI still should move
				gui::thread::asyncExecInMainThread([this]() {
					if (isCurrentPlayerAI() && !_aiThinking && !_state.isFinished()) {
						requestAiMove();
					}
				});
			});
		} else {
			requestAiMove();
		}
	} else {
		_boardCanvas.setInteractionEnabled(true);
		updateStatusForPhase(_boardCanvas.currentPhase());
	}
}

inline void MainView::handleAiMove(const Move& move) {
	finalizeAiThread();
	_aiThinking = false;
	_boardCanvas.setAiThinking(false);
	updateControlsState();

	if (move.player == Player::None) {
		evaluateWinState(_state);
		tryHandleGameEnd();
		return;
	}

	applyMove(_state, move);
	_boardCanvas.notifyMoveApplied(move);
	// Play the configured move sound for AI moves as well.
	_soundMove.play();
	tryHandleGameEnd();

	if (_state.isFinished()) {
		return;
	}

	if (!isCurrentPlayerAI()) {
		_boardCanvas.setInteractionEnabled(true);
		updateStatusForPhase(_boardCanvas.currentPhase());
	} else {
		requestAiMove();
	}
}

inline void MainView::requestAiMove() {
	if (_aiThinking || _state.isFinished() || !isCurrentPlayerAI()) {
		return;
	}

	_aiThinking = true;
	_boardCanvas.setAiThinking(true);
	updateControlsState();
	updateStatusForPhase(_boardCanvas.currentPhase());

	GameState snapshot = _state.clone();
	auto difficulty = _state.difficulty();

	finalizeAiThread();
	// reset cancellation token for this search
	_cancelAi.store(false);
	_aiThread = std::thread([this, snapshot, difficulty]() mutable {
		try {
			Move bestMove = getBestMove(snapshot, difficulty, &_cancelAi);
			gui::thread::asyncExecInMainThread([this, bestMove]() {
				handleAiMove(bestMove);
			});
		} catch (const SearchCanceled&) {
			// Search was cancelled cooperatively — stop thinking and update UI.
			gui::thread::asyncExecInMainThread([this]() {
				_aiThinking = false;
				_boardCanvas.setAiThinking(false);
				updateControlsState();
			});
		} catch (const std::exception& ex) {
			// Avoid terminating the process if something in the search throws.
			gui::thread::asyncExecInMainThread([this, what = td::String(ex.what())]() {
				_aiThinking = false;
				_boardCanvas.setAiThinking(false);
				updateControlsState();
				gui::Alert::show(tr("Error"), what);
			});
		} catch (...) {
			gui::thread::asyncExecInMainThread([this]() {
				_aiThinking = false;
				_boardCanvas.setAiThinking(false);
				updateControlsState();
				gui::Alert::show(tr("Error"), tr("Unknown AI error"));
			});
		}
	});
}

inline void MainView::updateStatusForPhase(AmazonsBoardCanvas::SelectionPhase phase) {
	if (_aiThinking) {
		setStatusText(tr("statusAiThinking"));
		return;
	}
	if (_state.isFinished()) {
		setStatusText(tr("statusGameFinished"));
		return;
	}

	switch (phase) {
		case AmazonsBoardCanvas::SelectionPhase::SelectQueen:
			setStatusText(tr("statusSelectQueen"));
			break;
		case AmazonsBoardCanvas::SelectionPhase::SelectDestination:
			setStatusText(tr("statusSelectDestination"));
			break;
		case AmazonsBoardCanvas::SelectionPhase::SelectArrow:
			setStatusText(tr("statusSelectArrow"));
			break;
		default:
			break;
	}
}

inline void MainView::setStatusText(const td::String& text) {
	if (_statusBarHandler) {
		_statusBarHandler(text);
	}
}

inline void MainView::updateControlsState() {
	bool allowChanges = !_aiThinking;
	_btnNewGame.enable(allowChanges);
	bool allowUndo = allowChanges && (_boardCanvas.currentPhase() != AmazonsBoardCanvas::SelectionPhase::SelectQueen);
	_btnUndo.enable(allowUndo);
	if (_toolbarStateHandler) {
		_toolbarStateHandler(allowChanges, allowUndo);
	}
}

inline void MainView::setToolbarStateHandler(std::function<void(bool,bool)> handler)
{
	_toolbarStateHandler = std::move(handler);
}

inline void MainView::setStatusBarHandler(std::function<void(const td::String&)> handler)
{
	_statusBarHandler = std::move(handler);
}

inline void MainView::tryHandleGameEnd() {
	if (_state.isFinished()) {
		_boardCanvas.setInteractionEnabled(false);
		Player winner = _state.winner();
		if (winner != Player::None && !_gameOverDialogShown) {
			_gameOverDialogShown = true;
			showWinnerDialog(winner);
		}
		updateStatusForPhase(_boardCanvas.currentPhase());
	}
}

inline void MainView::showWinnerDialog(Player winner) {
	td::String title = tr("gameOver");
	td::String message;
	
	// Determine message based on player types
	PlayerType winnerType = (winner == Player::White) ? _whitePlayerType : _blackPlayerType;
	
	if (_whitePlayerType == PlayerType::Human && _blackPlayerType == PlayerType::Human) {
		// Player vs Player mode
		message = (winner == Player::White) ? tr("msgWhiteWins") : tr("msgBlackWins");
		_soundVictory.play();
	} else if (_whitePlayerType == PlayerType::AI && _blackPlayerType == PlayerType::AI) {
		// AI vs AI mode
		message = (winner == Player::White) ? tr("msgWhiteWins") : tr("msgBlackWins");
		_soundVictory.play();
	} else {
		// Player vs AI mode
		if (winnerType == PlayerType::Human) {
			message = tr("msgYouWin");
			_soundVictory.play();
		} else {
			message = tr("msgYouLose");
			_soundLoss.play();
		}
	}
	
	gui::Alert::show(title, message);
}

inline BoardDimension MainView::selectedBoardDimension() const {
	return _selectedBoardDimension;
}

inline Difficulty MainView::selectedDifficulty() const {
	return _selectedDifficulty;
}

inline void MainView::finalizeAiThread() {
	// Signal cancellation and wait for any running AI thread to finish.
	_cancelAi.store(true);
	if (_aiThread.joinable()) {
		_aiThread.join();
	}
}

inline bool MainView::guardAgainstAiBusy() const {
	if (!_aiThinking) {
		return false;
	}
	gui::Sound::play(gui::Sound::Type::Beep);
	return true;
}

inline void MainView::cancelAiFromToolbar() {
	if (!_aiThinking) return;
	_cancelAi.store(true);
	// Immediately update UI to stop spinner; the worker will observe the token and exit.
	_aiThinking = false;
	_boardCanvas.setAiThinking(false);
	updateControlsState();
	// Wait for the thread to finish cleanup to avoid races when starting new searches.
	finalizeAiThread();
}

inline bool MainView::isCurrentPlayerAI() const {
	Player current = _state.currentPlayer();
	if (current == Player::White) {
		return _whitePlayerType == PlayerType::AI;
	} else if (current == Player::Black) {
		return _blackPlayerType == PlayerType::AI;
	}
	return false;
}

