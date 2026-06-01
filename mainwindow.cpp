#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "heroes.h"
#include "player.h"
#include <QCoreApplication>
#include <QDateTime>
// Disable std::filesystem usage in Qt
#ifndef QT_NO_FILESYSTEM
#define QT_NO_FILESYSTEM
#endif
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <iostream>

static QColor heroColor(Unit* u) {
    if (u->owner == Owner::EnemyCtrl) {
        if (u->name == "Garen")      return QColor(50, 120, 240);
        if (u->name == "Ryze")       return QColor(150, 80, 220);
        if (u->name == "Soraka")     return QColor(40, 180, 150);
        if (u->name == "Leona")      return QColor(240, 180, 50);
        if (u->name == "Ashe")       return QColor(60, 180, 240);
        if (u->name == "Jhin")       return QColor(200, 50, 80);
        return QColor(240, 60, 60);
    }
    if (u->name == "Garen")      return QColor(50, 120, 240);
    if (u->name == "Ryze")       return QColor(150, 80, 220);
    if (u->name == "Soraka")     return QColor(40, 180, 150);
    if (u->name == "Leona")     return QColor(240, 180, 50);
    if (u->name == "Ashe")      return QColor(60, 180, 240);
    if (u->name == "Jhin")      return QColor(200, 50, 80);
    return QColor(50, 120, 240);
}

static QColor itemColor(Item* item) {
    if (item->type == ItemType::Sword)  return QColor(220, 20, 60);
    if (item->type == ItemType::Armor)  return QColor(70, 130, 180);
    if (item->type == ItemType::Glove)  return QColor(46, 139, 87);
    if (item->type == ItemType::Crystal) return QColor(30, 144, 255);
    if (item->type == ItemType::Advanced) {
        if (item->name == "复活甲")     return QColor(255, 215, 0);
        if (item->name == "无尽之刃")   return QColor(180, 30, 30);
        if (item->name == "大天使之杖")  return QColor(100, 150, 255);
        if (item->name == "荆棘之甲")   return QColor(100, 100, 130);
        if (item->name == "狂徒铠甲")   return QColor(40, 120, 40);
        if (item->name == "卢登的回声")  return QColor(200, 50, 200);
        return QColor(200, 180, 50);
    }
    return QColor(200, 200, 200);
}
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setMenuBar(nullptr);
    statusBar()->setVisible(false);
    gameMgr = new GameManager();

    setMinimumSize(800, 790);
    resize(800, 790);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_rightPanel = new RightControlPanel(gameMgr, centralWidget());
    connect(m_rightPanel, &RightControlPanel::pauseRequested, this, &MainWindow::onPauseRequested);
    connect(m_rightPanel, &RightControlPanel::refreshShopRequested, this, &MainWindow::onRefreshShopRequested);
    connect(m_rightPanel, &RightControlPanel::buyXPRequested, this, &MainWindow::onBuyXPRequested);

    loadImages();
    gameTimer=new QTimer(this);
    connect(gameTimer,&QTimer::timeout,this,&MainWindow::onGameTick);
    gameTimer->start(33);
    refreshSaveList();
    update();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete gameMgr;
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    m_scaleX = (float)width() / 800.0f;
    m_scaleY = (float)height() / 790.0f;
    float s = std::min(m_scaleX, m_scaleY);
    m_scaleX = s;
    m_scaleY = s;

    const int panelDesignX = 610;
    const int panelDesignW = 190;
    int px = (int)(panelDesignX * m_scaleX);
    int py = 0;
    int pw = (int)(panelDesignW * m_scaleX);
    int ph = (int)(790 * m_scaleY);
    m_rightPanel->setGeometry(px, py, pw, ph);
}

void MainWindow::refreshSaveList() {
    // 扫描 saves 目录，按文件修改时间倒序列出历史存档。
    QDir dir("saves");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.json",
                                            QDir::Files,
                                            QDir::Time);
    saveFiles.clear();
    for (const QFileInfo& file : files) {
        saveFiles.append(file.filePath());
    }
}

QString MainWindow::createTimestampSavePath() const {
    // 为每次保存生成独立文件名，避免覆盖旧存档。
    QDir dir("saves");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return dir.filePath("save_" + stamp + ".json");
}

void MainWindow::resetInteractionState() {
    selectedUnit = nullptr;
    focusedUnit = nullptr;
    selectedItemIndex = -1;
    isDraggingItem = false;
    dragHoverTarget = QPoint(-1, -1);
    dragHoverIsBench = false;
    dragHoverValid = false;
}

