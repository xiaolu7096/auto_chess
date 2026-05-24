#include "mainwindow.h"
#include "ui_mainwindow.h"
#include"heroes.h"
#include"player.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    gameMgr = new GameManager();

    setFixedSize(800, 700); // 设置固定窗口大小
    setMouseTracking(true); // 极其重要：开启后鼠标不按下也能触发 MoveEvent
    //初始化并启动主时钟（每33毫秒跳一次）
    gameTimer=new QTimer(this);
    connect(gameTimer,&QTimer::timeout,this,&MainWindow::onGameTick);
    gameTimer->start(33);
    Unit* testHero = new Unit(100, 20, 1, 100, Owner::PlayerCtrl);
    // 把它放进备战区第 0 格
    // Unit* ryze = new Ryze(Owner::PlayerCtrl);
    // gameMgr->MoveUnit(ryze, 3, 7, false);

    // ✨ 召唤盖伦上阵测试（放在第 7 行第 4 列，顶在前面）
    Unit* garen = new Garen(Owner::PlayerCtrl);
    gameMgr->MoveUnit(garen, 4, 7, false);
    gameMgr->MoveUnit(testHero, 0, 0, true);
    gameMgr->spawnEnemyRound(1);
    update(); // 别忘了通知界面重绘
}

MainWindow::~MainWindow()
{
    delete ui;
    delete gameMgr;
}
// --- 绘图逻辑：每当执行 update() 时，系统会自动调用这个函数 ---
// void MainWindow::paintEvent(QPaintEvent *event) {
//     QPainter painter(this);
//     painter.setRenderHint(QPainter::Antialiasing);

//     // ==================== 1. 绘制棋盘网格 ====================
//     for(int i = 0; i < 8; i++) {
//         for(int j = 0; j < 8; j++) {
//             // 上半场红色（敌方），下半场蓝色（我方）
//             painter.setBrush(j < 4 ? QColor(255, 230, 230) : QColor(230, 230, 255));
//             painter.setPen(QColor(200, 200, 200));
//             painter.drawRect(OFFSET_X + i*CELL_SIZE, OFFSET_Y + j*CELL_SIZE, CELL_SIZE, CELL_SIZE);

//             Unit* u = gameMgr->getUnitOnBoard(i, j);
//             if(u && u != selectedUnit) {
//                 // 根据阵营染颜色
//                 painter.setBrush(u->owner == Owner::PlayerCtrl ? QColor(50, 120, 240) : QColor(240, 60, 60));
//                 painter.setPen(Qt::NoPen);

//                 // 绘制英雄主体圆圈
//                 int px = OFFSET_X + i * CELL_SIZE;
//                 int py = OFFSET_Y + j * CELL_SIZE;
//                 painter.drawEllipse(px + 5, py + 5, 50, 50);
//                 // 在 mainwindow.cpp 的 paintEvent 绘制棋盘英雄的循环内
//                 // 绘制完圆圈主体后：

//                 if (u->owner == Owner::PlayerCtrl) {
//                     painter.setPen(Qt::NoPen);
//                     painter.setBrush(QColor(255, 215, 0)); // 金色代表星星

//                     // 根据 star 数量画小方块或星星图标
//                     for (int k = 0; k < u->star; k++) {
//                         // 在圆圈上方排队画小星星
//                         int starSize = 8;
//                         int startX = px + (50 - u->star * 10) / 2; // 居中排布
//                         painter.drawRect(startX + k * 12, py - 5, starSize, starSize);
//                     }
//                 }
//                 // ✨ 如果在施法(Casting)，加一圈耀眼的金色边框效果！
//                 if (u->state == UnitState::Casting) {
//                     painter.setPen(QPen(QColor(255, 215, 0), 3));
//                     painter.setBrush(Qt::NoBrush);
//                     painter.drawEllipse(px + 3, py + 3, 54, 54);
//                 }

//                 // ==================== 动态绘制血条与蓝条 ====================
//                 // 血条底色（黑）
//                 painter.setPen(Qt::NoPen);
//                 painter.setBrush(Qt::black);
//                 painter.drawRect(px + 5, py + 2, 50, 4);
//                 // 血条当前值（绿）
//                 painter.setBrush(Qt::green);
//                 float hpRatio = std::max(0.0f, (float)u->hp / u->maxHp);
//                 painter.drawRect(px + 5, py + 2, (int)(50 * hpRatio), 4);

//                 // 蓝条（蓝，如果有最大法力值）
//                 if(u->maxMana > 0) {
//                     painter.setBrush(Qt::black);
//                     painter.drawRect(px + 5, py + 7, 50, 4);
//                     painter.setBrush(Qt::cyan);
//                     float manaRatio = std::max(0.0f, (float)u->mana / u->maxMana);
//                     painter.drawRect(px + 5, py + 7, (int)(50 * manaRatio), 4);
//                 }
//                 // ✨【新增】：在棋盘英雄脚下/右下角，绘制他目前穿戴的微型装备图标
//                 for (size_t k = 0; k < u->equippedItems.size(); k++) {
//                     Item* item = u->equippedItems[k];
//                     QRect itemRect(px + 6 + k * 15, py + 42, 12, 12); // 微型 12x12 像素
//                     if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
//                     else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
//                     else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
//                     else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));
//                     painter.setPen(QPen(Qt::white, 1));
//                     painter.drawRect(itemRect);
//                 }
//             }
//         }
//     }

//     // ==================== 2. 绘制备战区 ====================
//     for(int i = 0; i < 8; i++) {
//         painter.setBrush(QColor(220, 220, 220));
//         painter.setPen(QColor(160, 160, 160));
//         painter.drawRect(OFFSET_X + i*CELL_SIZE, BENCH_Y, CELL_SIZE, CELL_SIZE);

