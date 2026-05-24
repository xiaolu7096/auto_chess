#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QMouseEvent>
#include "gamemanager.h" // 引入你的大脑
#include <QTimer>//时钟头文件

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
    // --- 核心：重写父类函数 ---
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event)override;//重写键盘事件（方便我们用空格键测试进入战斗）
private slots:
    //时钟每跳动一次，就触发这个槽函数
    void onGameTick();
private:
    Ui::MainWindow *ui;
    GameManager* gameMgr; // 逻辑管理器

    // --- 布局常量 (你可以根据喜好调整) ---
    const int CELL_SIZE = 60;   // 格子大小
    const int OFFSET_X = 100;   // 棋盘左偏移
    const int OFFSET_Y = 50;    // 棋盘上偏移
    const int BENCH_Y = 550;    // 备战区高度
    const int PANEL_X=620;//侧边栏起始像素图标

    const int SHOP_Y = 620;        // 商店区域的起始Y坐标
    const int SHOP_CARD_W = 90;    // 商店每张英雄卡片的宽度
    const int SHOP_CARD_H = 60;    // 商店每张英雄卡片的高度
    const int SHOP_GAP = 15;       // 卡片之间的间距
    const QRect REFRESH_BTN_RECT = QRect(620, 400, 140, 35);
    const QRect BUY_XP_BTN_RECT=QRect(620, 450, 140, 35);
    const int ITEM_GRID_Y = 560; // 渲染的纵坐标，刚好在备战区下方
    const int ITEM_SIZE = 40;   // 每个装备框 40x40 像素

    int selectedItemIndex = -1;  // 当前用鼠标抓着的装备索引（-1表示空闲）
    bool isDraggingItem = false; // 是否处于拖拽装备状态
    // --- 交互状态 ---
    Unit* selectedUnit = nullptr; // 当前抓起的英雄
    QPoint dragPos;               // 鼠标当前的像素位置
    Unit*focusedUnit=nullptr;//当前点击选中的查看单位（职业..）

    // --- 辅助函数：像素坐标转逻辑坐标 ---
    // 返回值：first是x, second是y。如果点在备战区，y设为-1

    std::pair<int, int>getLogicalPos(QPoint pos) {
        int lx = (pos.x() - OFFSET_X) / CELL_SIZE;
        int ly = (pos.y() - OFFSET_Y) / CELL_SIZE;

        // 检查是否在棋盘内
        if(lx >= 0 && lx < 8 && ly >= 0 && ly < 8) return {lx, ly};

        // 检查是否在备战区
        if(lx >= 0 && lx < 8 && pos.y() >= BENCH_Y && pos.y() <= BENCH_Y + CELL_SIZE) {
            return {lx, -1}; // y=-1 表示备战区
        }

        return {-1, -1}; // 非法位置
    }
    //--时钟
    QTimer*gameTimer;//驱动游戏的主时钟
};
#endif // MAINWINDOW_H