void MainWindow::loadImages() {
    auto findResource = [](const QString& filename) -> QString {
        QDir dir(QCoreApplication::applicationDirPath());
        for (int levels = 0; levels <= 8; levels++) {
            QString resPath = dir.absoluteFilePath("resources/" + filename);
            if (QFile::exists(resPath)) return resPath;
            if (!dir.cdUp()) break;
        }
        if (QFile::exists("resources/" + filename))
            return QDir::current().absoluteFilePath("resources/" + filename);
        return QString();
    };

    auto loadScaled = [&](const QString& filename, int w, int h) -> QPixmap {
        QString fullPath = findResource(filename);
        if (fullPath.isEmpty()) return QPixmap();
        QPixmap pix(fullPath);
        if (pix.isNull()) return QPixmap();
        return pix.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    };

    heroPixmaps["Garen"]  = loadScaled("garen.png", 50, 50);
    heroPixmaps["Ryze"]   = loadScaled("ryze.png", 50, 50);
    heroPixmaps["Soraka"] = loadScaled("soraka.png", 50, 50);
    heroPixmaps["Leona"]  = loadScaled("leona.png", 50, 50);
    heroPixmaps["Ashe"]   = loadScaled("ashe.png", 50, 50);
    heroPixmaps["Jhin"]   = loadScaled("jhin.png", 50, 50);

    enemyPixmap1 = loadScaled("enemy1.png", 50, 50);
    enemyPixmap2 = loadScaled("enemy2.png", 50, 50);

    itemPixmaps[ItemType::Sword]   = loadScaled("item_sword.png", 16, 16);
    itemPixmaps[ItemType::Armor]   = loadScaled("item_armer.png", 16, 16);
    itemPixmaps[ItemType::Glove]   = loadScaled("item_glove.png", 16, 16);
    itemPixmaps[ItemType::Crystal] = loadScaled("item_crystal.png", 16, 16);

    projectileArrow = loadScaled("projectile_arrow.png", 24, 24);
    projectileMagic = loadScaled("projectile_magic.png", 30, 30);
    hitEffectPixmap = loadScaled("hit_effect.png", 40, 40);

    skillEffectPixmaps["garen_spin"]   = loadScaled("skill_garen_spin.png", 70, 70);
    skillEffectPixmaps["ryze_blast"]   = loadScaled("skill_ryze_blast.png", 48, 48);
    skillEffectPixmaps["soraka_heal"]  = loadScaled("skill_soraka_heal.png", 80, 80);
    skillEffectPixmaps["leona_shield"] = loadScaled("skill_leona_shield.png", 48, 48);
    skillEffectPixmaps["ashe_arrow"]   = loadScaled("skill_ashe_arrow.png", 48, 48);
    skillEffectPixmaps["jhin_snipe"]   = loadScaled("skill_jhin_snipe.png", 48, 48);
}

QPixmap MainWindow::getHeroPixmap(Unit* u) {
    if (!u) return QPixmap();
    if (u->owner == Owner::EnemyCtrl) {
        return (u->x % 2 == 0) ? enemyPixmap1 : enemyPixmap2;
    }
    QString name = QString::fromStdString(u->name);
    if (heroPixmaps.contains(name)) {
        return heroPixmaps[name];
    }
    return QPixmap();
}