//         Unit* u = gameMgr->getUnitOnBench(i);
//         if(u && u != selectedUnit) {
//             painter.setBrush(QColor(50, 120, 240));
//             painter.setPen(Qt::NoPen);
//             painter.drawEllipse(OFFSET_X + i*CELL_SIZE + 5, BENCH_Y + 5, 50, 50);
//             // ✨【新增】：备战区英雄圆圈内同样展示他的神装微型图标
//             for (size_t k = 0; k < u->equippedItems.size(); k++) {
//                 Item* item = u->equippedItems[k];
//                 QRect itemRect(px + 6 + k * 15, BENCH_Y + 42, 12, 12);
//                 if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
//                 else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
//                 else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
//                 else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));
//                 painter.setPen(QPen(Qt::white, 1));
//                 painter.drawRect(itemRect);
//             }
//         }
//     }

//     // ==================== 3. 绘制拖拽中的单位 ====================
//     if(selectedUnit) {
//         painter.setBrush(QColor(255, 255, 0, 150));
//         painter.setPen(Qt::NoPen);
//         painter.drawEllipse(dragPos.x() - 25, dragPos.y() - 25, 50, 50);
//     }
//     // ✨【新增】：绘制随鼠标移动的拖拽中装备
//     if(isDraggingItem && selectedItemIndex != -1 && selectedItemIndex < (int)gameMgr->itemBench.size()) {
//         Item* draggingItem = gameMgr->itemBench[selectedItemIndex];
//         QRect dragRect(dragPos.x() - ITEM_SIZE/2, dragPos.y() - ITEM_SIZE/2, ITEM_SIZE, ITEM_SIZE);

//         if (draggingItem->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
//         else if (draggingItem->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
//         else if (draggingItem->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
//         else if (draggingItem->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));

//         painter.setPen(QPen(Qt::white, 2)); // 耀眼的白色拖拽轮廓
//         painter.drawRect(dragRect);
//         painter.setPen(Qt::white);
//         painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
//         painter.drawText(dragRect, Qt::AlignCenter, QString::fromStdString(draggingItem->name));
//     }

//     // ==================== 4. 绘制右侧综合面板与全局资产 ====================
//     painter.setPen(Qt::black);
//     QFont infoFont = painter.font();
//     infoFont.setPointSize(11);
//     infoFont.setBold(true);
//     painter.setFont(infoFont);

//     painter.drawText(PANEL_X, OFFSET_Y, "当前关卡: 第 " + QString::number(gameMgr->getCurrentRound()) + " 轮");

//     QString stateStr;
//     if (gameMgr->getState() == GameState::Preparation) {
//         stateStr = "【准备阶段】\n操作：拖拽排兵布阵\n快捷键：[空格]开战";
//     } else if (gameMgr->getState() == GameState::Battle) {
//         stateStr = "【战斗进行中...】\n英雄正在自动寻路与施法";
//     } else {
//         stateStr = "【回合结算中】";
//     }

//     painter.setPen(QColor(100, 50, 150));
//     painter.drawText(QRect(PANEL_X, OFFSET_Y + 25, 180, 60), Qt::AlignLeft, stateStr);

//     painter.setPen(Qt::black);
//     painter.drawText(PANEL_X, OFFSET_Y + 100, "--------------------");
//     painter.setPen(QColor(220, 40, 40));
//     painter.drawText(PANEL_X, OFFSET_Y + 120, "玩家血量: " + QString::number(gameMgr->playerHp) + " / 100");
//     painter.setPen(QColor(210, 160, 10));
//     painter.drawText(PANEL_X, OFFSET_Y + 145, "拥有金币: " + QString::number(gameMgr->getPlayerGold()) + " G");
//     painter.setPen(Qt::black);
//     painter.drawText(PANEL_X, OFFSET_Y + 165, "当前人口: " + QString::number(gameMgr->getpoplulation()) + " 级");
//     painter.drawText(PANEL_X, OFFSET_Y + 185, "--------------------");

//     // 5.绘制点击选中的详细属性面板
//     if (focusedUnit) {
//         // 稍微拉长面板高度（由200变240），为展示神装留出空间
//         painter.setBrush(QColor(245, 245, 245));
//         painter.setPen(QColor(180, 180, 180));
//         painter.drawRect(PANEL_X, OFFSET_Y + 180, 160, 240);

//         painter.setPen(Qt::black);
//         painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
//         painter.drawText(PANEL_X + 10, OFFSET_Y + 210, "【 " + QString::fromStdString(focusedUnit->name) + " 的面板 】");

//         painter.setFont(QFont("Microsoft YaHei", 9, QFont::Normal));
//         painter.drawText(PANEL_X + 10, OFFSET_Y + 235, "阵营: " + QString(focusedUnit->owner == Owner::PlayerCtrl ? "我方英雄" : "敌方怪物"));

//         // ✨【优化点3实现】：动态计算并拆解展示装备带来的额外加成
//         int bonusAtkSum = 0;
//         int bonusHpSum = 0;
//         for (auto* item : focusedUnit->equippedItems) {
//             bonusAtkSum += item->bonusAtk;
//             bonusHpSum += item->bonusHp;
//         }

//         QString hpString = "生命: " + QString::number(focusedUnit->hp) + "/" + QString::number(focusedUnit->maxHp);
//         if (bonusHpSum > 0) hpString += " (+" + QString::number(bonusHpSum) + ")";
//         painter.drawText(PANEL_X + 10, OFFSET_Y + 260, hpString);

//         QString atkString = "攻击: " + QString::number(focusedUnit->atk);
//         if (bonusAtkSum > 0) atkString += " (+" + QString::number(bonusAtkSum) + ")";
//         painter.drawText(PANEL_X + 10, OFFSET_Y + 285, atkString);

//         painter.drawText(PANEL_X + 10, OFFSET_Y + 310, "射程: " + QString::number(focusedUnit->range));

//         QString t = "无";
//         if(!focusedUnit->traits.empty()) t = QString::fromStdString(focusedUnit->traits[0]);
//         painter.drawText(PANEL_X + 10, OFFSET_Y + 335, "核心羁绊: " + t);

//         // ✨【优化点3续】：直观在面板底部打印穿戴的装备名字
//         QString gearStr = "已装: ";
//         if (focusedUnit->equippedItems.empty()) gearStr += "暂无装备";
//         else {
//             for (auto* item : focusedUnit->equippedItems) gearStr += "[" + QString::fromStdString(item->name) + "] ";
//         }
//         painter.setPen(QColor(139, 69, 19)); // 棕色复古风显示装备栏
//         painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
//         painter.drawText(PANEL_X + 10, OFFSET_Y + 365, gearStr);
//     }

