#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#define BENCHSIZE 8

#include "board.h"
#include "player.h"
#include "unit.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <QPointF>
#include <QString>

enum class GameState {
    Preparation,
    Battle,
    Settlement,
};

struct Projectile {
    int fromX, fromY;
    int toX, toY;
    float progress = 0.0f;
    int totalFrames = 12;
    int currentFrame = 0;
    bool isSkill = false;
    bool showHit = false;
    int hitTimer = 0;
};

// 技能特效：位置 + 技能名 + 剩余帧数，由 mainwindow 用对应图片绘制。
struct SkillEffect {
    int x, y;
    std::string skillName;
    int timer;
};

class GameManager {
public:
    GameManager();
    ~GameManager();

    Unit* getUnitOnBoard(int x, int y);
    Unit* getUnitOnBench(int index);

    void updateUnitPosition(int oldX, int oldY, int newX, int newY);

    bool MoveUnit(Unit* target, int nextX, int nextY, bool toBench);
    bool canMoveUnit(Unit* target, int nextX, int nextY, bool toBench);

    void RemoveUnit(Unit* TargetUnit);

    bool Checkpopulation();

    void spawnEnemyRound(int round);

    GameState getState() { return currentState; }
    int getCurrentRound() { return currentround; }
    int getWinStreak() const { return winStreak; }
    int getLoseStreak() const { return loseStreak; }
    int getInterestGold() const { return interestGold; }

    void startBattle();
    void updateTick();
    void checkBattleResult();
    void cleanupDeadUnits();

    void refreshShop();
    void refreshShopManual();
    bool buyHeroFromShop(int shopIndex);
    void buyXP();
    int getpoplulation();
    Unit* getShopSlot(int index) { return shopSlots[index]; }
    int getPlayerGold() { return player->getGold(); }
    int getPlayerExp() { return player->getExp(); }
    int getPlayerExpToNextLevel() { return player->getExpToNextLevel(); }

    void checkAndCombineStars();
    void updateActiveTraits();

    bool saveGame(const QString& filePath);
    bool loadGame(const QString& filePath);

    void addProjectile(int fromX, int fromY, int toX, int toY, bool isSkill);
    void addSkillEffect(int x, int y, const std::string& skillName, int duration = 20);
    void updateProjectiles();

    // 高级装备钩子：在战斗各阶段遍历单位身上的高级装备并触发被动
    void triggerItemCallbacks_BattleStart();
    void triggerItemCallbacks_Tick();
    bool triggerItemCallbacks_Death(Unit* u);  // 返回 true 表示单位被复活

    int playerHp = 100;
    int enemyHp = 100;
    int resultDisplayTimer = 0;
    QString battleResultStr = "NONE";
    QString settlementBreakdown;  // 结算奖励明细
    std::map<std::string, int> activeTraitsCount;

    std::vector<Item*> itemBench;
    const int MAX_ITEM_BENCH = 8;

std::vector<Projectile> projectiles;
    std::vector<SkillEffect> skillEffects;

    Board board;

    int battleFrame = 0;  // 当前战斗帧计数，用于 onTick 周期钩子

    // 连胜 / 连败经济系统：每次结算时发放利息与额外奖励。
    int winStreak = 0;
    int loseStreak = 0;
    int interestGold = 0;  // 上轮生成的利息额，UI展示用

private:
    Unit* createUnitByName(const std::string& name, Owner owner);
    Item* createItemByName(const std::string& name);

    void registerUnit(Unit* u);
    void unregisterUnit(Unit* u);
    void clearAllUnits();

    Unit* bench[BENCHSIZE];
    Player* player;
    GameState currentState;
    int currentround;
    Unit* shopSlots[5];

    std::vector<Unit*> allUnits;
};

#endif // GAMEMANAGER_H
