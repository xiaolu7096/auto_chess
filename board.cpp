#include "board.h"
#include <algorithm>

Board::Board() {
    clear();
}

void Board::addUnit(int x, int y, Unit* unit) {
    if (!unit || !isValidPosition(x, y)) return;
    cells[x][y] = unit;
}

void Board::removeUnit(int x, int y) {
    if (!isValidPosition(x, y)) return;
    cells[x][y] = nullptr;
}

Unit* Board::getUnitAt(int x, int y) const {
    if (!isValidPosition(x, y)) return nullptr;
    return cells[x][y];
}

bool Board::hasUnitAt(int x, int y) const {
    return getUnitAt(x, y) != nullptr;
}

bool Board::isValidPosition(int x, int y) const {
    return x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS;
}

void Board::clear() {
    for (int x = 0; x < BOARD_COLS; x++) {
        for (int y = 0; y < BOARD_ROWS; y++) {
            cells[x][y] = nullptr;
        }
    }
}

int Board::indexOf(int x, int y) const {
    if (!isValidPosition(x, y)) return -1;
    return y * BOARD_COLS + x;
}
