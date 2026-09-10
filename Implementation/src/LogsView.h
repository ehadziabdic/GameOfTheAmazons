#pragma once

#include "GameState.h"

#include <gui/View.h>
#include <gui/VerticalLayout.h>
#include <gui/TextEdit.h>

class LogsView : public gui::View {
public:
	LogsView(const GameState* state)
		: _layout(1)
		, _state(state)
	{
		_textEdit.setAsReadOnly();
		_layout.append(_textEdit);
		setLayout(&_layout);
	}

	void updateMoveHistory() {
		if (!_state) return;
		
		const auto& history = _state->moveHistory();
		td::String logText = "► Move History\n";
		logText += "---------------------------------------------------------------------------\n\n";
		
		if (history.empty()) {
			logText += "No moves yet.";
		} else {
			int moveNum = 1;
			for (const auto& move : history) {
				td::String moveStr;
				moveStr.format("► Move %d: Queen (%d,%d) -> (%d,%d), Arrow -> (%d,%d)\n", 
					moveNum++,
					move.queenFrom.row + 1, move.queenFrom.col + 1,
					move.queenTo.row + 1, move.queenTo.col + 1,
					move.arrow.row + 1, move.arrow.col + 1);
				logText += moveStr;
			}
		}
		
		_textEdit.setValue(logText);
	}

private:
	gui::VerticalLayout _layout;
	gui::TextEdit _textEdit;
	const GameState* _state;
};