void MainWindow::drawStartMenu(QPainter& painter) {
    // 初始界面：提供新游戏、保存当前进度、历史存档入口。
    painter.fillRect(rect(), QColor(238, 242, 246));

    painter.setPen(QColor(30, 40, 55));
    painter.setFont(QFont("Microsoft YaHei", 28, QFont::Bold));
    painter.drawText(QRect(0, 110, width(), 60), Qt::AlignCenter, "Synera Auto-Arena");

    painter.setFont(QFont("Microsoft YaHei", 11));
    painter.setPen(QColor(80, 90, 105));
    painter.drawText(QRect(0, 155, width(), 30), Qt::AlignCenter, "开始新游戏，或从历史存档继续。");

    painter.setBrush(QColor(74, 132, 210));
    painter.setPen(QPen(QColor(46, 94, 160), 1));
    painter.drawRect(MENU_START_RECT);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    painter.drawText(MENU_START_RECT, Qt::AlignCenter, "开始新游戏");

    painter.setBrush(QColor(155, 120, 60));
    painter.setPen(QPen(QColor(120, 85, 30), 1));
    painter.drawRect(MENU_HELP_RECT);
    painter.setPen(Qt::white);
    painter.drawText(MENU_HELP_RECT, Qt::AlignCenter, "游戏说明");

    painter.setBrush(QColor(92, 170, 120));
    painter.setPen(QPen(QColor(52, 125, 82), 1));
    painter.drawRect(MENU_SAVE_RECT);
    painter.setPen(Qt::white);
    painter.drawText(MENU_SAVE_RECT, Qt::AlignCenter, "保存当前进度");

    painter.setBrush(QColor(255, 255, 255));
    painter.setPen(QPen(QColor(185, 195, 205), 1));
    painter.drawRect(MENU_LIST_RECT);

    painter.setPen(QColor(30, 40, 55));
    painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    painter.drawText(MENU_LIST_RECT.adjusted(16, 12, -16, -220), Qt::AlignLeft, "历史存档");

    painter.setFont(QFont("Microsoft YaHei", 9));
    if (saveFiles.empty()) {
        painter.setPen(QColor(130, 140, 150));
        painter.drawText(MENU_LIST_RECT.adjusted(16, 58, -16, -16), Qt::AlignLeft, "暂无存档。");
        return;
    }

    int rowY = MENU_LIST_RECT.top() + 48;
    int maxRows = saveFiles.size()> 7?7:saveFiles.size();
    for (int i = 0; i < maxRows; ++i) {
        QRect rowRect(MENU_LIST_RECT.left() + 12, rowY + i * 28, MENU_LIST_RECT.width() - 24, 24);
        painter.setBrush(i % 2 == 0 ? QColor(244, 247, 250) : QColor(255, 255, 255));
        painter.setPen(Qt::NoPen);
        painter.drawRect(rowRect);

        QFileInfo info(saveFiles[i]);
        painter.setPen(QColor(40, 55, 75));
        QString label = info.completeBaseName() + "    " + info.lastModified().toString("MM-dd hh:mm");
        painter.drawText(rowRect.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
    }
}

bool MainWindow::handleStartMenuClick(const QPoint& pos) {
    // 处理初始界面的按钮和存档列表点击；返回 true 表示事件已消费。
    if (MENU_START_RECT.contains(pos)) {
        delete gameMgr;
        gameMgr = new GameManager();
        resetInteractionState();
        inStartMenu = false;
        update();
        return true;
    }

    if (MENU_HELP_RECT.contains(pos)) {
        showManual = true;
        manualScrollOffset = 0;
        update();
        return true;
    }

    if (MENU_SAVE_RECT.contains(pos)) {
        gameMgr->saveGame(createTimestampSavePath());
        refreshSaveList();
        update();
        return true;
    }

    int rowY = MENU_LIST_RECT.top() + 48;
    int maxRows = saveFiles.size()> 7?7:saveFiles.size();
    for (int i = 0; i < maxRows; ++i) {
        QRect rowRect(MENU_LIST_RECT.left() + 12, rowY + i * 28, MENU_LIST_RECT.width() - 24, 24);
        if (rowRect.contains(pos)) {
            if (gameMgr->loadGame(saveFiles[i])) {
                resetInteractionState();
                inStartMenu = false;
            }
            update();
            return true;
        }
    }

    return false;
}

QString MainWindow::getHeroSkillDesc(const std::string& name) {
    if (name == "Garen") return "【旋风斩】对周围3x3范围造成80点AOE伤害";
    if (name == "Ryze")  return "【超负荷法球】对目标造成150点法术伤害";
    if (name == "Soraka") return "【祈愿】为全场友军回复80点生命值";
    if (name == "Leona") return "【日蚀】对目标造成120伤害并眩晕";
    if (name == "Ashe")  return "【万箭齐发】纵向3格AOE各100点伤害";
    if (name == "Jhin")  return "【完美谢幕】锁定最低血量敌人造成200点伤害";
    return "未知技能";
}

void MainWindow::drawPauseMenu(QPainter& painter) {
    painter.fillRect(rect(), QColor(0, 0, 0, 160));

    int panelW = 260, panelH = 170;
    int px = (width() - panelW) / 2, py = (height() - panelH) / 2;
    QRect panel(px, py, panelW, panelH);

    painter.setBrush(QColor(40, 44, 52));
    painter.setPen(QPen(QColor(80, 90, 110), 2));
    painter.drawRoundedRect(panel, 12, 12);

    painter.setPen(QColor(220, 220, 240));
    painter.setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    painter.drawText(QRect(px, py + 6, panelW, 36), Qt::AlignCenter, "游戏暂停");

    QRect continueBtn(px + 30, py + 50, panelW - 60, 38);
    painter.setBrush(QColor(74, 132, 210));
    painter.setPen(QPen(QColor(46, 94, 160), 1));
    painter.drawRoundedRect(continueBtn, 6, 6);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    painter.drawText(continueBtn, Qt::AlignCenter, "继续游戏");

    QRect exitBtn(px + 30, py + 104, panelW - 60, 38);
    painter.setBrush(QColor(200, 70, 70));
    painter.setPen(QPen(QColor(150, 40, 40), 1));
    painter.drawRoundedRect(exitBtn, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(exitBtn, Qt::AlignCenter, "返回主菜单 (自动保存)");
}

bool MainWindow::handlePauseMenuClick(const QPoint& pos) {
    int panelW = 260, panelH = 170;
    int px = (width() - panelW) / 2, py = (height() - panelH) / 2;

    QRect continueBtn(px + 30, py + 50, panelW - 60, 38);
    if (continueBtn.contains(pos)) {
        isPaused = false;
        update();
        return true;
    }

    QRect exitBtn(px + 30, py + 104, panelW - 60, 38);
    if (exitBtn.contains(pos)) {
        gameMgr->saveGame(createTimestampSavePath());
        refreshSaveList();
        isPaused = false;
        inStartMenu = true;
        showManual = false;
        resetInteractionState();
        update();
        return true;
    }
    return false;
}

void MainWindow::drawHelpScreen(QPainter& painter) {
    painter.fillRect(rect(), QColor(30, 35, 45));

    painter.setPen(QColor(220, 220, 240));
    painter.setFont(QFont("Microsoft YaHei", 20, QFont::Bold));
    painter.drawText(QRect(0, 20, width(), 40), Qt::AlignCenter, "游戏说明 —— Synera Auto-Arena");

    painter.setPen(QColor(140, 150, 170));
    painter.setFont(QFont("Microsoft YaHei", 9));
    painter.drawText(QRect(0, 58, width(), 20), Qt::AlignCenter, "点击任意位置或按 ESC 返回");

    const int textX = 40, startY = 90;
    int y = startY + manualScrollOffset;
    const int lineH = 16;

    struct Line { QString text; QColor color; int size; bool bold; };
    std::vector<Line> lines = {
        {"【游戏目标】", QColor(74, 220, 120), 11, true},
        {"通过商店购买英雄、搭配羁绊、合成装备，在8x8战场上排兵布阵，击败逐轮增强的敌人。", QColor(200, 200, 210), 9, false},
        {"", QColor(0,0,0), 9, false},
        {"【操作指南】", QColor(74, 180, 240), 11, true},
        {"点击商店卡片购买英雄 | 拖拽英雄到棋盘第5-8行 | 拖拽装备到英雄身上穿戴", QColor(200, 200, 210), 9, false},
        {"空格键开始战斗 | S键快速保存 | L键快速读取 | ESC键暂停", QColor(200, 200, 210), 9, false},
        {"", QColor(0,0,0), 9, false},
        {"【六位英雄】", QColor(240, 180, 50), 11, true},
        {"盖伦(1G/Vanguard) 450HP/35ATK/近战 - 旋风斩: 3x3范围80点AOE", QColor(200, 200, 210), 9, false},
        {"瑞兹(2G/Mage) 300HP/45ATK/射程4 - 超负荷: 单体150点法术伤害", QColor(200, 200, 210), 9, false},
        {"索拉卡(2G/Healer) 250HP/20ATK/射程3 - 祈愿: 全场友军回复80生命", QColor(200, 200, 210), 9, false},
        {"蕾欧娜(2G/Knight) 480HP/28ATK/近战 - 日蚀: 单体120伤害+眩晕", QColor(200, 200, 210), 9, false},
        {"艾希(2G/Knight) 260HP/42ATK/射程5 - 万箭齐发: 纵向3格各100伤害", QColor(200, 200, 210), 9, false},
        {"烬(2G/Knight) 240HP/60ATK/射程4 - 完美谢幕: 锁定最低血敌人200伤害", QColor(200, 200, 210), 9, false},
        {"", QColor(0,0,0), 9, false},
        {"【羁绊系统】", QColor(200, 120, 220), 11, true},
        {"Vanguard(先锋)>=1名: 所有先锋+120生命", QColor(200, 200, 210), 9, false},
        {"Mage(法师)>=2名: 所有法师最大法力-20", QColor(200, 200, 210), 9, false},
        {"Knight(骑士)>=2名: 全体+100生命、+15攻击", QColor(200, 200, 210), 9, false},
        {"Healer(治愈者)>=2名: 全体+80生命、+5攻击", QColor(200, 200, 210), 9, false},
        {"", QColor(0,0,0), 9, false},
        {"【装备系统】", QColor(220, 80, 60), 11, true},
        {"铁剑(+15攻击) | 锁子甲(+150生命) | 急速手套(+20%攻速) | 蓝水晶(最大法力-30)", QColor(200, 200, 210), 9, false},
        {"两件基础装备拖到同一英雄身上自动合成高级装备:", QColor(200, 200, 210), 9, false},
        {"铁剑+锁子甲=复活甲(阵亡满血复活) | 铁剑+手套=无尽之刃(25%暴击2倍伤害)", QColor(200, 200, 210), 9, false},
        {"铁剑+蓝水晶=大天使之杖(初始+30法力) | 锁子甲+手套=荆棘之甲(反弹30%伤害)", QColor(200, 200, 210), 9, false},
        {"锁子甲+蓝水晶=狂徒铠甲(每2秒回5%HP) | 手套+蓝水晶=卢登的回声(技能触发AOE)", QColor(200, 200, 210), 9, false},
        {"2星英雄可穿戴2件装备, 1星仅1件。", QColor(200, 200, 210), 9, false},
        {"", QColor(0,0,0), 9, false},
        {"【升星系统】", QColor(255, 210, 50), 11, true},
        {"3个同名同星英雄自动合成升1星, 生命和攻击x1.8倍。全局自动检测。", QColor(200, 200, 210), 9, false},
        {"", QColor(0,0,0), 9, false},
        {"【经济系统】", QColor(100, 200, 200), 11, true},
        {"胜利+6G | 战败+5G | 每10金币+1利息(上限5G) | 3连击起额外奖励", QColor(200, 200, 210), 9, false},
        {"购买经验(4G获4XP)升级提升人口上限。存钱吃利息是核心策略。", QColor(200, 200, 210), 9, false},
    };

    for (const auto& line : lines) {
        if (y > startY - lineH && y < height() - 30) {
            painter.setFont(QFont("Microsoft YaHei", line.size, line.bold ? QFont::Bold : QFont::Normal));
            painter.setPen(line.color);
            painter.drawText(textX, y, line.text);
        }
        y += lineH;
    }

    painter.setPen(QColor(100, 110, 130));
    painter.setFont(QFont("Microsoft YaHei", 8));
    painter.drawText(QRect(0, height() - 25, width(), 20), Qt::AlignCenter, "滚轮可上下翻页");
}

bool MainWindow::handleHelpScreenClick(const QPoint& pos) {
    Q_UNUSED(pos);
    showManual = false;
    manualScrollOffset = 0;
    update();
    return true;
}
void MainWindow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(m_scaleX, m_scaleY);

    if (inStartMenu) {
        if (showManual) {
            drawHelpScreen(painter);
        } else {
            drawStartMenu(painter);
        }
        return;
    }

    // 按 Z-Order 从底到顶依次调用模块化绘制函数，每个模块由独立函数维护，便于增删改。
    drawBoardGrid(painter);
    drawSkillEffects(painter);
    drawBench(painter);
    drawDragTargetHighlight(painter);
    drawDragShadow(painter);
    drawShopCards(painter);
    drawTraitSidebar(painter);
    drawItemBench(painter);
    drawProjectiles(painter);
    drawBattleResult(painter);
    // drawBottomHint(painter);

    if (isPaused) { drawPauseMenu(painter); }
}

// ================================================================
// 以下为 paintEvent 拆分出的 11 个独立绘制模块函数。
// 每个函数只负责一块 UI 区域的绘制，相互无耦合。
// ================================================================

// 遍历 8×8 战场网格，绘制棋盘底色、单位头像、血条、蓝条、星级与穿戴装备。
void MainWindow::drawBoardGrid(QPainter& painter) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            // j<4=上半场敌方偏红, j>=4=下半场我方偏蓝
            painter.setBrush(j < 4 ? QColor(255, 230, 230) : QColor(230, 230, 255));
            painter.setPen(QColor(200, 200, 200));
            painter.drawRect(OFFSET_X + i*CELL_SIZE, OFFSET_Y + j*CELL_SIZE, CELL_SIZE, CELL_SIZE);

            // 跳过正被拖拽的单位，避免绘出残影
            Unit* u = gameMgr->getUnitOnBoard(i, j);
            if(!u || u == selectedUnit) continue;

            int px = OFFSET_X + i * CELL_SIZE;
            int py = OFFSET_Y + j * CELL_SIZE;

            // 单位头像：优先用PNG图片，其次用纯色圆形
            QPixmap heroPix = getHeroPixmap(u);
            if (!heroPix.isNull()) {
                painter.drawPixmap(px + 5, py + 5, 50, 50, heroPix);
            } else {
                painter.setBrush(heroColor(u));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(px + 5, py + 5, 50, 50);
            }

            // 我方单位头顶金色星级方块
            if (u->owner == Owner::PlayerCtrl) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(255, 215, 0));
                for (int k = 0; k < u->star; k++) {
                    int startX = px + (50 - u->star * 10) / 2;
                    painter.drawRect(startX + k * 12, py - 5, 8, 8);
                }
            }

            // 技能施法金色外圈特效
            if (u->state == UnitState::Casting) {
                painter.setPen(QPen(QColor(255, 215, 0), 3));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(px + 3, py + 3, 54, 54);
            }

            // 动态血量绿条（黑底+绿色百分比填充）
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::black);
            painter.drawRect(px + 5, py + 2, 50, 4);
            painter.setBrush(Qt::green);
            float hpRatio = std::max(0.0f, (float)u->hp / u->maxHp);
            painter.drawRect(px + 5, py + 2, (int)(50 * hpRatio), 4);

            // 蓝条（法力条）：仅在有最大法力时渲染
            if(u->maxMana > 0) {
                painter.setBrush(Qt::black);
                painter.drawRect(px + 5, py + 7, 50, 4);
                painter.setBrush(Qt::cyan);
                float manaRatio = std::max(0.0f, (float)u->mana / u->maxMana);
                painter.drawRect(px + 5, py + 7, (int)(50 * manaRatio), 4);
            }

            // 英雄脚下微型装备图标（右下角排列）
            for (size_t k = 0; k < u->equippedItems.size(); k++) {
                Item* item = u->equippedItems[k];
                QRect itemRect(px + 6 + k * 15, py + 42, 12, 12);
                if (itemPixmaps.count(item->type)) {
                    painter.drawPixmap(itemRect, itemPixmaps[item->type]);
                } else {
                    if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
                    else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
                    else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
                    else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));
                    else painter.setBrush(itemColor(item));
                    painter.setPen(QPen(Qt::white, 1));
                    painter.drawRect(itemRect);
                }
            }
        }
    }
}

