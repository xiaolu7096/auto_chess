#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Disable std::filesystem usage in Qt
#ifndef QT_NO_FILESYSTEM
#define QT_NO_FILESYSTEM
#endif

#include <QKeyEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QStringList>
#include <QTimer>

#include "gamemanager.h"
#include "rightcontrolpanel.h"

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
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // Main timer callback: advances game logic and refreshes the screen.
    void onGameTick();
    // Right panel button handlers.
    void onPauseRequested();
    void onRefreshShopRequested();
    void onBuyXPRequested();

private:
    Ui::MainWindow *ui;
    GameManager* gameMgr;
    RightControlPanel* m_rightPanel;

    // Main layout constants: board, bench, item bar, and shop are arranged top to bottom.
    const int CELL_SIZE = 60;
    const int OFFSET_X = 100;
    const int OFFSET_Y = 50;
    const int BENCH_Y = 550;
    const int ITEM_GRID_Y = 620;
    const int SHOP_Y = 680;

    // Right-side panel design-space X offset.
    const int PANEL_X = 620;

    // Start menu buttons and save-list area.
    const QRect MENU_START_RECT = QRect(300, 200, 200, 48);
    const QRect MENU_HELP_RECT = QRect(300, 265, 200, 48);
    const QRect MENU_SAVE_RECT = QRect(300, 330, 200, 48);
    const QRect MENU_LIST_RECT = QRect(210, 410, 380, 230);

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

    // Drag target highlighting: shows where the dragged unit may land.
    QPoint dragHoverTarget = QPoint(-1, -1);
    bool dragHoverIsBench = false;
    bool dragHoverValid = false;

    // Start menu state and discovered save files.
    bool inStartMenu = true;
    QStringList saveFiles;
    bool showManual = false;
    int manualScrollOffset = 0;

    // Pause state.
    bool isPaused = false;

    // Window-scale factors for resize support.
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;

    // Converts a widget-pixel position into design-space coordinates for event handlers.
    QPoint designPos(const QPoint& pixelPos) const {
        return QPoint(pixelPos.x() / m_scaleX, pixelPos.y() / m_scaleY);
    }

    // Start menu drawing and interaction helpers.
    void drawStartMenu(QPainter& painter);
    bool handleStartMenuClick(const QPoint& pos);
    void drawHelpScreen(QPainter& painter);
    bool handleHelpScreenClick(const QPoint& pos);
    void drawPauseMenu(QPainter& painter);
    bool handlePauseMenuClick(const QPoint& pos);
    void refreshSaveList();
    QString createTimestampSavePath() const;
    void resetInteractionState();

    // Helper: get hero skill description text.
    static QString getHeroSkillDesc(const std::string& name);

    // === 模块化绘制函数：每块独立维护，便于增删改查 ===
    void drawBoardGrid(QPainter& painter);
    void drawSkillEffects(QPainter& painter);
    void drawBench(QPainter& painter);
    void drawDragTargetHighlight(QPainter& painter);
    void drawDragShadow(QPainter& painter);
    void drawShopCards(QPainter& painter);
    void drawTraitSidebar(QPainter& painter);
    void drawItemBench(QPainter& painter);
    void drawProjectiles(QPainter& painter);
    void drawBattleResult(QPainter& painter);
    void drawBottomHint(QPainter& painter);

    // === 模块化事件处理函数 ===
    bool tryStartItemDrag(const QPoint& pos);
    bool tryShopCardPurchase(const QPoint& pos);
    void tryStartUnitDrag(const QPoint& pos);
    bool handleItemEquip(const QPoint& pos);
    void handleUnitPlace(const QPoint& pos);
    void handleStartMenuKeys(QKeyEvent* event);
    void handleInGameKeys(QKeyEvent* event);

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

    void loadImages();
    QPixmap getHeroPixmap(Unit* u);

    QHash<QString, QPixmap> heroPixmaps;
    QPixmap enemyPixmap1;
    QPixmap enemyPixmap2;
    QHash<ItemType, QPixmap> itemPixmaps;
    QPixmap projectileArrow;
    QPixmap projectileMagic;
    QPixmap hitEffectPixmap;

    QHash<QString, QPixmap> skillEffectPixmaps;
};

#endif // MAINWINDOW_H
