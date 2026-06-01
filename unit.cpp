#include "unit.h"
#include "gamemanager.h"

#include <algorithm>
#include <queue>

Unit::Unit(int _hp, int _atk, int _range, int _maxMana, Owner _owner)
    : hp(_hp), maxHp(_hp), atk(_atk), range(_range),
      mana(0), maxMana(_maxMana), baseMaxHp(_hp), baseAtk(_atk), baseMaxMana(_maxMana),
      owner(_owner)
{
    // 单位刚创建时还没有真实位置，由 GameManager 放入棋盘或备战区。
    x = -1;
    y = -1;
    isBench = true;

    // 战斗状态机默认从空闲开始，等待每帧 updateAction 驱动。
    state = UnitState::Idle;
    target = nullptr;

    // 以 30ms 左右一帧估算：30 帧攻击一次，20 帧移动一格。
    attackInterval = 30;
    attackTimer = 0;
    moveInterval = 20;
    moveTimer = 0;

    baseAttackInterval = attackInterval;
}

Unit::~Unit() {
    for (Item* item : equippedItems) {
        delete item;
    }
    equippedItems.clear();
}

bool Unit::isAlive() const {
    // 生命值大于 0 才视为存活。
    return hp > 0;
}

void Unit::takeDamage(int damage) {
    // 扣除伤害，并保证生命值不会低于 0。
    hp = std::max(0, hp - damage);
}

void Unit::addMana(int amount) {
    // 增加法力，并保证不会超过最大法力值。
    mana = std::min(maxMana, mana + amount);
}

void Unit::updateAction(GameManager* gameMgr) {
    // 每一帧由 GameManager 调用，根据当前状态分发到具体行为。
    if (state == UnitState::Dead || !isAlive()) {
        state = UnitState::Dead;
        return;
    }

    if (mana >= maxMana && maxMana > 0) {
        state = UnitState::Casting;
    }

    switch (state) {
    case UnitState::Idle:
        handleIdle(gameMgr);
        break;
    case UnitState::Moving:
        handleMoving(gameMgr);
        break;
    case UnitState::Attacking:
        handleAttacking(gameMgr);
        break;
    case UnitState::Casting:
        handleCasting(gameMgr);
        break;
    case UnitState::Dead:
        break;
    }
}

void Unit::handleIdle(GameManager* gameMgr) {
    // 空闲状态负责索敌：找最近敌人，距离相同则按血量、坐标稳定决策。
    Unit* bestTarget = nullptr;
    int bestDistSq = 999999;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Unit* potential = gameMgr->getUnitOnBoard(i, j);
            if (!potential || potential->owner == owner || !potential->isAlive()) {
                continue;
            }

            int dx = potential->x - x;
            int dy = potential->y - y;
            int distSq = dx * dx + dy * dy;

            bool better = bestTarget == nullptr || distSq < bestDistSq;
            if (!better && distSq == bestDistSq) {
                if (potential->hp != bestTarget->hp) {
                    better = potential->hp > bestTarget->hp;
                } else if (potential->x != bestTarget->x) {
                    better = potential->x < bestTarget->x;
                } else {
                    better = potential->y > bestTarget->y;
                }
            }

            if (better) {
                bestTarget = potential;
                bestDistSq = distSq;
            }
        }
    }

    if (!bestTarget) {
        return;
    }

    target = bestTarget;
    if (bestDistSq <= range * range) {
        state = UnitState::Attacking;
        attackTimer = 0;
    } else {
        state = UnitState::Moving;
        moveTimer = 0;
    }
}