//     // ==================== 6. ✨ 绘制五联抽商店卡片 ====================
//     for (int i = 0; i < 5; i++) {
//         // 计算每一张卡片的横坐标 X
//         int cardX = OFFSET_X + i * (SHOP_CARD_W + SHOP_GAP);
//         QRect cardRect(cardX, SHOP_Y, SHOP_CARD_W, SHOP_CARD_H);

//         Unit* shopHero = gameMgr->getShopSlot(i);

//         if (shopHero != nullptr) {
//             // 商品存在：根据英雄名字给卡片染上不同的背景色（高级感！）
//             if (shopHero->name == "Garen") {
//                 painter.setBrush(QColor(230, 245, 230)); // 浅绿
//             } else if (shopHero->name == "Ryze") {
//                 painter.setBrush(QColor(230, 230, 250)); // 浅紫
//             } else {
//                 painter.setBrush(QColor(255, 250, 230)); // 浅黄
//             }
//             painter.setPen(QPen(QColor(140, 140, 140), 1));
//             painter.drawRect(cardRect);

//             // 绘制卡片内部文字（英雄名与价格）
//             painter.setPen(Qt::black);
//             painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
//             painter.drawText(cardRect.adjusted(10, 8, 0, 0), Qt::AlignLeft, QString::fromStdString(shopHero->name));

//             painter.setPen(QColor(210, 150, 10)); // 金黄色字体写价格
//             painter.setFont(QFont("Microsoft YaHei", 8, QFont::Normal));
//             painter.drawText(cardRect.adjusted(10, 32, -10, -5), Qt::AlignLeft | Qt::AlignVCenter, QString::number(shopHero->cost) + " 金币");
//         } else {
//             // 商品已被买走：画一个虚线灰色框，表示已售罄
//             painter.setBrush(QColor(240, 240, 240));
//             QPen dashPen(QColor(180, 180, 180), 1, Qt::DashLine);
//             painter.setPen(dashPen);
//             painter.drawRect(cardRect);

//             painter.setPen(QColor(160, 160, 160));
//             painter.setFont(QFont("Microsoft YaHei", 9, QFont::Normal));
//             painter.drawText(cardRect, Qt::AlignCenter, "已售罄");
//         }
//     }

//     // ==================== 7. ✨ 绘制“刷新商店”像素按钮 ====================
//     // 如果在战斗中，按钮变灰不可用；在准备阶段则是耀眼的亮黄色
//     if (gameMgr->getState() == GameState::Preparation) {
//         painter.setBrush(QColor(255, 215, 0)); // 金黄色
//         painter.setPen(QPen(QColor(200, 150, 0), 1));
//     } else {
//         painter.setBrush(QColor(200, 200, 200)); // 禁用灰色
//         painter.setPen(QPen(QColor(160, 160, 160), 1));
//     }
//     painter.drawRect(REFRESH_BTN_RECT);

//     // 按钮文字
//     painter.setPen(gameMgr->getState() == GameState::Preparation ? Qt::black : QColor(120, 120, 120));
//     painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
//     painter.drawText(REFRESH_BTN_RECT, Qt::AlignCenter, "刷新商店 (2G)");
//     // ==================== 7.5 ✨ 绘制“购买经验”像素按钮 ====================
//     if (gameMgr->getState() == GameState::Preparation) {
//         painter.setBrush(QColor(147, 112, 219)); // 耀眼的紫色（代表奥术/经验）
//         painter.setPen(QPen(QColor(100, 50, 150), 1));
//     } else {
//         painter.setBrush(QColor(200, 200, 200)); // 战斗中禁用灰色
//         painter.setPen(QPen(QColor(160, 160, 160), 1));
//     }
//     painter.drawRect(BUY_XP_BTN_RECT);

//     // 按钮文字与当前经验进度
//     painter.setPen(gameMgr->getState() == GameState::Preparation ? Qt::white : QColor(120, 120, 120));
//     painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));

//     // 从 player 拿到当前经验和升级所需经验（假设你给它们写了 getter 接口，如果没有可以直接通过 gameMgr 间接获取，或者先写死显示 "购买经验 (4G)"）
//     painter.drawText(BUY_XP_BTN_RECT, Qt::AlignCenter, "购买经验 (4G)");
//     // ==================== 8. ✨ 绘制左侧实时羁绊看板 ====================
//     int traitY = OFFSET_Y; // 从顶部开始排
//     painter.setPen(Qt::black);
//     painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
//     painter.drawText(20, traitY, "【 羁绊计数 】"); // 坐标 20 刚好在屏幕最左边边缘
//     traitY += 25;

//     if (gameMgr->activeTraitsCount.empty()) {
//         painter.setFont(QFont("Arial", 9, QFont::Normal));
//         painter.setPen(QColor(150, 150, 150));
//         painter.drawText(20, traitY, "暂无激活羁绊");
//     } else {
//         for (auto const& [trait, count] : gameMgr->activeTraitsCount) {
//             // 根据是否达到激活标准染颜色：法师要2个，重装要1个
//             bool isActivated = false;
//             if (trait == "Mage" && count >= 2) isActivated = true;
//             if (trait == "Vanguard" && count >= 1) isActivated = true;

//             if (isActivated) {
//                 painter.setBrush(QColor(255, 140, 0)); // 橙金底色（激活）
//                 painter.setPen(Qt::NoPen);
//                 painter.drawRect(20, traitY - 14, 110, 20);
//                 painter.setPen(Qt::white); // 白字
//             } else {
//                 painter.setBrush(QColor(220, 220, 220)); // 灰底色（未激活）
//                 painter.setPen(Qt::NoPen);
//                 painter.drawRect(20, traitY - 14, 110, 20);
//                 painter.setPen(QColor(100, 100, 100)); // 灰字
//             }

