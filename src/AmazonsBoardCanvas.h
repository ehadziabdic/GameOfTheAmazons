#pragma once

#include "GameState.h"
#include "Rules.h"

#include <gui/Canvas.h>
#include <gui/DrawableString.h>
#include <gui/Image.h>
#include <gui/Application.h>
#include <gui/InputDevice.h>
#include <gui/Shape.h>
#include <gui/Sound.h>
#include <gui/Timer.h>
#include <gui/Transformation.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <vector>

class AmazonsBoardCanvas : public gui::Canvas {
public:
    enum class SelectionPhase { SelectQueen, SelectDestination, SelectArrow };
    using Style = BoardStyle;

    using MoveHandler = std::function<void(const Move&)>;
    using PhaseChangedHandler = std::function<void(SelectionPhase)>;

    AmazonsBoardCanvas();

    void setGameState(GameState* state);
    void setMoveHandler(MoveHandler handler);
    void setPhaseChangedHandler(PhaseChangedHandler handler);
    void setInteractionEnabled(bool enabled);
    void setAiThinking(bool thinking);
    void notifyMoveApplied(const Move& move);
    void resetSelections();
    void setBoardStyle(Style style);
    void setCustomColors(td::ColorID lightColor, td::ColorID darkColor);
    [[nodiscard]] SelectionPhase currentPhase() const { return _phase; }
    void setAnimationFinishedHandler(std::function<void()> handler);
    [[nodiscard]] bool isAnimating() const;
    void showGameOverOverlay(bool victory);

protected:
    void onDraw(const gui::Rect& rect) override;
    void onResize(const gui::Size& size) override;
    void onPrimaryButtonPressed(const gui::InputDevice& inputDevice) override;
    bool onTimer(gui::Timer* pTimer) override;

private:
    struct AnimationState {
        bool active = false;
        Position from;
        Position to;
        double progress = 0.0;
        double duration = 0.25;
        TileContent tile = TileContent::Empty;
    };

    GameState* _state = nullptr;
    MoveHandler _moveHandler;
    PhaseChangedHandler _phaseHandler;
    std::function<void()> _animationFinishedHandler;

    SelectionPhase _phase = SelectionPhase::SelectQueen;
    bool _interactionEnabled = true;
    bool _aiThinking = false;

    gui::Size _viewSize;
    gui::Rect _boardRect;
    double _cellSize = 0.0;
    double _padding = 24.0;
    Style _boardStyle = Style::Wooden;
    td::ColorID _customLightColor = td::ColorID::White;
    td::ColorID _customDarkColor = td::ColorID::SaddleBrown;
    
    // Theme tile images
    gui::Image _lightTileImage;
    gui::Image _darkTileImage;

    Position _selectedQueen;
    Position _selectedDestination;
    std::vector<Position> _queenTargets;
    std::vector<Position> _arrowTargets;

    std::optional<Move> _lastMove;
    AnimationState _queenAnimation;
    AnimationState _arrowAnimation;
    int _impactFrames = 0;
    bool _arrowAnimationQueued = false;
    bool _impactPending = false;
    bool _pendingBlazeSound = false;

    gui::Sound _soundArrowFly;
    gui::Sound _soundBlazeHit;

    gui::Timer _animationTimer;
    
    // Victory/Defeat overlay
    gui::Image _victoryImage;
    gui::Image _defeatImage;
    bool _showGameOverOverlay = false;
    bool _isVictory = false;
    gui::Timer _overlayTimer;

    struct ImageResources {
        inline static gui::Image whiteQueen;
        inline static gui::Image blackQueen;
        inline static gui::Image arrow;
        inline static gui::Image impact;
        inline static bool loaded = false;

        static void preload();
        static double calculateArrowAngle(int dRow, int dCol);
    private:
        static void loadImage(gui::Image& image, const char* id);
    };

    void computeBoardGeometry();
    [[nodiscard]] gui::Rect cellRect(const Position& pos) const;
    [[nodiscard]] gui::Point cellCenter(const Position& pos) const;
    [[nodiscard]] gui::Rect insetRect(gui::Rect rect, double inset) const;
    [[nodiscard]] Position hitTest(const gui::Point& framePoint) const;

