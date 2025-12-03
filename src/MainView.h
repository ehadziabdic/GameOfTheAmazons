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

	void focusBoard();

private:
	AmazonsBoardCanvas _boardCanvas;
	gui::GridLayout _layout;
	gui::Image _imgNewGame;
	gui::Image _imgUndo;
	gui::Image _imgSettings;
	gui::Button _btnNewGame;
	gui::Button _btnUndo;
	gui::Button _btnSettings;
	gui::Label _lblStatus;
	gui::Sound _soundMove;
	gui::Sound _soundVictory;
	gui::Sound _soundLoss;
    
	// settings dialog is created on demand

	BoardDimension _selectedBoardDimension = BoardDimension::Ten;
	Difficulty _selectedDifficulty = Difficulty::Medium;
	BoardStyle _selectedBoardStyle = BoardStyle::Wooden;

	GameState _state;
	Player _humanPlayer = Player::White;
	Player _aiPlayer = Player::Black;
	bool _gameOverDialogShown = false;

	std::thread _aiThread;
	bool _aiThinking = false;
	std::atomic_bool _cancelAi{false};
	std::function<void(bool,bool)> _toolbarStateHandler;

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
};

inline MainView::MainView()
: _layout(2, 4)
	, _imgNewGame(":reset")
	, _imgUndo(":undo")
	, _imgSettings(":settings")
	, _btnNewGame(&_imgNewGame, tr("newGame"))
	, _btnUndo(&_imgUndo, tr("undo"))
	, _btnSettings(&_imgSettings, tr("settings"))
	, _lblStatus(tr("statusSelectQueen"))
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
	// Status row only; toolbar lives in the window header now.
	composer.appendRow(_lblStatus);
	composer.appendRow(_boardCanvas, 4);
}

inline void MainView::populateControls() {
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

	if (_state.currentPlayer() == _aiPlayer) {
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
	dlg->setBoardStyleChangedHandler([this](BoardStyle style) {
		if (guardAgainstAiBusy()) {
			return;
		}
		_selectedBoardStyle = style;
		_boardCanvas.setBoardStyle(_selectedBoardStyle);
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

	if (_state.currentPlayer() == _aiPlayer) {
		// If board is animating (arrow in flight / impact pending), defer AI until animations complete
		if (_boardCanvas.isAnimating()) {
			_boardCanvas.setAnimationFinishedHandler([this]() {
				// ensure this runs on the main thread and that AI still should move
				gui::thread::asyncExecInMainThread([this]() {
					if (_state.currentPlayer() == _aiPlayer && !_aiThinking && !_state.isFinished()) {
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

	if (_state.currentPlayer() == _humanPlayer) {
		_boardCanvas.setInteractionEnabled(true);
		updateStatusForPhase(_boardCanvas.currentPhase());
	} else {
		requestAiMove();
	}
}

inline void MainView::requestAiMove() {
	if (_aiThinking || _state.isFinished() || _state.currentPlayer() != _aiPlayer) {
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
	_lblStatus.setTitle(text);
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
	td::String message = (winner == _humanPlayer) ? tr("msgYouWin") : tr("msgYouLose");
	if (winner == _humanPlayer) {
		_soundVictory.play();
	} else {
		_soundLoss.play();
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