// 技能施法时的半透明特效叠加层，覆盖在棋盘单位上方展示。
void MainWindow::drawSkillEffects(QPainter& painter) {
    for (const auto& e : gameMgr->skillEffects) {
        int px = OFFSET_X + e.x * CELL_SIZE;
        int py = OFFSET_Y + e.y * CELL_SIZE;
        QString key = QString::fromStdString(e.skillName);
        painter.setOpacity(0.45);
        if (skillEffectPixmaps.contains(key)) {
            QPixmap& sp = skillEffectPixmaps[key];
            painter.drawPixmap(px - (sp.width() - CELL_SIZE) / 2,
                               py - (sp.height() - CELL_SIZE) / 2, sp);
        } else {
            painter.setBrush(QColor(255, 255, 255, 100));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(px + 5, py + 5, 50, 50);
        }
        painter.setOpacity(1.0);
    }
}

// 下方备战区 8 格横排：浅灰背景 + 待命英雄 + 装备图标。
void MainWindow::drawBench(QPainter& painter) {
    for(int i = 0; i < 8; i++) {
        painter.setBrush(QColor(220, 220, 220));
        painter.setPen(QColor(160, 160, 160));
        painter.drawRect(OFFSET_X + i*CELL_SIZE, BENCH_Y, CELL_SIZE, CELL_SIZE);

        Unit* u = gameMgr->getUnitOnBench(i);
        if(!u || u == selectedUnit) continue;

        int px = OFFSET_X + i * CELL_SIZE;
        QPixmap heroPix = getHeroPixmap(u);
        if (!heroPix.isNull()) {
            painter.drawPixmap(px + 5, BENCH_Y + 5, 50, 50, heroPix);
        } else {
            painter.setBrush(heroColor(u));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(px + 5, BENCH_Y + 5, 50, 50);
        }

        // 备战区英雄脚下的微型穿戴装备
        for (size_t k = 0; k < u->equippedItems.size(); k++) {
            Item* item = u->equippedItems[k];
            QRect itemRect(px + 6 + k * 15, BENCH_Y + 42, 12, 12);
            if (itemPixmaps.count(item->type)) {
                painter.drawPixmap(itemRect, itemPixmaps[item->type]);
            } else {
                if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
                else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
                else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
                else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));
                else painter.setBrush(itemColor(item));
                painter.setPen(QPen(Qt::white, 1));
                painter.drawRect(itemRect);
            }
        }
    }
}

