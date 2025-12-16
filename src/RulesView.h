#pragma once

#include <gui/View.h>
#include <gui/VerticalLayout.h>
#include <gui/TextEdit.h>

class RulesView : public gui::View {
public:
	RulesView()
		: _layout(1)
	{
		_textEdit.setAsReadOnly();
		td::String rulesText = "ABOUT THE GAME:\n\n";
		rulesText += "The Game of the Amazons is a strategy game for two players, which can be played on a small (6x6), medium (8x8) and big (10x10) board. The game combines the movement of chess with the added space-restriction mechanics of Go.\n\n";
		rulesText += "Each player controls four Amazons (Queens). The game is a battle for space where players try to isolate their opponent by blocking off sections of the board with arrows. The game ends when one of the players is completely blocked from moving. This makes every turn in The Game of the Amazons a fight for survival and for whatever space is left on the board.\n\n";
		rulesText += "-------------------------\n\n";
		rulesText = "HOW TO PLAY:\n\n";
		rulesText += "1. THE OBJECTIVE: \nThe goal is to be the last player that is able to make a move. If you cannot move or fire when it is your turn, you lose.\n\n";
		rulesText += "2. THE MOVE: \nWhite goes first. Pick one Amazon and move it exactly like a Chess Queen (any distance horizontally, vertically, or diagonally).\n\n";
		rulesText += "3. THE SHOT: \nAfter landing, that same Amazon must shoot an arrow to an empty square. The arrow also travels like a Queen (any distance horizontally, vertically, or diagonally). The spot where the arrow lands becomes blocked for the entirety of the game.\n\n";
		rulesText += "4. RESTRICTIONS\nNeither Amazons nor Arrows can jump over any other pieces or existing blocks. \n\n";
		rulesText += "5. GAME OVER: \nThe game continues until a player is trapped and has no legal moves left.\n\n";
		rulesText += "-------------------------\n\n";
		rulesText += "FEATURES\n\n";
		rulesText += "VERSATILE GAME MODES\n";
		rulesText += "- Player vs AI: Challenge a computer opponent to hone your skills and test your strategies.\n";
		rulesText += "- AI vs AI: Watch two computer agents battle it out to observe optimal strategies and territory control in real-time.\n\n";
		rulesText += "CUSTOMIZATION\n";
		rulesText += "- Board Styles: Customize your visual experience with different board themes, ranging from classic wooden textures to modern high-contrast styles, such as:";
		rulesText += "- Diamond\n";
		rulesText += "- Ice\n";
		rulesText += "- Stone\n";
		rulesText += "- Tournament\n";
		
		_textEdit.setValue(rulesText);
		
		_layout.append(_textEdit);
		setLayout(&_layout);
	}

private:
	gui::VerticalLayout _layout;
	gui::TextEdit _textEdit;
};