    void handleSelectQueen(const Position& hitTile);
    void handleSelectDestination(const Position& hitTile);
    void handleSelectArrow(const Position& hitTile);

    void setPhase(SelectionPhase newPhase);
    void updateQueenTargets();
    void updateArrowTargets();
    void startAnimationsForMove(const Move& move);
    void ensureAnimationTimerRunning();

    void drawBoardGrid() const;
    void drawHighlights() const;
    void drawArrows() const;
    void drawQueens() const;
    void drawAnimations() const;
    void drawAiOverlay() const;

    static bool containsPosition(const std::vector<Position>& list, const Position& value);
};

inline AmazonsBoardCanvas::AmazonsBoardCanvas()
: Canvas({gui::InputDevice::Event::PrimaryClicks})
, _animationTimer(this, 1.0f / 60.0f, false)
, _soundArrowFly(gui::getResFileName("arrow-sound"))
, _soundBlazeHit(gui::getResFileName("blaze-sound"))
, _victoryImage(gui::getResFileName("victory-img"))
, _defeatImage(gui::getResFileName("defeat-img"))
, _overlayTimer(this, 2.0f, false)
{
    enableResizeEvent(true);
}

inline void AmazonsBoardCanvas::setGameState(GameState* state) {
    _state = state;
    resetSelections();
    computeBoardGeometry();
    reDraw();
}

inline void AmazonsBoardCanvas::setBoardStyle(Style style) {
    _boardStyle = style;
    
    // Load theme tile images if needed using proper resource loading
    auto loadThemeImage = [](gui::Image& image, const char* id) {
        td::String fn = gui::getResFileName(id);
        bool hasPath = fn.c_str() && fn.c_str()[0] != '\0';
        if (!hasPath) {
            td::String colonId(":");
            colonId += id;
            fn = colonId;
        }
        image.load(fn);
    };
    
    if (style == Style::Wooden) {
        loadThemeImage(_lightTileImage, "wooden_light");
        loadThemeImage(_darkTileImage, "wooden_dark");
    } else if (style == Style::IceTheme) {
        loadThemeImage(_lightTileImage, "ice_light");
        loadThemeImage(_darkTileImage, "ice_dark");
    } else if (style == Style::StoneTheme) {
        loadThemeImage(_lightTileImage, "stone_light");
        loadThemeImage(_darkTileImage, "stone_dark");
    } else if (style == Style::DiamondTheme) {
        loadThemeImage(_lightTileImage, "diamond_light");
        loadThemeImage(_darkTileImage, "diamond_dark");
    } else if (style == Style::TournamentTheme) {
        loadThemeImage(_lightTileImage, "tournament_light");
        loadThemeImage(_darkTileImage, "tournament_dark");
    }
    
    reDraw();
}

inline void AmazonsBoardCanvas::setCustomColors(td::ColorID lightColor, td::ColorID darkColor) {
    _customLightColor = lightColor;
    _customDarkColor = darkColor;
    if (_boardStyle == Style::CustomTheme) {
        reDraw();
    }
}

inline void AmazonsBoardCanvas::setMoveHandler(MoveHandler handler) {
    _moveHandler = std::move(handler);
}

inline void AmazonsBoardCanvas::setPhaseChangedHandler(PhaseChangedHandler handler) {
    _phaseHandler = std::move(handler);
}

inline void AmazonsBoardCanvas::setInteractionEnabled(bool enabled) {
    _interactionEnabled = enabled;
    if (!enabled) {
        resetSelections();
    }
    reDraw();
}

inline void AmazonsBoardCanvas::setAiThinking(bool thinking) {
    _aiThinking = thinking;
    reDraw();
}

inline void AmazonsBoardCanvas::notifyMoveApplied(const Move& move) {
    _lastMove = move;
    resetSelections();
    startAnimationsForMove(move);
    reDraw();
}