// 拖拽英雄时，在目标位置绘制绿色(合法)或红色(非法)的高亮边框。
void MainWindow::drawDragTargetHighlight(QPainter& painter) {
    if (!selectedUnit || dragHoverTarget.x() < 0) return;

    int hx = dragHoverTarget.x();
    int hy = dragHoverTarget.y();
    if (dragHoverIsBench) {
        int px = OFFSET_X + hx * CELL_SIZE;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(dragHoverValid ? QColor(100, 220, 100) : QColor(220, 100, 100), 3));
        painter.drawRect(px, BENCH_Y, CELL_SIZE, CELL_SIZE);
    } else if (hy >= 0 && hy < 8) {
        int px = OFFSET_X + hx * CELL_SIZE;
        int py = OFFSET_Y + hy * CELL_SIZE;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(dragHoverValid ? QColor(100, 220, 100) : QColor(220, 100, 100), 3));
        painter.drawRect(px, py, CELL_SIZE, CELL_SIZE);
    }
}

// 鼠标拖拽跟随阴影：英雄为半透明黄色圆圈，装备为带标签的彩色方块。
void MainWindow::drawDragShadow(QPainter& painter) {
    // 英雄拖拽半透明黄圈跟随鼠标
    if(selectedUnit) {
        painter.setBrush(QColor(255, 255, 0, 150));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(dragPos.x() - 25, dragPos.y() - 25, 50, 50);
    }

    // 装备拖拽彩色方块跟随鼠标，印装备名称
    if(isDraggingItem && selectedItemIndex != -1 && selectedItemIndex < (int)gameMgr->itemBench.size()) {
        Item* draggingItem = gameMgr->itemBench[selectedItemIndex];
        QRect dragRect(dragPos.x() - ITEM_SIZE/2, dragPos.y() - ITEM_SIZE/2, ITEM_SIZE, ITEM_SIZE);

        if (draggingItem->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
        else if (draggingItem->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
        else if (draggingItem->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
        else if (draggingItem->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));
        else painter.setBrush(itemColor(draggingItem));

        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(dragRect);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
        painter.drawText(dragRect, Qt::AlignCenter, QString::fromStdString(draggingItem->name));
    }
}

// 底部五联抽商店：5 张英雄卡片（名称、羁绊、价格）或"已售罄"空位。
void MainWindow::drawShopCards(QPainter& painter) {
    for (int i = 0; i < 5; i++) {
        int cardX = OFFSET_X + i * (SHOP_CARD_W + SHOP_GAP);
        QRect cardRect(cardX, SHOP_Y, SHOP_CARD_W, SHOP_CARD_H);
        Unit* shopHero = gameMgr->getShopSlot(i);

        if (shopHero != nullptr) {
            // 按英雄名分配独特的浅色背景，一目了然
            if (shopHero->name == "Garen") painter.setBrush(QColor(230, 245, 230));
            else if (shopHero->name == "Ryze") painter.setBrush(QColor(230, 230, 250));
            else if (shopHero->name == "Soraka") painter.setBrush(QColor(225, 245, 235));
            else if (shopHero->name == "Leona") painter.setBrush(QColor(255, 245, 220));
            else if (shopHero->name == "Ashe") painter.setBrush(QColor(225, 240, 250));
            else painter.setBrush(QColor(250, 235, 240));
            painter.setPen(QPen(QColor(140, 140, 140), 1));
            painter.drawRect(cardRect);

            painter.setPen(Qt::black);
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            painter.drawText(cardRect.adjusted(10, 8, 0, 0), Qt::AlignLeft, QString::fromStdString(shopHero->name));

            painter.setPen(QColor(100, 100, 180));
            painter.setFont(QFont("Microsoft YaHei", 7));
            QString traitStr;
            if (!shopHero->traits.empty()) traitStr = QString::fromStdString(shopHero->traits[0]);
            painter.drawText(cardRect.adjusted(10, 22, 0, 0), Qt::AlignLeft, traitStr);

            painter.setPen(QColor(210, 150, 10));
            painter.setFont(QFont("Microsoft YaHei", 8, QFont::Normal));
            painter.drawText(cardRect.adjusted(10, 38, -10, -5), Qt::AlignLeft | Qt::AlignVCenter,
                             QString::number(shopHero->cost) + " 金币");
        } else {
            painter.setBrush(QColor(240, 240, 240));
            QPen dashPen(QColor(180, 180, 180), 1, Qt::DashLine);
            painter.setPen(dashPen);
            painter.drawRect(cardRect);
            painter.setPen(QColor(160, 160, 160));
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Normal));
            painter.drawText(cardRect, Qt::AlignCenter, "已售罄");
        }
    }
}