//             painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
//             QString text = QString::fromStdString(trait) + " : " + QString::number(count);
//             painter.drawText(25, traitY, text);
//             traitY += 25;
//         }
//     }// ==================== ✨ 9.绘制装备库存栏 ====================
//     painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
//     painter.setPen(Qt::black);
//     painter.drawText(20, ITEM_GRID_Y - 10, "【 装备库存 】");

//     for (int i = 0; i < gameMgr->MAX_ITEM_BENCH; i++) {
//         int x = 20 + i * (ITEM_SIZE + 10); // 每个格子横向间隔 10 像素
//         QRect rect(x, ITEM_GRID_Y, ITEM_SIZE, ITEM_SIZE);

//         // 绘制虚线灰色背景格
//         painter.setBrush(QColor(240, 240, 240));
//         painter.setPen(QPen(QColor(180, 180, 180), 1, Qt::DashLine));
//         painter.drawRect(rect);

//         // 如果这个格子里有真实装备
//         if (i < (int)gameMgr->itemBench.size()) {
//             Item* item = gameMgr->itemBench[i];

//             // 根据装备类型，渲染高辨识度的炫彩颜色
//             if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));     // 猩红铁剑
//             else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));   // 钢蓝锁子甲
//             else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));    // 海绿手套
//             else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255)); // 闪耀蓝水晶

//             painter.setPen(Qt::NoPen);
//             painter.drawRect(rect.adjusted(3, 3, -3, -3)); // 稍微往内缩一点，留出质感边框

//             // 绘制装备简短文本
//             painter.setPen(Qt::white);
//             painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
//             painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(item->name));
//         }
//     }
//     /// ==================== 5. ✨ 10.全屏大字报：宣布回合胜负结果 ====================
//     // 【修复】：直接读取计时器，只要后台说需要展示胜负，就展示，不受阶段切换影响！
//     if (gameMgr->resultDisplayTimer > 0) {
//         painter.setBrush(QColor(0, 0, 0, 180)); // 半透明黑色遮罩背景
//         painter.drawRect(0, 0, width(), height());

//         QFont victoryFont("Microsoft YaHei", 36, QFont::Bold);
//         painter.setFont(victoryFont);