inline void AmazonsBoardCanvas::resetSelections() {
    _selectedQueen = {};
    _selectedDestination = {};
    _queenTargets.clear();
    _arrowTargets.clear();
    _arrowAnimationQueued = false;
    _impactPending = false;
    setPhase(SelectionPhase::SelectQueen);
}

inline void AmazonsBoardCanvas::onDraw(const gui::Rect& rect) {
    if (!_state) {
        return;
    }
    ImageResources::preload();
    drawBoardGrid();
    drawHighlights();
    drawArrows();
    drawQueens();
    drawAnimations();
    drawAiOverlay();
    
    // Draw game over overlay (victory/defeat)
    if (_showGameOverOverlay) {
        // Calculate image rect centered on board
        gui::Rect overlayRect;
        gui::CoordType imageSize = _cellSize * 5; // Make it 5 cells wide for better visibility
        overlayRect.left = _boardRect.left + (_boardRect.right - _boardRect.left - imageSize) / 2;
        overlayRect.top = _boardRect.top + (_boardRect.bottom - _boardRect.top - imageSize) / 2;
        overlayRect.right = overlayRect.left + imageSize;
        overlayRect.bottom = overlayRect.top + imageSize;
        
        if (_isVictory) {
            if (_victoryImage.isOK()) {
                _victoryImage.draw(overlayRect);
            }
        } else {
            if (_defeatImage.isOK()) {
                _defeatImage.draw(overlayRect);
            }
        }
    }
}

inline void AmazonsBoardCanvas::onResize(const gui::Size& size) {
    _viewSize = size;
    computeBoardGeometry();
}

inline void AmazonsBoardCanvas::onPrimaryButtonPressed(const gui::InputDevice& inputDevice) {
    if (!_state || !_interactionEnabled || _aiThinking || _state->isFinished()) {
        gui::Sound::play(gui::Sound::Type::Beep);
        return;
    }
    auto hit = hitTest(inputDevice.getFramePoint());
    if (!hit.isValid()) {
        return;
    }
    switch (_phase) {
        case SelectionPhase::SelectQueen:
            handleSelectQueen(hit);
            break;
        case SelectionPhase::SelectDestination:
            handleSelectDestination(hit);
            break;
        case SelectionPhase::SelectArrow:
            handleSelectArrow(hit);
            break;
        default:
            break;
    }
}

inline bool AmazonsBoardCanvas::onTimer(gui::Timer* pTimer) {
    // Handle overlay timer
    if (pTimer == &_overlayTimer) {
        _showGameOverOverlay = false;
        _overlayTimer.stop();
        reDraw();
        return true;
    }
    
    bool keepRunning = false;
    auto advance = [](AnimationState& anim, double delta) {
        if (!anim.active) {
            return false;
        }
        anim.progress = std::min(1.0, anim.progress + delta / std::max(anim.duration, 0.001));
        if (anim.progress >= 1.0) {
            anim.active = false;
            return false;
        }
        return true;
    };

    bool queenActive = advance(_queenAnimation, 1.0 / 60.0);
    if (!queenActive && !_arrowAnimation.active && _arrowAnimationQueued && _lastMove.has_value()) {
        _arrowAnimation.active = true;
        _arrowAnimation.from = _lastMove->queenTo;
        _arrowAnimation.to = _lastMove->arrow;
        _arrowAnimation.progress = 0.0;
        _arrowAnimation.duration = 0.5;
        _arrowAnimation.tile = TileContent::Arrow;
        _arrowAnimationQueued = false;
        // Play arrow flight sound when arrow animation starts.
        _soundArrowFly.play();
    }
    bool arrowActive = advance(_arrowAnimation, 1.0 / 60.0);
    if (!arrowActive && !_arrowAnimation.active && _impactFrames == 0 && _impactPending && _lastMove.has_value()) {
        _impactFrames = 18;
        _impactPending = false;
        // Schedule blaze impact sound to play after all animations finish
        // (this ensures the sound is heard when the final blaze remains, not on the temporary preview).
        _pendingBlazeSound = true;
    }
    if (_impactFrames > 0) {
        --_impactFrames;
        keepRunning = true;
    }
    keepRunning = keepRunning || queenActive || arrowActive;
    if (!keepRunning) {
        // If a blaze sound was scheduled for this move, play it now — after the temporary impact frames expired
        // so the sound corresponds to the final, persistent blaze on the board.
        if (_pendingBlazeSound && _lastMove.has_value()) {
            _pendingBlazeSound = false;
            _soundBlazeHit.play();
        }

        _animationTimer.stop();
        if (_animationFinishedHandler) {
            // move handler out and clear to avoid reentrancy
            auto h = _animationFinishedHandler;
            _animationFinishedHandler = nullptr;
            h();
        }
    }
    reDraw();
    return true;
}

