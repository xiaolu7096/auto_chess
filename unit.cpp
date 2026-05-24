#include "unit.h"
#include"gamemanager.h"
#include <algorithm> // 用于使用 std::max 和 std::min

// 构造函数的实现
Unit::Unit(int _hp, int _atk, int _range, int _maxMana, Owner _owner)
    : hp(_hp), maxHp(_hp), atk(_atk), range(_range),
    mana(0), maxMana(_maxMana), owner(_owner)
{
    // 默认初始位置设为非法值，等待 GameManager 分配
    x = -1;
    y = -1;
    isBench = true; // 默认先放在备战区
    // ✨ 初始化状态机和默认计时器数值
    state = UnitState::Idle;
    target = nullptr;

    attackInterval = 30; // 默认 30 帧（1秒）普攻一次
    attackTimer = 0;

    moveInterval = 20;   // 默认 20 帧（约0.6秒）移动一格
    moveTimer = 0;
}

Unit::~Unit() {
    // 阶段一暂不需要复杂的清理工作
}

// --- TODO: 请你完成以下函数的具体逻辑 ---

bool Unit::isAlive() const {
    // TODO: 返回 hp 是否大于 0
    if(hp>0){
        return true;
    }else{
        return false;
    }
}

void Unit::takeDamage(int damage) {
    // TODO: 减去伤害值
    // 提示：hp = std::max(0, hp - damage);
    hp=std::max(0,hp-damage);
}

void Unit::addMana(int amount) {
    // TODO: 增加法力值，但不能超过 maxMana
    // 提示：mana = std::min(maxMana, mana + amount);
    mana=std::min(maxMana,mana+amount);
}
// --- 每帧的核心分发机制 ---
void Unit::updateAction(GameManager* gameMgr) {
    // 如果已经死亡，不执行任何 AI 逻辑
    if (state == UnitState::Dead || !isAlive()) {
        state = UnitState::Dead;
        return;
    }

    // 每一帧，技能蓝量判定拥有最高优先级
    if (mana >= maxMana && maxMana > 0) {
        state = UnitState::Casting;
    }

    // 状态机行为分发
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
    // TODO 任务 A: 寻找最近的敌人。
    // 提示：现在先留空，下一关我们将在此调用索敌算法。
    // 如果找到了目标，且目标在攻击范围内 -> 切换到 Attacking 状态
    // 如果找到了目标，但目标太远够不着     -> 切换到 Moving 状态
    Unit* bestTarget = nullptr;
    int bestDistSq = 999999; // 先设一个很大的初始距离平方值

    // 1. 遍历 8x8 棋盘寻找所有潜在敌人
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Unit* potential = gameMgr->getUnitOnBoard(i,j);

            // 必须是对面阵营，且必须活着
            if (potential && potential->owner != this->owner && potential->isAlive()) {

                // 计算距离平方: dx^2 + dy^2
                int dx = potential->x - this->x;
                int dy = potential->y - this->y;
                int distSq = dx * dx + dy * dy;

                if (bestTarget == nullptr) {
                    // 第一个找到的敌人，先无条件作为临时最优目标
                    bestTarget = potential;
                    bestDistSq = distSq;
                }
                else if (distSq < bestDistSq) {
                    // 情况一：发现距离更近的敌人，直接换人
                    bestTarget = potential;
                    bestDistSq = distSq;
                }
                else if (distSq == bestDistSq) {
                    // 情况二：距离居然一样近！触发规则决胜

                    // 👑 【TODO 任务 E】: 实现决胜判定
                    // 规则 1：优先生命值高低 (potential->hp > bestTarget->hp)
                    // 规则 2：若血量也相同，从左向右 (potential->x < bestTarget->x)
                    // 规则 3：若 X 也相同，从下到上 (potential->y > bestTarget->y)
                    //
                    // 请在下方完善 if 判定，决定是否要将 bestTarget 替换为 potential：
                    if (potential->hp != bestTarget->hp) {
                        if (potential->hp > bestTarget->hp) {
                            bestTarget = potential; // 优先生命值高的
                        }
                    } else if (potential->x != bestTarget->x) {
                        if (potential->x < bestTarget->x) {
                            bestTarget = potential;
                        }
                    } else if (potential->y > bestTarget->y) {
                        bestTarget = potential;
                    }
                }
            }
        }
    }

    // 2. 检查最终索敌结果，决定下一阶段去干嘛
    if (bestTarget != nullptr) {
        this->target = bestTarget;

        // 判断目标是否在我的射程之内
        // 提示：我们的攻击距离 range 也是逻辑格子数。如果距离平方 <= 射程的平方，说明够得着！
        if (bestDistSq <= this->range * this->range) {
            this->state = UnitState::Attacking;
            this->attackTimer = 0; // 锁定目标后可以立刻尝试出手
        } else {
            this->state = UnitState::Moving;
            this->moveTimer = 0;   // 够不着，准备开始迈腿跑
        }
    }
}