void Unit::handleMoving(GameManager* gameMgr) {
    // 移动状态用 BFS 找到通向目标附近的下一步，避免被简单贪心卡住。
    if (!target || !target->isAlive()) {
        target = nullptr;
        state = UnitState::Idle;
        return;
    }

    int dx = target->x - x;
    int dy = target->y - y;
    int distSq = dx * dx + dy * dy;
    if (distSq <= range * range) {
        state = UnitState::Attacking;
        attackTimer = 0;
        return;
    }

    if (moveTimer > 0) {
        --moveTimer;
        return;
    }

    bool visited[8][8] = {};
    int parentX[8][8];
    int parentY[8][8];
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            parentX[i][j] = -1;
            parentY[i][j] = -1;
        }
    }

    std::queue<std::pair<int, int>> q;
    q.push({x, y});
    visited[x][y] = true;

    int bestX = x;
    int bestY = y;
    int bestScore = distSq;
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        int tx = target->x - cx;
        int ty = target->y - cy;
        int score = tx * tx + ty * ty;
        if (score < bestScore) {
            bestScore = score;
            bestX = cx;
            bestY = cy;
        }

        for (const auto& dir : dirs) {
            int nx = cx + dir[0];
            int ny = cy + dir[1];
            if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8 || visited[nx][ny]) {
                continue;
            }

            Unit* blocker = gameMgr->getUnitOnBoard(nx, ny);
            if (blocker != nullptr && blocker != this) {
                continue;
            }

            visited[nx][ny] = true;
            parentX[nx][ny] = cx;
            parentY[nx][ny] = cy;
            q.push({nx, ny});
        }
    }

    int nextX = x;
    int nextY = y;
    if (bestX != x || bestY != y) {
        int stepX = bestX;
        int stepY = bestY;
        while (parentX[stepX][stepY] != x || parentY[stepX][stepY] != y) {
            int px = parentX[stepX][stepY];
            int py = parentY[stepX][stepY];
            if (px == -1 || py == -1) {
                stepX = x;
                stepY = y;
                break;
            }
            stepX = px;
            stepY = py;
        }
        nextX = stepX;
        nextY = stepY;
    }

    if (nextX != x || nextY != y) {
        gameMgr->updateUnitPosition(x, y, nextX, nextY);
        x = nextX;
        y = nextY;
    }

    moveTimer = moveInterval;
}

void Unit::handleAttacking(GameManager* gameMgr) {
    if (!target || !target->isAlive()) {
        target = nullptr;
        state = UnitState::Idle;
        return;
    }

    int dx = target->x - x;
    int dy = target->y - y;
    if (dx * dx + dy * dy > range * range) {
        state = UnitState::Moving;
        return;
    }

    if (stunFrames > 0) {
        --stunFrames;
        attackTimer = attackInterval;
        return;
    }

    if (attackTimer > 0) {
        --attackTimer;
        return;
    }

    // 计算基础伤害，逐级穿过攻击者与受击者的高级装备被动钩子
    int attackDamage = atk;

    // 攻击者 onAttack（无尽之刃暴击等）
    for (Item* item : equippedItems) {
        AdvancedItem* adv = dynamic_cast<AdvancedItem*>(item);
        if (adv) adv->onAttack(this, target, attackDamage, gameMgr);
    }

    // 受击者 onDamaged（荆棘之甲反弹等）
    for (Item* item : target->equippedItems) {
        AdvancedItem* adv = dynamic_cast<AdvancedItem*>(item);
        if (adv) adv->onDamaged(target, this, attackDamage, gameMgr);
    }

    target->takeDamage(attackDamage);
    addMana(10);
    attackTimer = attackInterval;

    if (!target->isAlive()) {
        target = nullptr;
        state = UnitState::Idle;
        return;
    }

    if (range > 1 && gameMgr) {
        gameMgr->addProjectile(x, y, target->x, target->y, false);
    }
}

void Unit::handleCasting(GameManager* gameMgr) {
    castSkill(gameMgr);
    mana = 0;

    // 技能释放后触发高级装备被动（卢登的回声 AOE 等）
    for (Item* item : equippedItems) {
        AdvancedItem* adv = dynamic_cast<AdvancedItem*>(item);
        if (adv) adv->onSkillCast(this, target, gameMgr);
    }

    if (range > 1 && gameMgr && target && target->isAlive()) {
        gameMgr->addProjectile(x, y, target->x, target->y, true);
    }

    state = UnitState::Idle;
}

void Unit::castSkill(GameManager* gameMgr) {
    // 基类默认技能：对当前目标造成一次双倍攻击伤害。
    (void)gameMgr;
    if (target && target->isAlive()) {
        target->takeDamage(atk * 2);
    }
}