inline void AmazonsBoardCanvas::setAnimationFinishedHandler(std::function<void()> handler) {
    _animationFinishedHandler = std::move(handler);
}

inline bool AmazonsBoardCanvas::isAnimating() const {
    return _queenAnimation.active || _arrowAnimation.active || _arrowAnimationQueued || _impactPending || (_impactFrames > 0);
}

inline void AmazonsBoardCanvas::showGameOverOverlay(bool victory) {
    _showGameOverOverlay = true;
    _isVictory = victory;
    _overlayTimer.start();
    reDraw();
}

inline void AmazonsBoardCanvas::computeBoardGeometry() {
    if (!_state) {
        return;
    }
    int dim = _state->board().dimension();
    if (dim <= 0) {
        return;
    }
    double available = std::min(_viewSize.width, _viewSize.height) - 2.0 * _padding;
    _cellSize = available / dim;
    double width = _cellSize * dim;
    double left = (_viewSize.width - width) / 2.0;
    double top = (_viewSize.height - width) / 2.0;
    _boardRect = gui::Rect(left, top, left + width, top + width);
}

inline gui::Rect AmazonsBoardCanvas::cellRect(const Position& pos) const {
    double x = _boardRect.left + pos.col * _cellSize;
    double y = _boardRect.top + pos.row * _cellSize;
    return gui::Rect(x, y, x + _cellSize, y + _cellSize);
}

inline gui::Point AmazonsBoardCanvas::cellCenter(const Position& pos) const {
    auto rect = cellRect(pos);
    return gui::Point((rect.left + rect.right) * 0.5, (rect.top + rect.bottom) * 0.5);
}

inline gui::Rect AmazonsBoardCanvas::insetRect(gui::Rect rect, double inset) const {
    rect.left += inset;
    rect.top += inset;
    rect.right -= inset;
    rect.bottom -= inset;
    return rect;
}

inline Position AmazonsBoardCanvas::hitTest(const gui::Point& framePoint) const {
    Position pos;
    if (!_state || !_boardRect.contains(framePoint)) {
        return pos;
    }
    int col = static_cast<int>((framePoint.x - _boardRect.left) / _cellSize);
    int row = static_cast<int>((framePoint.y - _boardRect.top) / _cellSize);
    if (_state->board().isInsideBoard(row, col)) {
        pos.row = row;
        pos.col = col;
    }
    return pos;
}

inline void AmazonsBoardCanvas::handleSelectQueen(const Position& hitTile) {
    const auto tile = _state->board().getTile(hitTile.row, hitTile.col);
    auto expected = tileForPlayer(_state->currentPlayer());
    if (tile != expected) {
        // Only play a beep when the user clicks the opponent's queen.
        // Clicking empty tiles or arrows should be silent.
        auto opponentTile = tileForPlayer(opponentOf(_state->currentPlayer()));
        if (tile == opponentTile) {
            gui::Sound::play(gui::Sound::Type::Beep);
        }
        return;
    }
    // If the user clicks the already-selected queen, toggle deselect.
    if (_selectedQueen.isValid() && _selectedQueen == hitTile) {
        resetSelections();
        reDraw();
        return;
    }

    _selectedQueen = hitTile;
    updateQueenTargets();
    setPhase(SelectionPhase::SelectDestination);
    reDraw();
}