// 左侧羁绊计数看板：按激活状态（橙色=激活 / 灰色=未激活）列出所有羁绊与计数。
void MainWindow::drawTraitSidebar(QPainter& painter) {
    int traitY = OFFSET_Y;
    painter.setPen(Qt::black);
    painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    painter.drawText(20, traitY, "【 羁绊计数 】");
    traitY += 25;

    if (gameMgr->activeTraitsCount.empty()) {
        painter.setFont(QFont("Arial", 9, QFont::Normal));
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(20, traitY, "暂无激活羁绊");
    } else {
        for (auto const& [trait, count] : gameMgr->activeTraitsCount) {
            int threshold = 1;
            if (trait == "Mage") threshold = 2;
            if (trait == "Knight") threshold = 2;
            if (trait == "Healer") threshold = 2;
            if (trait == "Vanguard") threshold = 1;

            bool isActivated = (count >= threshold);

            if (isActivated) {
                painter.setBrush(QColor(255, 140, 0));
                painter.setPen(Qt::NoPen);
                painter.drawRect(20, traitY - 14, 110, 20);
                painter.setPen(Qt::white);
            } else {
                painter.setBrush(QColor(220, 220, 220));
                painter.setPen(Qt::NoPen);
                painter.drawRect(20, traitY - 14, 110, 20);
                painter.setPen(QColor(100, 100, 100));
            }
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            QString text = QString::fromStdString(trait) + " : " + QString::number(count) + "/" + QString::number(threshold);
            painter.drawText(25, traitY, text);
            traitY += 25;
        }
    }
}

// 底部装备库存栏：虚线格子 + 装备图标 + 名称标签。
void MainWindow::drawItemBench(QPainter& painter) {
    painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    painter.setPen(Qt::black);
    painter.drawText(20, ITEM_GRID_Y - 10, "【 装备库存 】");

    for (int i = 0; i < gameMgr->MAX_ITEM_BENCH; i++) {
        int x = 20 + i * (ITEM_SIZE + 10);
        QRect rect(x, ITEM_GRID_Y, ITEM_SIZE, ITEM_SIZE);

        painter.setBrush(QColor(240, 240, 240));
        painter.setPen(QPen(QColor(180, 180, 180), 1, Qt::DashLine));
        painter.drawRect(rect);

        if (i < (int)gameMgr->itemBench.size()) {
            Item* item = gameMgr->itemBench[i];
            if (itemPixmaps.count(item->type)) {
                painter.drawPixmap(rect.adjusted(3, 3, -3, -3), itemPixmaps[item->type]);
            } else {
                if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
                else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
                else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
                else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));
                else painter.setBrush(itemColor(item));
                painter.setPen(Qt::NoPen);
                painter.drawRect(rect.adjusted(3, 3, -3, -3));
            }

            painter.setPen(Qt::white);
            painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
            painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(item->name));
        }
    }
}