//         if (gameMgr->battleResultStr == "VICTORY") {
//             painter.setPen(QColor(50, 255, 50)); // 绿字胜利
//             painter.drawText(rect(), Qt::AlignCenter, "VICTORY\n回合胜利！");
//         } else if (gameMgr->battleResultStr == "DEFEAT") {
//             painter.setPen(QColor(255, 50, 50)); // 红字失败
//             painter.drawText(rect(), Qt::AlignCenter, "DEFEAT\n回合失败！");
//         } else if (gameMgr->battleResultStr == "DRAW") {
//             painter.setPen(Qt::white); // 白字平局
//             painter.drawText(rect(), Qt::AlignCenter, "DRAW\n同归于尽！");
//         }
//     }
// }
void MainWindow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 开启抗锯齿，让英雄圆圈和线条边缘更平滑

    // ==================== 1. 绘制棋盘网格 (8x8 战场) ====================
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            // 【棋盘色彩分块】：j < 4 是上半场（敌方偏红），j >= 4 是下半场（我方偏蓝）
            painter.setBrush(j < 4 ? QColor(255, 230, 230) : QColor(230, 230, 255));
            painter.setPen(QColor(200, 200, 200)); // 浅灰色网格线
            painter.drawRect(OFFSET_X + i*CELL_SIZE, OFFSET_Y + j*CELL_SIZE, CELL_SIZE, CELL_SIZE);

            // 检查当前格子（i, j）上有没有存活的单位（排除当前正被鼠标提起来拖拽的英雄）
            Unit* u = gameMgr->getUnitOnBoard(i, j);
            if(u && u != selectedUnit) {
                // 【阵营染色】：我方英雄染蓝色，敌方怪物染红色
                painter.setBrush(u->owner == Owner::PlayerCtrl ? QColor(50, 120, 240) : QColor(240, 60, 60));
                painter.setPen(Qt::NoPen); // 英雄主体不画边框

                // 计算当前英雄在窗口中的绝对像素左上角坐标（px, py）
                int px = OFFSET_X + i * CELL_SIZE;
                int py = OFFSET_Y + j * CELL_SIZE;
                // 在网格居中画一个 50x50 的圆形代表英雄
                painter.drawEllipse(px + 5, py + 5, 50, 50);

                // --- 绘制英雄星级 (仅我方显示) ---
                if (u->owner == Owner::PlayerCtrl) {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(255, 215, 0)); // 金黄色
                    // 循环星级数量，在英雄头顶排队画出金色小方块代表星星
                    for (int k = 0; k < u->star; k++) {
                        int starSize = 8;
                        int startX = px + (50 - u->star * 10) / 2; // 根据星级数量自动居中排列
                        painter.drawRect(startX + k * 12, py - 5, starSize, starSize);
                    }
                }

                // --- 绘制大本营/技能施法特效 ---
                if (u->state == UnitState::Casting) {
                    painter.setPen(QPen(QColor(255, 215, 0), 3)); // 3像素宽的金色耀眼外圈
                    painter.setBrush(Qt::NoBrush);
                    painter.drawEllipse(px + 3, py + 3, 54, 54);
                }

                // --- 动态血条渲染 (绿条) ---
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::black); // 黑底座
                painter.drawRect(px + 5, py + 2, 50, 4);
                painter.setBrush(Qt::green); // 绿血条
                float hpRatio = std::max(0.0f, (float)u->hp / u->maxHp); // 算血量百分比
                painter.drawRect(px + 5, py + 2, (int)(50 * hpRatio), 4);

                // --- 动态蓝条渲染 (蓝条，仅在有最大法力值时展示) ---
                if(u->maxMana > 0) {
                    painter.setBrush(Qt::black); // 黑底座
                    painter.drawRect(px + 5, py + 7, 50, 4);
                    painter.setBrush(Qt::cyan); // 青蓝色蓝条
                    float manaRatio = std::max(0.0f, (float)u->mana / u->maxMana); // 算蓝量百分比
                    painter.drawRect(px + 5, py + 7, (int)(50 * manaRatio), 4);
                }

                // --- ✨【新增】：在棋盘英雄的脚下（右下角）渲染微型穿戴装备小方块 ---
                for (size_t k = 0; k < u->equippedItems.size(); k++) {
                    Item* item = u->equippedItems[k];
                    QRect itemRect(px + 6 + k * 15, py + 42, 12, 12); // 每个装备格 12x12 像素

                    // 匹配装备对应的代表颜色
                    if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));      // 铁剑-猩红
                    else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));  // 锁子甲-钢蓝
                    else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));   // 手套-海绿
                    else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255)); // 蓝水晶-闪耀蓝

                    painter.setPen(QPen(Qt::white, 1)); // 白色细边框提升辨识度
                    painter.drawRect(itemRect);
                }
            }
        }
    }

    // ==================== 2. 绘制备战区格子 (下方横排 8 格) ====================
    for(int i = 0; i < 8; i++) {
        painter.setBrush(QColor(220, 220, 220)); // 浅灰色备战格背景
        painter.setPen(QColor(160, 160, 160));
        painter.drawRect(OFFSET_X + i*CELL_SIZE, BENCH_Y, CELL_SIZE, CELL_SIZE);

        // 检查备战席第 i 个位置有没有英雄
        Unit* u = gameMgr->getUnitOnBench(i);
        if(u && u != selectedUnit) {
            int px = OFFSET_X + i * CELL_SIZE;
            painter.setBrush(QColor(50, 120, 240)); // 我方统一蓝色圆圈
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(px + 5, BENCH_Y + 5, 50, 50);

            // --- ✨【新增】：备战区的英雄在穿上装备后，圆圈内同样绘制神装微型格 ---
            for (size_t k = 0; k < u->equippedItems.size(); k++) {
                Item* item = u->equippedItems[k];
                QRect itemRect(px + 6 + k * 15, BENCH_Y + 42, 12, 12);

                if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
                else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
                else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
                else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));

                painter.setPen(QPen(Qt::white, 1));
                painter.drawRect(itemRect);
            }
        }
    }

    // ==================== 3. 绘制拖拽中的动态阴影 (英雄 / 装备) ====================
    // 状态 A：如果当前手里抓着一只英雄，在鼠标坐标(dragPos)处画一个半透明发光黄圈跟随
    if(selectedUnit) {
        painter.setBrush(QColor(255, 255, 0, 150)); // 最后的 150 代表 Alpha 透明度
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(dragPos.x() - 25, dragPos.y() - 25, 50, 50); // 以当前鼠标为圆心
    }

    // 状态 B：✨【新增】：如果手里抓着一件装备，在鼠标坐标(dragPos)画一个高辨识度的小方块跟随
    if(isDraggingItem && selectedItemIndex != -1 && selectedItemIndex < (int)gameMgr->itemBench.size()) {
        Item* draggingItem = gameMgr->itemBench[selectedItemIndex];
        QRect dragRect(dragPos.x() - ITEM_SIZE/2, dragPos.y() - ITEM_SIZE/2, ITEM_SIZE, ITEM_SIZE); // 居中鼠标

        if (draggingItem->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
        else if (draggingItem->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
        else if (draggingItem->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
        else if (draggingItem->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));

        painter.setPen(QPen(Qt::white, 2)); // 耀眼的纯白拖拽边框
        painter.drawRect(dragRect);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
        painter.drawText(dragRect, Qt::AlignCenter, QString::fromStdString(draggingItem->name)); // 把装备字样印在正中央
    }

    // ==================== 4. 绘制右侧综合面板与全局资产 (金币/血量/人口) ====================
    painter.setPen(Qt::black);
    QFont infoFont = painter.font();
    infoFont.setPointSize(11);
    infoFont.setBold(true);
    painter.setFont(infoFont);

    // 打印当前的关卡轮数
    painter.drawText(PANEL_X, OFFSET_Y, "当前关卡: 第 " + QString::number(gameMgr->getCurrentRound()) + " 轮");

    // 转换当前游戏阶段的状态机文案
    QString stateStr;
    if (gameMgr->getState() == GameState::Preparation) {
        stateStr = "【准备阶段】\n操作：拖拽排兵布阵\n快捷键：[空格]开战";
    } else if (gameMgr->getState() == GameState::Battle) {
        stateStr = "【战斗进行中...】\n英雄正在自动寻路与施法";
    } else {
        stateStr = "【回合结算中】";
    }

    painter.setPen(QColor(100, 50, 150)); // 紫色加粗展示阶段状态
    painter.drawText(QRect(PANEL_X, OFFSET_Y + 25, 180, 60), Qt::AlignLeft, stateStr);

    // 渲染大本营实时玩家核心数值
    painter.setPen(Qt::black);
    painter.drawText(PANEL_X, OFFSET_Y + 100, "--------------------");
    painter.setPen(QColor(220, 40, 40)); // 红色写血量
    painter.drawText(PANEL_X, OFFSET_Y + 120, "玩家血量: " + QString::number(gameMgr->playerHp) + " / 100");
    painter.setPen(QColor(210, 160, 10)); // 金黄色写金币
    painter.drawText(PANEL_X, OFFSET_Y + 145, "拥有金币: " + QString::number(gameMgr->getPlayerGold()) + " G");
    painter.setPen(Qt::black);
    painter.drawText(PANEL_X, OFFSET_Y + 165, "当前人口: " + QString::number(gameMgr->getpoplulation()) + " 级");
    painter.drawText(PANEL_X, OFFSET_Y + 185, "--------------------");

    // ==================== 5. 绘制点击选中的详细属性面板 ====================
    if (focusedUnit) {
        // 【调优】：面板框高拉长到 240 像素，确保底部塞得下新加的装备信息
        painter.setBrush(QColor(245, 245, 245));
        painter.setPen(QColor(180, 180, 180));
        painter.drawRect(PANEL_X, OFFSET_Y + 180, 160, 240);

        painter.setPen(Qt::black);
        painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        painter.drawText(PANEL_X + 10, OFFSET_Y + 210, "【 " + QString::fromStdString(focusedUnit->name) + " 的面板 】");

        painter.setFont(QFont("Microsoft YaHei", 9, QFont::Normal));
        painter.drawText(PANEL_X + 10, OFFSET_Y + 235, "阵营: " + QString(focusedUnit->owner == Owner::PlayerCtrl ? "我方英雄" : "敌方怪物"));

        // ✨【优化点3核心实现】：循环累加计算该单位当前所有神装的额外属性加成综合
        int bonusAtkSum = 0;
        int bonusHpSum = 0;
        for (auto* item : focusedUnit->equippedItems) {
            bonusAtkSum += item->bonusAtk;
            bonusHpSum += item->bonusHp;
        }

        // 拆解渲染生命值数值。如果加成大于0，拼上 "(+150)" 后缀
        QString hpString = "生命: " + QString::number(focusedUnit->hp) + "/" + QString::number(focusedUnit->maxHp);
        if (bonusHpSum > 0) hpString += " (+" + QString::number(bonusHpSum) + ")";
        painter.drawText(PANEL_X + 10, OFFSET_Y + 260, hpString);

        // 拆解渲染攻击力面板。如果加成大于0，拼上 "(+15)" 后缀
        QString atkString = "攻击: " + QString::number(focusedUnit->atk);
        if (bonusAtkSum > 0) atkString += " (+" + QString::number(bonusAtkSum) + ")";
        painter.drawText(PANEL_X + 10, OFFSET_Y + 285, atkString);

        painter.drawText(PANEL_X + 10, OFFSET_Y + 310, "射程: " + QString::number(focusedUnit->range));

        QString t = "无";
        if(!focusedUnit->traits.empty()) t = QString::fromStdString(focusedUnit->traits[0]);
        painter.drawText(PANEL_X + 10, OFFSET_Y + 335, "核心羁绊: " + t);

        // ✨【优化点3续】：直观在属性框最底部显示穿戴的装备中文字样
        QString gearStr = "已装: ";
        if (focusedUnit->equippedItems.empty()) gearStr += "暂无装备";
        else {
            for (auto* item : focusedUnit->equippedItems) gearStr += "[" + QString::fromStdString(item->name) + "] ";
        }
        painter.setPen(QColor(139, 69, 19)); // 棕色复古字，突出神装效果
        painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
        painter.drawText(PANEL_X + 10, OFFSET_Y + 365, gearStr);
    }

    // ==================== 6. 绘制五联抽商店卡片 ====================
    for (int i = 0; i < 5; i++) {
        int cardX = OFFSET_X + i * (SHOP_CARD_W + SHOP_GAP); // 动态等距算横向 X 像素点
        QRect cardRect(cardX, SHOP_Y, SHOP_CARD_W, SHOP_CARD_H);
        Unit* shopHero = gameMgr->getShopSlot(i);

        if (shopHero != nullptr) {
            // 根据商品名字给格子染高级感浅色背景
            if (shopHero->name == "Garen") painter.setBrush(QColor(230, 245, 230));     // 浅绿
            else if (shopHero->name == "Ryze") painter.setBrush(QColor(230, 230, 250)); // 浅紫
            else painter.setBrush(QColor(255, 250, 230));                             // 浅黄
            painter.setPen(QPen(QColor(140, 140, 140), 1));
            painter.drawRect(cardRect);

            // 绘制卡片内英雄字样
            painter.setPen(Qt::black);
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            painter.drawText(cardRect.adjusted(10, 8, 0, 0), Qt::AlignLeft, QString::fromStdString(shopHero->name));

            // 金黄色字体写下价格
            painter.setPen(QColor(210, 150, 10));
            painter.setFont(QFont("Microsoft YaHei", 8, QFont::Normal));
            painter.drawText(cardRect.adjusted(10, 32, -10, -5), Qt::AlignLeft | Qt::AlignVCenter, QString::number(shopHero->cost) + " 金币");
        } else {
            // 已售罄：画灰色虚线框框防占位
            painter.setBrush(QColor(240, 240, 240));
            QPen dashPen(QColor(180, 180, 180), 1, Qt::DashLine);
            painter.setPen(dashPen);
            painter.drawRect(cardRect);
            painter.setPen(QColor(160, 160, 160));
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Normal));
            painter.drawText(cardRect, Qt::AlignCenter, "已售罄");
        }
    }

    // ==================== 7. 绘制“刷新商店”与“购买经验”功能按钮 ====================
    // --- 刷新按钮 ---
    if (gameMgr->getState() == GameState::Preparation) {
        painter.setBrush(QColor(255, 215, 0)); // 准备阶段亮黄可用
        painter.setPen(QPen(QColor(200, 150, 0), 1));
    } else {
        painter.setBrush(QColor(200, 200, 200)); // 战斗中变灰禁用
        painter.setPen(QPen(QColor(160, 160, 160), 1));
    }
    painter.drawRect(REFRESH_BTN_RECT);
    painter.setPen(gameMgr->getState() == GameState::Preparation ? Qt::black : QColor(120, 120, 120));
    painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    painter.drawText(REFRESH_BTN_RECT, Qt::AlignCenter, "刷新商店 (2G)");

    // --- 升级按钮 ---
    if (gameMgr->getState() == GameState::Preparation) {
        painter.setBrush(QColor(147, 112, 219)); // 绚丽紫色代表法力/经验
        painter.setPen(QPen(QColor(100, 50, 150), 1));
    } else {
        painter.setBrush(QColor(200, 200, 200)); // 战斗中禁用
        painter.setPen(QPen(QColor(160, 160, 160), 1));
    }
    painter.drawRect(BUY_XP_BTN_RECT);
    painter.setPen(gameMgr->getState() == GameState::Preparation ? Qt::white : QColor(120, 120, 120));
    painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    painter.drawText(BUY_XP_BTN_RECT, Qt::AlignCenter, "购买经验 (4G)");

    // ==================== 8. 绘制左边栏实时羁绊看板 ====================
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
            // 条件硬编码判定羁绊是否满足生效（重装 >= 1, 法师 >= 2）
            bool isActivated = false;
            if (trait == "Mage" && count >= 2) isActivated = true;
            if (trait == "Vanguard" && count >= 1) isActivated = true;

            if (isActivated) {
                painter.setBrush(QColor(255, 140, 0)); // 橙色激活高亮
                painter.setPen(Qt::NoPen);
                painter.drawRect(20, traitY - 14, 110, 20);
                painter.setPen(Qt::white);
            } else {
                painter.setBrush(QColor(220, 220, 220)); // 灰色未激活
                painter.setPen(Qt::NoPen);
                painter.drawRect(20, traitY - 14, 110, 20);
                painter.setPen(QColor(100, 100, 100));
            }
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            QString text = QString::fromStdString(trait) + " : " + QString::number(count);
            painter.drawText(25, traitY, text);
            traitY += 25; // 下移，排成一列
        }
    }

    // ==================== 9. 绘制下方装备库存栏格子 ====================
    painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    painter.setPen(Qt::black);
    painter.drawText(20, ITEM_GRID_Y - 10, "【 装备库存 】");

    for (int i = 0; i < gameMgr->MAX_ITEM_BENCH; i++) {
        int x = 20 + i * (ITEM_SIZE + 10); // 横向每个小方块间距 10 像素
        QRect rect(x, ITEM_GRID_Y, ITEM_SIZE, ITEM_SIZE);

        painter.setBrush(QColor(240, 240, 240));
        painter.setPen(QPen(QColor(180, 180, 180), 1, Qt::DashLine)); // 虚线画出装备槽格子
        painter.drawRect(rect);

        // 如果这个格子里确实有爆出来的空闲装备，渲染核心块
        if (i < (int)gameMgr->itemBench.size()) {
            Item* item = gameMgr->itemBench[i];
            if (item->type == ItemType::Sword) painter.setBrush(QColor(220, 20, 60));
            else if (item->type == ItemType::Armor) painter.setBrush(QColor(70, 130, 180));
            else if (item->type == ItemType::Glove) painter.setBrush(QColor(46, 139, 87));
            else if (item->type == ItemType::Crystal) painter.setBrush(QColor(30, 144, 255));

            painter.setPen(Qt::NoPen);
            painter.drawRect(rect.adjusted(3, 3, -3, -3)); // 缩进3像素让轮廓有悬浮质感

            painter.setPen(Qt::white);
            painter.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
            painter.drawText(rect, Qt::AlignCenter, QString::fromStdString(item->name)); // 把它的名字画正中心
        }
    }

    // ==================== ✨【优化点2核心实现】：全动态装备属性悬停提示面板 ====================
    Item* inspectItem = nullptr;
    QPoint localMouse = this->mapFromGlobal(QCursor::pos()); // 关键：抓取系统的实时鼠标像素位置，并映射转换到当前窗口

    // 状态检测分支 1：如果鼠标正抓着一件装备走，直接锁定这件被拖拽的装备
    if (isDraggingItem && selectedItemIndex != -1 && selectedItemIndex < (int)gameMgr->itemBench.size()) {
        inspectItem = gameMgr->itemBench[selectedItemIndex];
    }
    // 状态检测分支 2：手里没拖，扫描鼠标此时此刻是不是单纯“悬停”在下面的某个库存槽上方
    else {
        for (int i = 0; i < (int)gameMgr->itemBench.size(); i++) {
            int x = 20 + i * (ITEM_SIZE + 10);
            QRect rect(x, ITEM_GRID_Y, ITEM_SIZE, ITEM_SIZE);
            if (rect.contains(localMouse)) {
                inspectItem = gameMgr->itemBench[i]; // 完美捕获悬停格
                break;
            }
        }
    }

    // 如果满足任意一种查看态，在右下角（属性面板的正下方）渲染羊皮纸金边提示框
    if (inspectItem) {
        int tooltipY = OFFSET_Y + 435; // 紧贴着选定单位属性面板框的下方
        painter.setBrush(QColor(255, 255, 245)); // 优雅的羊皮纸奶黄底色
        painter.setPen(QPen(QColor(218, 165, 32), 2)); // 耀眼的暗金框线
        painter.drawRect(PANEL_X, tooltipY, 160, 105); // 160x105大小的装备看板

        // 打印装备大名
        painter.setPen(Qt::black);
        painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        painter.drawText(PANEL_X + 10, tooltipY + 20, "【 " + QString::fromStdString(inspectItem->name) + " 】");

        // 亮绿色加粗字，打印装备的具体核心增益属性
        painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        painter.setPen(QColor(34, 139, 34)); // 森林绿
        QString bonusText;
        if (inspectItem->type == ItemType::Sword) bonusText = "攻击力 +15";
        else if (inspectItem->type == ItemType::Armor) bonusText = "生命值 +150";
        else if (inspectItem->type == ItemType::Glove) bonusText = "攻击速度 +20%";
        else if (inspectItem->type == ItemType::Crystal) bonusText = "最大法力值 -30";
        painter.drawText(PANEL_X + 10, tooltipY + 45, bonusText);

        // 深灰色小字，打印自走棋装备风味小故事描述
        painter.setPen(QColor(110, 110, 110));
        painter.setFont(QFont("Microsoft YaHei", 8, QFont::Normal));
        QRect descRect(PANEL_X + 10, tooltipY + 58, 140, 42); // 限制自动换行的排版矩形范围
        QString descText;
        if (inspectItem->type == ItemType::Sword) descText = "锋利的生铁短剑，提供最纯粹的物理破坏力。";
        else if (inspectItem->type == ItemType::Armor) descText = "锁扣极其紧密的半身甲，大幅强化前排肉度。";
        else if (inspectItem->type == ItemType::Glove) descText = "轻便耐磨的纤维织物，使英雄出招迅捷如电。";
        else if (inspectItem->type == ItemType::Crystal) descText = "封印着奥术核心，能缩短技能释放所需的法力值。";
        painter.drawText(descRect, Qt::TextWordWrap, descText); // 开启自动折行
    }

    // ==================== 10. 全屏半透明大字报：宣布回合胜负结果 ====================
    if (gameMgr->resultDisplayTimer > 0) {
        painter.setBrush(QColor(0, 0, 0, 180)); // 180透明度的黑天鹅绒全屏遮罩
        painter.drawRect(0, 0, width(), height());
        QFont victoryFont("Microsoft YaHei", 36, QFont::Bold);
        painter.setFont(victoryFont);

        if (gameMgr->battleResultStr == "VICTORY") {
            painter.setPen(QColor(50, 255, 50)); // 亮绿大字报
            painter.drawText(rect(), Qt::AlignCenter, "VICTORY\n回合胜利！");
        } else if (gameMgr->battleResultStr == "DEFEAT") {
            painter.setPen(QColor(255, 50, 50)); // 鲜红大字报
            painter.drawText(rect(), Qt::AlignCenter, "DEFEAT\n回合失败！");
        } else if (gameMgr->battleResultStr == "DRAW") {
            painter.setPen(Qt::white); // 纯白大字报
            painter.drawText(rect(), Qt::AlignCenter, "DRAW\n同归于尽！");
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    // 优先检测是否点中了装备库存格
    for (int i = 0; i < (int)gameMgr->itemBench.size(); i++) {
        int x = 20 + i * (ITEM_SIZE + 10);
        QRect rect(x, ITEM_GRID_Y, ITEM_SIZE, ITEM_SIZE);

        if (rect.contains(event->pos())) {
            selectedItemIndex = i;
            isDraggingItem = true;
            dragPos = event->pos(); // ✨【新增】：记录装备拖拽的起始像素位置

            update();
            return; // 成功抓取到装备，立刻返回，不触发抓英雄逻辑
        }
    }
    //点刷新
    if (REFRESH_BTN_RECT.contains(event->pos())) {
        if (gameMgr->getState() == GameState::Preparation) {
            gameMgr->refreshShopManual(); // 调用扣钱刷新
            update(); // 刷新界面
        }
        return; // 点了按钮就直接返回，不触发下面的抓取英雄逻辑
    }
    //点买经验
    if(BUY_XP_BTN_RECT.contains(event->pos())){
        if(gameMgr->getState()==GameState::Preparation){
            gameMgr->buyXP();
            update();
        }
    }
    // 2. ✨ 新增判定：是否点击了 5 联抽商店的某张卡片
    if (gameMgr->getState() == GameState::Preparation) { // 只有准备阶段允许买牌
        for (int i = 0; i < 5; i++) {
            // 重新计算第 i 张卡片的像素范围（与 paintEvent 里的计算完全一致）
            int cardX = OFFSET_X + i * (SHOP_CARD_W + SHOP_GAP);
            QRect cardRect(cardX, SHOP_Y, SHOP_CARD_W, SHOP_CARD_H);

            if (cardRect.contains(event->pos())) {
                // 点击了第 i 张卡片，尝试调用购买
                bool success = gameMgr->buyHeroFromShop(i);
                if (success) {
                    update(); // 购买成功，刷新界面（钱减少、商品变售罄、备战区长出圆圈）
                }
                return; // 点了商店就直接返回，不触发下面拖拽/选中的逻辑
            }
        }
    }
    if(gameMgr->getState()!=GameState::Preparation){return;}//若不是准备状态，禁止抓取
    auto [lx, ly] = getLogicalPos(event->pos());
    if(lx != -1) {
        // 尝试获取单位
        Unit* clickedUnit = (ly == -1) ? gameMgr->getUnitOnBench(lx) : gameMgr->getUnitOnBoard(lx, ly);

        if(clickedUnit) {
            // 🔴【核心修复】：如果单位是敌方阵营，或者已经阵亡(HP<=0/Dead状态)，直接拦截，拒绝抓取！
            if (clickedUnit->owner == Owner::EnemyCtrl || !clickedUnit->isAlive() || clickedUnit->state == UnitState::Dead) {
                selectedUnit = nullptr;
            } else {
                selectedUnit = clickedUnit;
                focusedUnit = selectedUnit;
                dragPos = event->pos();
                update();
            }
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if(selectedUnit) {
        dragPos = event->pos();
        update();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    // 如果松开鼠标时，手里正抓着一件装备
    if (isDraggingItem && selectedItemIndex != -1) {
        isDraggingItem = false;
        Item* draggingItem = gameMgr->itemBench[selectedItemIndex];

        Unit* targetHero = nullptr;

        // 1. 尝试转换为棋盘网格坐标
        // ✨【核心修复】：调用你的 std::pair 版本的 getLogicalPos
        std::pair<int, int> logicalPos = getLogicalPos(event->pos());
        int nx = logicalPos.first;
        int ny = logicalPos.second;

        // 根据你函数返回的特征，直接优雅地捕捉目标英雄
        if (nx >= 0 && nx < 8) {
            if (ny >= 0 && ny < 8) {
                // 情况 1：落点在棋盘内
                targetHero = gameMgr->getUnitOnBoard(nx, ny);
            } else if (ny == -1) {
                // 情况 2：落点在备战区（得益于你自带的 y = -1 逻辑！）
                targetHero = gameMgr->getUnitOnBench(nx);
            }
        }

        // 3. 如果落点处确实是我方英雄，执行穿戴
        if (targetHero && targetHero->owner == Owner::PlayerCtrl) {
            if (targetHero->equipItem(draggingItem)) {
                // 穿戴成功，从装备仓库剔除
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
        return; // 处理完毕，拦截事件
    }

    if(selectedUnit) {
        auto [nx, ny] = getLogicalPos(event->pos());
        if(nx != -1) {
            // 调用你之前写好的 GameManager 逻辑！
            gameMgr->MoveUnit(selectedUnit, nx, ny, (ny == -1));
        }
        selectedUnit = nullptr;
        update();
    }
}

void MainWindow::onGameTick(){
    gameMgr->updateTick();//驱动后台逻辑
    update();//强制界面重绘
}
void MainWindow::keyPressEvent(QKeyEvent*event){
    if(event->key()==Qt::Key_Space){
        //按键为空格，则开始战斗
        gameMgr->startBattle();
        update();
    }
}