inline void AmazonsBoardCanvas::handleSelectDestination(const Position& hitTile) {
    const auto tile = _state->board().getTile(hitTile.row, hitTile.col);
    auto expected = tileForPlayer(_state->currentPlayer());
    if (tile == expected) {
        handleSelectQueen(hitTile);
        return;
    }
    if (!containsPosition(_queenTargets, hitTile)) {
        // If the clicked tile is not a valid destination for the selected queen,
        // clear the selection and return to queen selection phase so the path
        // highlighting is hidden and the user can pick another queen.
        resetSelections();
        reDraw();
        return;
    }
    _selectedDestination = hitTile;
    updateArrowTargets();
    setPhase(SelectionPhase::SelectArrow);
    reDraw();
}

inline void AmazonsBoardCanvas::handleSelectArrow(const Position& hitTile) {
    if (!containsPosition(_arrowTargets, hitTile)) {
        gui::Sound::play(gui::Sound::Type::Beep);
        return;
    }
    Move move;
    move.player = _state->currentPlayer();
    move.queenFrom = _selectedQueen;
    move.queenTo = _selectedDestination;
    move.arrow = hitTile;
    if (_moveHandler) {
        _moveHandler(move);
    }
}

inline void AmazonsBoardCanvas::setPhase(SelectionPhase newPhase) {
    _phase = newPhase;
    if (_phaseHandler) {
        _phaseHandler(_phase);
    }
}

inline void AmazonsBoardCanvas::updateQueenTargets() {
    _queenTargets = gatherReachableTiles(_state->board(), _selectedQueen);
}

inline void AmazonsBoardCanvas::updateArrowTargets() {
    Board simulated = _state->board();
    auto tile = tileForPlayer(_state->currentPlayer());
    simulated.setTile(_selectedQueen.row, _selectedQueen.col, TileContent::Empty);
    simulated.setTile(_selectedDestination.row, _selectedDestination.col, tile);
    _arrowTargets = gatherReachableTiles(simulated, _selectedDestination);
}

inline void AmazonsBoardCanvas::startAnimationsForMove(const Move& move) {
    _queenAnimation.active = true;
    _queenAnimation.from = move.queenFrom;
    _queenAnimation.to = move.queenTo;
    _queenAnimation.progress = 0.0;
    _queenAnimation.duration = 0.25;
    _queenAnimation.tile = tileForPlayer(move.player);

    _arrowAnimation.active = false;
    _arrowAnimation.progress = 0.0;
    _impactFrames = 0;
    _arrowAnimationQueued = true;
    _impactPending = true;
    ensureAnimationTimerRunning();
}

inline void AmazonsBoardCanvas::ensureAnimationTimerRunning() {
    if (!_animationTimer.isRunning()) {
        _animationTimer.start();
    }
}

inline void AmazonsBoardCanvas::drawBoardGrid() const {
    const auto& board = _state->board();
    int dim = board.dimension();
    for (int row = 0; row < dim; ++row) {
        for (int col = 0; col < dim; ++col) {
            auto rect = cellRect({row, col});
            bool light = ((row + col) % 2) == 0;
            
            // Image-based themes
            if (_boardStyle == Style::Wooden || _boardStyle == Style::IceTheme || 
                _boardStyle == Style::StoneTheme || _boardStyle == Style::DiamondTheme || 
                _boardStyle == Style::TournamentTheme) {
                const gui::Image& tileImage = light ? _lightTileImage : _darkTileImage;
                if (tileImage.isOK()) {
                    // Use simpler draw for better performance
                    tileImage.draw(rect);
                }
            }
            // Color-based themes
            else {
                gui::Shape cell;
                cell.createRect(rect);
                
                if (_boardStyle == Style::BlackWhite) {
                    auto fillColor = light ? td::ColorID::White : td::ColorID::Black;
                    cell.drawFillAndWire(fillColor, td::ColorID::Silver);
                } else if (_boardStyle == Style::BubblegumTheme) {
                    auto fillColor = light ? td::ColorID::White : td::ColorID::LightPink;
                    cell.drawFillAndWire(fillColor, td::ColorID::HotPink);
                } else if (_boardStyle == Style::CustomTheme) {
                    auto fillColor = light ? _customLightColor : _customDarkColor;
                    cell.drawFillAndWire(fillColor, td::ColorID::Black);
                } else {
                    // Fallback
                    auto fillColor = light ? td::ColorID::SaddleBrown : td::ColorID::BurlyWood;
                    cell.drawFillAndWire(fillColor, td::ColorID::Black);
                }
            }
        }
    }
}

