#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#define BENCHSIZE 8
#define LENGTH 8
#define WIDTH 8

#include "player.h"
#include "unit.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <QString>

// 游戏的三个主阶段：准备、战斗、结算。
enum class GameState {
    Preparation,
    Battle,
    Settlement,
};

class GameManager {
public:
    GameManager();
    ~GameManager();

    // 查询棋盘/备战区单位，UI 绘制和战斗 AI 都会使用。
    Unit* getUnitOnBoard(int x, int y);
    Unit* getUnitOnBench(int index);

    // 战斗中同步单位坐标与棋盘指针。
    void updateUnitPosition(int oldX, int oldY, int newX, int newY);

    // 准备阶段拖拽单位：可移动到空格，也可与友方单位交换。
    bool MoveUnit(Unit* target, int nextX, int nextY, bool toBench);

    // 从棋盘或备战区移除单位。
    void RemoveUnit(Unit* TargetUnit);

    // 检查玩家是否还能继续上阵。
    bool Checkpopulation();

    // 按轮数生成敌人。
    void spawnEnemyRound(int round);

    GameState getState() { return currentState; }
    int getCurrentRound() { return currentround; }

    // 阶段流：开始战斗、每帧更新、胜负结算、死亡清理。
    void startBattle();
    void updateTick();
    void checkBattleResult();
    void cleanupDeadUnits();

    // 商店和经济操作。
    void refreshShop();
    void refreshShopManual();
    bool buyHeroFromShop(int shopIndex);
    void buyXP();
    int getpoplulation();
    Unit* getShopSlot(int index) { return shopSlots[index]; }
    int getPlayerGold() { return player->getGold(); }

    // 自动升星与羁绊统计。
    void checkAndCombineStars();
    void updateActiveTraits();

    // 存档/读档：把当前游戏状态写入 JSON 文件，再从文件完整恢复。
    bool saveGame(const QString& filePath);
    bool loadGame(const QString& filePath);

    // UI 直接读取的全局状态。
    int playerHp = 100;
    int enemyHp = 100;
    int resultDisplayTimer = 0;
    QString battleResultStr = "NONE";
    std::map<std::string, int> activeTraitsCount;

    // 玩家装备库存。
    std::vector<Item*> itemBench;
    const int MAX_ITEM_BENCH = 8;

private:
    // 存档辅助：单位/装备的序列化与反序列化。
    Unit* createUnitByName(const std::string& name, Owner owner);
    Item* createItemByName(const std::string& name);

    Unit* board[LENGTH][WIDTH];
    Unit* bench[BENCHSIZE];
    Player* player;
    GameState currentState;
    int currentround;
    Unit* shopSlots[5];
};

#endif // GAMEMANAGER_H
