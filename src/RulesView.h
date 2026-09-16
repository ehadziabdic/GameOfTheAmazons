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
		td::String rulesText = "► ABOUT THE GAME:\n";
		rulesText += "---------------------------------------------------------------------------\n";
		rulesText += "The Game of the Amazons is a strategy game for two players, which can be played on a small (6x6), medium (8x8) and big (10x10) board. The game combines the movement of chess with the added space-restriction mechanics of Go.\n";
		rulesText += "Each player controls four Amazons (Queens). The game is a battle for space where players try to isolate their opponent by blocking off sections of the board with arrows.\n" ;
		rulesText += "The game ends when one of the players is completely blocked from moving. This makes every turn in The Game of the Amazons a fight for survival and for whatever space is left on the board.\n\n";
		rulesText += "► GAME RULES:\n";
		rulesText += "---------------------------------------------------------------------------\n";
		rulesText += "◊ THE OBJECTIVE: \n	● The goal is to be the last player that is able to make a move. If you cannot move or fire when it is your turn, you lose.\n";
		rulesText += "◊ THE MOVE: \n	● White goes first. Pick one Amazon and move it exactly like a Chess Queen (any distance horizontally, vertically, or diagonally).\n";
		rulesText += "◊ THE SHOT: \n	● After landing, that same Amazon must shoot an arrow to an empty square. The arrow also travels like a Queen (any distance horizontally, vertically, or diagonally).\n	● The spot where the arrow lands becomes blocked for the entirety of the game.\n";
		rulesText += "◊ RESTRICTIONS: \n	● Neither Amazons nor Arrows can jump over any other pieces or existing blocks. \n";
		rulesText += "◊ GAME OVER: \n	● The game continues until a player is trapped and has no legal moves left.\n\n";
		rulesText += "► TIPS & STRATEGY\n";
		rulesText += "---------------------------------------------------------------------------\n";
		rulesText += "◊ Control the Center: Queens near the center have more mobility.\n";
		rulesText += "◊ Territory Management: Try to claim more reachable board space.\n";
		rulesText += "◊ Block Wisely: Use arrows to limit opponent's movement options.\n";
		rulesText += "◊ Plan Ahead: Consider both your queen move and arrow placement.\n";
		rulesText += "◊ Stay Mobile: Avoid boxing yourself in with your own arrows.\n\n";
		rulesText += "► APP FEATURES\n";
		rulesText += "---------------------------------------------------------------------------\n";
		rulesText += "◊ VERSATILE GAME MODES\n";
		rulesText += "	● Player vs Player: Challenge a friend in local multiplayer.\n";
		rulesText += "	● Player vs AI: Test your skills against an intelligent computer opponent.\n";
		rulesText += "	● AI vs AI: Watch two computer agents battle to observe optimal strategies in real-time.\n\n";
		rulesText += "◊ BOARD SIZES\n";
		rulesText += "	● 6x6 (Small): Quick games with faster pace, ideal for beginners.\n";
		rulesText += "	● 8x8 (Medium): Balanced gameplay with moderate complexity.\n";
		rulesText += "	● 10x10 (Large): Classic board size with maximum strategic depth.\n\n";
		rulesText += "◊ AI DIFFICULTY LEVELS\n";
		rulesText += "	● Easy: AI searches 1-2 moves ahead, perfect for learning.\n";
		rulesText += "	● Medium: AI searches 3-4 moves ahead, balanced challenge.\n";
		rulesText += "	● Hard: AI searches 5+ moves ahead, expert-level opponent.\n";
		rulesText += "	-> AI uses Minimax algorithm with Alpha-Beta pruning for optimal play.\n\n";
		rulesText += "◊ BOARD THEMES\n";
		rulesText += "	● Wooden: Classic warm wood texture (default).\n";
		rulesText += "	● Black & White: High-contrast minimalist style.\n";
		rulesText += "	● Diamond: Elegant crystalline theme with sparkling effects.\n";
		rulesText += "	● Ice: Cool frost-inspired design.\n";
		rulesText += "	● Stone: Solid granite texture.\n";
		rulesText += "	● Tournament: Professional competition style.\n";
		rulesText += "	● Bubblegum: Vibrant colorful theme.\n";
		rulesText += "	● Custom Theme: Create your own colors with light and dark tile pickers.\n\n";
		rulesText += "◊ ANIMATIONS & EFFECTS\n";
		rulesText += "	● Smooth queen movement animations.\n";
		rulesText += "	● Arrow shooting with trajectory visualization.\n";
		rulesText += "	● Field explosion effects when arrows land.\n";
		rulesText += "	● Adjustable animation speed slider for preferred pacing.\n";
		rulesText += "	● Victory/defeat overlay displays.\n\n";
		rulesText += "◊ SOUND EFFECTS\n";
		rulesText += "	● Movement sounds for queen actions.\n";
		rulesText += "	● Arrow flight sound effects.\n";
		rulesText += "	● Field explosion audio feedback.\n";
		rulesText += "	● Victory and defeat sounds.\n\n";
		rulesText += "◊ MULTI-LANGUAGE SUPPORT\n";
		rulesText += "	● English (EN)\n";
		rulesText += "	● Bosnian (BA)\n";
		rulesText += "	● German (DE)\n";
		rulesText += "	● Spanish (ES)\n";
		rulesText += "	● French (FR)\n";
		rulesText += "	● Japanese (JP)\n\n";
		rulesText += "◊ GAME CONTROLS\n";
		rulesText += "	● Move History: View complete log of all moves played.\n";
		rulesText += "	● Undo Button: Take back the last move (disabled during AI thinking).\n";
		rulesText += "	● New Game: Start fresh with current settings.\n";
		rulesText += "	● Stop AI: Stops AI in middle of calculaions so game can be turned off safely.\n";
		rulesText += "	● Settings Menu: Access all customization options.";
		
		_textEdit.setValue(rulesText);
		_layout.append(_textEdit);
		setLayout(&_layout);
	}

private:
	gui::VerticalLayout _layout;
	gui::TextEdit _textEdit;
};