inline void AmazonsBoardCanvas::drawHighlights() const {
    if (!_state) {
        return;
    }
    auto drawHighlight = [this](const Position& pos, td::ColorID color, td::ColorID /*wire*/) {
        auto rect = cellRect(pos);
        // Draw a semi-transparent fill. Border intentionally omitted (transparent).
        gui::Shape::drawRect(rect, 0.5f, color);
    };

    if (_lastMove.has_value()) {
        drawHighlight(_lastMove->queenFrom, td::ColorID::PaleTurquoise, td::ColorID::Black);
        drawHighlight(_lastMove->queenTo, td::ColorID::DarkSeaGreen, td::ColorID::Black);
        drawHighlight(_lastMove->arrow, td::ColorID::SandyBrown, td::ColorID::Black);
    }

    if (_selectedQueen.isValid()) {
        drawHighlight(_selectedQueen, td::ColorID::LightSkyBlue, td::ColorID::Black);
    }
    if (_phase == SelectionPhase::SelectDestination) {
        for (const auto& pos : _queenTargets) {
            drawHighlight(pos, td::ColorID::PaleGreen, td::ColorID::Black);
        }
    }
    if (_phase == SelectionPhase::SelectArrow) {
        for (const auto& pos : _arrowTargets) {
            drawHighlight(pos, td::ColorID::LightSalmon, td::ColorID::Black);
        }
        drawHighlight(_selectedDestination, td::ColorID::DarkSeaGreen, td::ColorID::Black);
    }
}

inline void AmazonsBoardCanvas::drawArrows() const {
    for (const auto& pos : _state->arrowPositions()) {
        if (_arrowAnimation.active && pos == _arrowAnimation.to) {
            continue;
        }
        auto rect = insetRect(cellRect(pos), _cellSize * 0.1);
        ImageResources::impact.draw(rect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center);
    }
}

inline void AmazonsBoardCanvas::drawQueens() const {
    auto drawQueen = [this](const Position& pos, Player player) {
        auto rect = insetRect(cellRect(pos), _cellSize * 0.05);
        const gui::Image& img = (player == Player::White) ? ImageResources::whiteQueen : ImageResources::blackQueen;
        img.draw(rect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center);
    };

    for (const auto& pos : _state->queenPositions(Player::White)) {
        if (_queenAnimation.active && pos == _queenAnimation.to && _queenAnimation.tile == TileContent::WhiteQueen) {
            continue;
        }
        drawQueen(pos, Player::White);
    }
    for (const auto& pos : _state->queenPositions(Player::Black)) {
        if (_queenAnimation.active && pos == _queenAnimation.to && _queenAnimation.tile == TileContent::BlackQueen) {
            continue;
        }
        drawQueen(pos, Player::Black);
    }
}

inline void AmazonsBoardCanvas::drawAnimations() const {
    auto drawMovingPiece = [this](const AnimationState& anim) {
        if (!anim.active) {
            return;
        }
        auto start = cellCenter(anim.from);
        auto end = cellCenter(anim.to);
        gui::Point current(start.x + (end.x - start.x) * anim.progress,
                           start.y + (end.y - start.y) * anim.progress);
        double radius = _cellSize * 0.35;
        gui::Rect rect(current.x - radius, current.y - radius, current.x + radius, current.y + radius);
        gui::Shape shape;
        if (anim.tile == TileContent::Arrow) {
            rect = insetRect(rect, _cellSize * 0.1);
            int dRow = anim.to.row - anim.from.row;
            int dCol = anim.to.col - anim.from.col;
            double angle = ImageResources::calculateArrowAngle(dRow, dCol);
            
            gui::Transformation::saveContext();
            gui::Transformation t;
            gui::Point center((rect.left + rect.right) * 0.5, (rect.top + rect.bottom) * 0.5);
            t.translate(center.x, center.y);
            t.rotateDeg(angle);
            t.translate(-center.x, -center.y);
            t.appendToContext();
            
            ImageResources::arrow.draw(rect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center);
            
            gui::Transformation::restoreContext();
        } else {
            auto player = (anim.tile == TileContent::WhiteQueen) ? Player::White : Player::Black;
            const gui::Image& img = (player == Player::White) ? ImageResources::whiteQueen : ImageResources::blackQueen;
            img.draw(rect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center);
        }
    };

    drawMovingPiece(_queenAnimation);
    drawMovingPiece(_arrowAnimation);

    if (_impactFrames > 0 && _lastMove.has_value()) {
        auto rect = insetRect(cellRect(_lastMove->arrow), _cellSize * 0.1);
        ImageResources::impact.draw(rect, gui::Image::AspectRatio::Keep, td::HAlignment::Center, td::VAlignment::Center);
    }
}