// 弹道飞行物（箭矢/法球）与受击爆炸特效的动画渲染。
void MainWindow::drawProjectiles(QPainter& painter) {
    for (const auto& p : gameMgr->projectiles) {
        if (p.showHit) {
            int tpx = OFFSET_X + p.toX * CELL_SIZE + 5;
            int tpy = OFFSET_Y + p.toY * CELL_SIZE + 5;
            QPixmap& hitPix = hitEffectPixmap;
            if (!hitPix.isNull()) {
                float scale = 1.0f + 0.3f * (float)p.hitTimer / 8.0f;
                int sz = (int)(40 * scale);
                painter.drawPixmap(tpx + 25 - sz/2, tpy + 25 - sz/2, sz, sz, hitPix);
            }
        } else {
            float fx = OFFSET_X + (p.fromX + (p.toX - p.fromX) * p.progress) * CELL_SIZE + 30;
            float fy = OFFSET_Y + (p.fromY + (p.toY - p.fromY) * p.progress) * CELL_SIZE + 30;
            QPixmap& projPix = p.isSkill ? projectileMagic : projectileArrow;
            if (!projPix.isNull()) {
                painter.save();
                painter.translate(fx, fy);
                int dx = p.toX - p.fromX;
                int dy = p.toY - p.fromY;
                float angle = std::atan2((float)dy, (float)dx) * 180.0f / 3.14159f;
                painter.rotate(angle);
                int sz = p.isSkill ? 30 : 24;
                painter.drawPixmap(-sz/2, -sz/2, sz, sz, projPix);
                painter.restore();
            }
        }
    }
}

// 全屏半透明黑色遮罩 + 大号胜负结果文字（VICTORY/DEFEAT/DRAW）。
void MainWindow::drawBattleResult(QPainter& painter) {
    if (gameMgr->resultDisplayTimer <= 0) return;

    painter.setBrush(QColor(0, 0, 0, 180));
    painter.drawRect(0, 0, width(), height());
    QFont victoryFont("Microsoft YaHei", 36, QFont::Bold);
    painter.setFont(victoryFont);

    if (gameMgr->battleResultStr == "VICTORY") {
        painter.setPen(QColor(50, 255, 50));
        painter.drawText(QRect(0, height()/2 - 140, width(), 60), Qt::AlignCenter, "VICTORY");
        painter.setPen(QColor(200, 220, 200));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(QRect(0, height()/2 - 70, width(), 120), Qt::AlignCenter, gameMgr->settlementBreakdown);
    } else if (gameMgr->battleResultStr == "DEFEAT") {
        painter.setPen(QColor(255, 50, 50));
        painter.drawText(QRect(0, height()/2 - 140, width(), 60), Qt::AlignCenter, "DEFEAT");
        painter.setPen(QColor(220, 200, 200));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(QRect(0, height()/2 - 70, width(), 120), Qt::AlignCenter, gameMgr->settlementBreakdown);
    } else if (gameMgr->battleResultStr == "DRAW") {
        painter.setPen(Qt::white);
        painter.drawText(QRect(0, height()/2 - 140, width(), 60), Qt::AlignCenter, "DRAW");
        painter.setPen(QColor(200, 200, 200));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(QRect(0, height()/2 - 70, width(), 120), Qt::AlignCenter, gameMgr->settlementBreakdown);
    }
}

// 底部深色操作提示栏：根据当前游戏阶段显示不同快捷键提示。
void MainWindow::drawBottomHint(QPainter& painter) {
    int barY = height() - 22;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(40, 42, 48));
    painter.drawRect(0, barY, width(), 22);
    painter.setPen(QColor(160, 165, 175));
    painter.setFont(QFont("Microsoft YaHei", 8));
    QString hint;
    if (gameMgr->getState() == GameState::Preparation) {
        hint = "点击商店购买 | 拖拽布阵/穿戴装备 | 空格开战 | S保存 | L读取 | ESC暂停";
    } else if (gameMgr->getState() == GameState::Battle) {
        hint = "战斗自动进行中... | ESC暂停";
    } else {
        hint = "结算中...";
    }
    painter.drawText(QRect(10, barY, width() - 20, 22), Qt::AlignVCenter | Qt::AlignLeft, hint);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    // 起手界面和暂停菜单优先处理，避免穿透点击。
    if (inStartMenu) {
        if (showManual) { handleHelpScreenClick(designPos(event->pos())); }
        else { handleStartMenuClick(designPos(event->pos())); }
        return;
    }
    if (isPaused) { handlePauseMenuClick(designPos(event->pos())); return; }

    QPoint pos = designPos(event->pos());

    // 按优先级依次检测：装备库存格 → 商店卡片 → 棋盘/备战区单位抓取
    if (tryStartItemDrag(pos)) return;
    if (tryShopCardPurchase(pos)) return;
    tryStartUnitDrag(pos);
}

// 检测点击位置是否命中装备库存栏的某个格子，命中则抓起该装备开始拖拽。
bool MainWindow::tryStartItemDrag(const QPoint& pos) {
    for (int i = 0; i < (int)gameMgr->itemBench.size(); i++) {
        int x = 20 + i * (ITEM_SIZE + 10);
        if (QRect(x, ITEM_GRID_Y, ITEM_SIZE, ITEM_SIZE).contains(pos)) {
            selectedItemIndex = i;
            isDraggingItem = true;
            dragPos = pos;
            update();
            return true;
        }
    }
    return false;
}

// 检测点击位置是否在五联抽商店的某张卡片上，命中则尝试购买该英雄。
bool MainWindow::tryShopCardPurchase(const QPoint& pos) {
    if (gameMgr->getState() != GameState::Preparation) return false;

    for (int i = 0; i < 5; i++) {
        int cardX = OFFSET_X + i * (SHOP_CARD_W + SHOP_GAP);
        if (QRect(cardX, SHOP_Y, SHOP_CARD_W, SHOP_CARD_H).contains(pos)) {
            if (gameMgr->buyHeroFromShop(i)) update();
            return true;
        }
    }
    return false;
}

