#ifndef BOARD_H
#define BOARD_H

#include "unit.h"

static constexpr int BOARD_ROWS = 8;
static constexpr int BOARD_COLS = 8;

class Board {
public:
    Board();
    ~Board() = default;

    void addUnit(int x, int y, Unit* unit);
    void removeUnit(int x, int y);
    Unit* getUnitAt(int x, int y) const;
    bool hasUnitAt(int x, int y) const;
    bool isValidPosition(int x, int y) const;
    void clear();

private:
    int indexOf(int x, int y) const;
    Unit* cells[BOARD_ROWS][BOARD_COLS];
};

#endif // BOARD_H