inline void AmazonsBoardCanvas::drawAiOverlay() const {
    if (!_aiThinking) {
        return;
    }
    // Draw a subtle boxed label centered over the board while AI is thinking.
    gui::DrawableString ds(tr("aiThinking"));
    gui::Size sz;
    ds.measure(gui::Font::ID::SystemLargestBold, sz);

    // Box metrics with small padding
    const double paddingX = 12.0;
    const double paddingY = 6.0;
    double boxW = sz.width + 2.0 * paddingX;
    double boxH = sz.height + 2.0 * paddingY;

    // Center box over the board rect (both horizontally and vertically) if available,
    // otherwise center in the view.
    bool boardRectValid = (_boardRect.right > _boardRect.left) && (_boardRect.bottom > _boardRect.top);
    double centerX;
    double centerY;
    if (boardRectValid) {
        centerX = (_boardRect.right + _boardRect.left) * 0.5;
        centerY = (_boardRect.top + _boardRect.bottom) * 0.5;
    } else {
        centerX = _viewSize.width * 0.5;
        centerY = _viewSize.height * 0.5;
    }

    gui::Rect boxRect(centerX - boxW * 0.5, centerY - boxH * 0.5, centerX + boxW * 0.5, centerY + boxH * 0.5);

    // Draw semi-transparent background and a subtle border
    gui::Shape::drawRect(boxRect, 0.9f, td::ColorID::SysCtrlBack);
    gui::Shape border;
    border.createRect(boxRect);
    border.drawWire(td::ColorID::Gray);

    // Draw the text centered in the box with neutral color
    gui::Point origin(boxRect.left + paddingX, boxRect.top + paddingY);
    ds.draw(origin, gui::Font::ID::SystemLargestBold, td::ColorID::SysText);
}

inline void AmazonsBoardCanvas::ImageResources::preload() {
    if (loaded) {
        return;
    }
    loadImage(whiteQueen, "whiteQueen");
    loadImage(blackQueen, "blackQueen");
    loadImage(arrow, "arrow");
    loadImage(impact, "blaze");
    loaded = true;
}

inline double AmazonsBoardCanvas::ImageResources::calculateArrowAngle(int dRow, int dCol) {
    // Calculate angle in degrees. atan2 gives angle from positive x-axis.
    // dCol is x-axis (positive right), dRow is y-axis (positive down)
    // Arrow image points to the right (0 degrees)
    double angleRad = std::atan2(static_cast<double>(dRow), static_cast<double>(dCol));
    double angleDeg = angleRad * 180.0 / 3.14159265358979323846;
    return angleDeg;
}

inline void AmazonsBoardCanvas::ImageResources::loadImage(gui::Image& image, const char* id) {
    td::String fn = gui::getResFileName(id);
    bool hasPath = fn.c_str() && fn.c_str()[0] != '\0';
    if (!hasPath) {
        td::String colonId(":");
        colonId += id;
        fn = colonId;
    }
    image.load(fn);
}

inline bool AmazonsBoardCanvas::containsPosition(const std::vector<Position>& list, const Position& value) {
    return std::any_of(list.begin(), list.end(), [&value](const Position& pos) { return pos == value; });
}