void Unit::handleMoving(GameManager* gameMgr) {
    // 1. 目标失效检查
    if (!target || !target->isAlive()) {
        target = nullptr;
        state = UnitState::Idle;
        return;
    }

    // 2. 距离检查：如果敌人在途中自己走过来了，进入了我的射程，立刻转为攻击！
    int dx = target->x - this->x;
    int dy = target->y - this->y;
    int distSq = dx * dx + dy * dy;
    if (distSq <= this->range * this->range) {
        state = UnitState::Attacking;
        attackTimer = 0;
        return;
    }

    // 3. 移动时钟冷却
    if (moveTimer > 0) {
        --moveTimer;
        return;
    }

    // 执行到位移时，说明 moveTimer == 0，冷却好了！
    if (moveTimer == 0) {
        int nextX = this->x;
        int nextY = this->y;

        // 👑 【TODO 任务 F】: 贪心计算下一步想去的空格子
        // 提示：
        // 1. 先看 dx。如果 dx != 0，计算出临时的 stepX = this->x + (dx > 0 ? 1 : -1);
        //    调用 gameMgr->getUnitOnBoard(stepX, this->y)，如果返回 nullptr，说明没被阻挡！
        //    此时令 nextX = stepX;
        // 2. 如果横向被挡住了（或者 dx==0），再看 dy。
        //    如果 dy != 0，计算出临时的 stepY = this->y + (dy > 0 ? 1 : -1);
        //    验证 (this->x, stepY) 是否为空格。如果是，令 nextY = stepY;

        // ---- 请在此处写下你的贪心选格逻辑 ----
        if(dx!=0){
            int stepX=this->x+(dx>0?1:-1);
            if(stepX>=0&&stepX<8&&!gameMgr->getUnitOnBoard(stepX,this->y)){
                nextX=stepX;
            }
        }
        if(nextX==this->x&&dy!=0){
            int stepY=this->y+(dy>0?1:-1);
            if(stepY>=0&&stepY<8&&gameMgr->getUnitOnBoard(this->x,stepY)==nullptr){
                nextY=stepY;
            }
        }
        // ------------------------------------

        // 4. 如果成功找到了未被阻挡的格子，通知 GameManager 挪动指针
        if (nextX != this->x || nextY != this->y) {
            // ⚠️ 假设你在 GameManager 里有类似的移动同步函数：
            gameMgr->updateUnitPosition(this->x, this->y, nextX, nextY);

            // 同时更新单位内部的坐标
            this->x = nextX;
            this->y = nextY;
        }

        // 5. 迈出一步后，无论成功移动还是被卡住，重置移动冷却计时器
        moveTimer = moveInterval;
    }
}
// 3. 普攻状态处理器
void Unit::handleAttacking(GameManager* gameMgr) {
    if (!target || !target->isAlive()) {
        target = nullptr;
        state = UnitState::Idle;
        return;
    }

    // TODO 任务 C: 实现普攻计时器的冷却计算与伤害触发
    // 1. 如果 attackTimer 大于 0，说明武器还在挥舞/拉弓后摇中，让 attackTimer 自减 1。
    // 2. 如果 attackTimer 等于 0，说明可以发动攻击：
    //    - 让目标受到伤害：target->takeDamage(this->atk);
    //    - 自身回复 10 点法力值：this->addMana(10);
    //    - 重置计时器，让它重新进入冷却：attackTimer = attackInterval;
    if(attackTimer>0){
        --attackTimer;
    }else if(attackTimer==0){
        state=UnitState::Attacking;
        target->takeDamage(this->atk);
        this->addMana(10);
        attackTimer=attackInterval;
    }
}

// 4. 大招状态处理器
void Unit::handleCasting(GameManager* gameMgr) {
    // 释放大招时自动清空蓝条
    castSkill(gameMgr);
    mana = 0;
    state=UnitState::Idle;
    // TODO 任务 D: 释放大招后的行为收尾
    // 大招释放是一瞬间的动作。大招结束后，角色应该变成什么状态？
    // 请在此行将 state 修改为正确的后续状态。
}

void Unit::castSkill(GameManager*gameMgr){//基类的默认大招实现（如果没有子类重写，就触发普通痛击）
    if (target && target->isAlive()) {
        target->takeDamage(this->atk * 2); // 默认：对当前目标造成双倍伤害
    }
}