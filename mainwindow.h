#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QKeyEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QStringList>
#include <QTimer>

#include "gamemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    // Qt event handlers: draw the game, receive mouse drag/clicks, and handle hotkeys.
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // Main timer callback: advances game logic and refreshes the screen.
    void onGameTick();

private:
    Ui::MainWindow *ui;
    GameManager* gameMgr;

    // Main layout constants: board, bench, item bar, and shop are arranged top to bottom.
    const int CELL_SIZE = 60;
    const int OFFSET_X = 100;
    const int OFFSET_Y = 50;
    const int BENCH_Y = 550;
    const int ITEM_GRID_Y = 620;
    const int SHOP_Y = 680;

    // Right-side panel and in-game operation buttons.
    const int PANEL_X = 620;
    const QRect REFRESH_BTN_RECT = QRect(620, 400, 140, 35);
    const QRect BUY_XP_BTN_RECT = QRect(620, 445, 140, 35);
    const QRect SAVE_BTN_RECT = QRect(620, 490, 140, 35);
    const QRect LOAD_BTN_RECT = QRect(620, 535, 140, 35);

    // Start menu buttons and save-list area.
    const QRect MENU_START_RECT = QRect(300, 220, 200, 48);
    const QRect MENU_SAVE_RECT = QRect(300, 285, 200, 48);
    const QRect MENU_LIST_RECT = QRect(210, 380, 380, 260);

    // Shop card and item icon sizes.
    const int SHOP_CARD_W = 90;
    const int SHOP_CARD_H = 60;
    const int SHOP_GAP = 15;
    const int ITEM_SIZE = 40;

    // Current item-dragging state.
    int selectedItemIndex = -1;
    bool isDraggingItem = false;

    // Current unit-dragging and focused-unit state.
    Unit* selectedUnit = nullptr;
    QPoint dragPos;
    Unit* focusedUnit = nullptr;

    // Start menu state and discovered save files.
    bool inStartMenu = true;
    QStringList saveFiles;

    // Start menu drawing and interaction helpers.
    void drawStartMenu(QPainter& painter);
    bool handleStartMenuClick(const QPoint& pos);
    void refreshSaveList();
    QString createTimestampSavePath() const;
    void resetInteractionState();

    // Converts a pixel position to a logical board/bench coordinate; y == -1 means bench.
    std::pair<int, int> getLogicalPos(QPoint pos) {
        int lx = (pos.x() - OFFSET_X) / CELL_SIZE;
        int ly = (pos.y() - OFFSET_Y) / CELL_SIZE;

        if (lx >= 0 && lx < 8 && ly >= 0 && ly < 8) return {lx, ly};

        if (lx >= 0 && lx < 8 && pos.y() >= BENCH_Y && pos.y() <= BENCH_Y + CELL_SIZE) {
            return {lx, -1};
        }

        return {-1, -1};
    }

    QTimer* gameTimer;
};

#endif // MAINWINDOW_H
