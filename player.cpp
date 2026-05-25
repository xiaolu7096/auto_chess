#include "player.h"

#include <algorithm>

Player::Player() {
    // 玩家资源初始化：100 血、10 金币、3 人口起步。
    hp = 100;
    gold = 10;
    level = 3;
    exp = 0;
    expToNextLevel = 4;
}

Player::~Player() {
}

void Player::addGold(int amount) {
    // 战斗奖励、失败补偿等都会通过这里增加金币。
    gold += amount;
}

bool Player::spendGold(int amount) {
    // 统一消费入口：金币足够才扣除，避免商店和升级各写一套判断。
    if (gold >= amount) {
        gold -= amount;
        return true;
    }
    return false;
}

void Player::takeDamage(int damage) {
    // 扣除基地血量，并保证不会低于 0。
    hp = std::max(0, hp - damage);
}

void Player::addXP(int amount) {
    // 增加经验后立刻检查是否升级。
    exp += amount;
    checkLevelUp();
}

int Player::getPopulationCap() const {
    // 当前版本采用“等级 = 人口上限”的简单规则。
    return level;
}

void Player::restoreState(int newHp, int newGold, int newLevel, int newExp, int newExpToNextLevel) {
    // 读档时直接恢复玩家资源，避免通过 add/spend 触发额外逻辑。
    hp = newHp;
    gold = newGold;
    level = newLevel;
    exp = newExp;
    expToNextLevel = newExpToNextLevel;
}

void Player::checkLevelUp() {
    // 支持一次获得大量经验时连续升级。
    while (exp >= expToNextLevel) {
        exp -= expToNextLevel;
        level++;
    }
}