// 检测点击位置是否在棋盘/备战区的英雄单位上，命中则抓起该单位准备拖拽。
void MainWindow::tryStartUnitDrag(const QPoint& pos) {
    if (gameMgr->getState() != GameState::Preparation) return;

    auto [lx, ly] = getLogicalPos(pos);
    if (lx == -1) return;

    Unit* clickedUnit = (ly == -1) ? gameMgr->getUnitOnBench(lx) : gameMgr->getUnitOnBoard(lx, ly);
    if (!clickedUnit) return;

    // 敌方单位、已阵亡单位或 Dead 状态的单位不允许抓取
    if (clickedUnit->owner == Owner::EnemyCtrl || !clickedUnit->isAlive()
        || clickedUnit->state == UnitState::Dead) {
        selectedUnit = nullptr;
        return;
    }

    selectedUnit = clickedUnit;
    focusedUnit = selectedUnit;
    dragPos = pos;
    update();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (inStartMenu || isPaused) {
        return;
    }

    if(selectedUnit) {
        dragPos = designPos(event->pos());

        auto [lx, ly] = getLogicalPos(designPos(event->pos()));
        if (lx >= 0) {
            bool isBench = (ly == -1);
            dragHoverTarget = QPoint(lx, isBench ? -1 : ly);
            dragHoverIsBench = isBench;
            dragHoverValid = gameMgr->canMoveUnit(selectedUnit, lx, isBench ? -1 : ly, isBench);
        } else {
            dragHoverTarget = QPoint(-1, -1);
            dragHoverValid = false;
        }

        update();
    }
    if (isDraggingItem) {
        dragPos = designPos(event->pos());
        update();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (inStartMenu || isPaused) return;

    QPoint pos = designPos(event->pos());

    // 装备掉落处理：尝试将手中装备穿戴到落点英雄身上
    if (isDraggingItem && selectedItemIndex != -1) {
        handleItemEquip(pos);
        return;
    }

    // 单位放置处理：将拖拽的英雄放置到目标棋盘/备战区位置
    if (selectedUnit) {
        handleUnitPlace(pos);
    }
}

// 装备穿戴逻辑：检测拖拽释放位置是否有可穿戴装备的友方英雄。
bool MainWindow::handleItemEquip(const QPoint& pos) {
    isDraggingItem = false;
    Item* draggingItem = gameMgr->itemBench[selectedItemIndex];

    auto [nx, ny] = getLogicalPos(pos);
    Unit* targetHero = nullptr;

    if (nx >= 0 && nx < 8) {
        if (ny >= 0 && ny < 8) targetHero = gameMgr->getUnitOnBoard(nx, ny);
        else if (ny == -1)      targetHero = gameMgr->getUnitOnBench(nx);
    }

    if (targetHero && targetHero->owner == Owner::PlayerCtrl) {
        if (targetHero->equipItem(draggingItem)) {
            gameMgr->itemBench.erase(gameMgr->itemBench.begin() + selectedItemIndex);
            std::cout << "⚔️ " << targetHero->name << " 穿戴了 " << draggingItem->name
                      << "！当前面板ATK: " << targetHero->atk << " | MaxHP: " << targetHero->maxHp << std::endl;
        } else {
            std::cout << "❌ 穿戴失败！该英雄装备槽已满（当前星级限制最多穿戴 "
                      << targetHero->getMaxItemSlots() << " 件）！" << std::endl;
        }
    }

    selectedItemIndex = -1;
    update();
    return true;
}

// 单位放置逻辑：将拖拽的英雄移动到有效目标位置，或回退原位。
void MainWindow::handleUnitPlace(const QPoint& pos) {
    auto [nx, ny] = getLogicalPos(pos);
    if (nx != -1) {
        gameMgr->MoveUnit(selectedUnit, nx, ny, (ny == -1));
    }

    selectedUnit = nullptr;
    dragHoverTarget = QPoint(-1, -1);
    dragHoverIsBench = false;
    dragHoverValid = false;
    update();
}

void MainWindow::onGameTick(){
    if (!inStartMenu && !isPaused) {
        gameMgr->updateTick();
    }
    m_rightPanel->setVisible(!inStartMenu);
    m_rightPanel->Refresh();
    if (focusedUnit) {
        m_rightPanel->SetFocusedUnit(focusedUnit, getHeroSkillDesc(focusedUnit->name));
    }
    update();
}
void MainWindow::keyPressEvent(QKeyEvent*event){
    // 起手界面和游戏中键盘操作互不干扰，分别由独立函数处理。
    if (inStartMenu) {
        handleStartMenuKeys(event);
        return;
    }
    handleInGameKeys(event);
}

// 起手界面键盘操作：ESC 关闭帮助，回车开始新游戏。
void MainWindow::handleStartMenuKeys(QKeyEvent* event) {
    if (showManual) {
        if (event->key() == Qt::Key_Escape) {
            showManual = false;
            manualScrollOffset = 0;
            update();
        }
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        inStartMenu = false;
        update();
    }
}

// 游戏中键盘操作：ESC 切暂停/继续，空格开战，S 快速保存，L 快速读取。
void MainWindow::handleInGameKeys(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        isPaused = !isPaused;
        update();
        return;
    }

    if (isPaused) return;

    if (event->key() == Qt::Key_Space) {
        gameMgr->startBattle();
        update();
    } else if (event->key() == Qt::Key_S) {
        gameMgr->saveGame(createTimestampSavePath());
        refreshSaveList();
        update();
    } else if (event->key() == Qt::Key_L) {
        refreshSaveList();
        if (!saveFiles.empty()) {
            gameMgr->loadGame(saveFiles.first());
            resetInteractionState();
        }
        update();
    }
}

void MainWindow::onPauseRequested() {
    isPaused = true;
    setFocus();
    update();
}

void MainWindow::onRefreshShopRequested() {
    if (gameMgr->getState() == GameState::Preparation) {
        gameMgr->refreshShopManual();
        setFocus();
        update();
    }
}

void MainWindow::onBuyXPRequested() {
    if (gameMgr->getState() == GameState::Preparation) {
        gameMgr->buyXP();
        setFocus();
        update();
    }
}

void MainWindow::wheelEvent(QWheelEvent* event) {
    if (showManual) {
        manualScrollOffset -= event->angleDelta().y() / 8;
        if (manualScrollOffset > 0) manualScrollOffset = 0;
        update();
        event->accept();
        return;
    }
    QMainWindow::wheelEvent(event);
}
