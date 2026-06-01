#ifndef UNIT_H
#define UNIT_H

#include <string>
#include <vector>
#include <algorithm>

#include "Item.h"
#include "advanceditems.h"

// 单位归属：玩家单位和敌方单位共用同一个 Unit 体系。
enum class Owner {
    PlayerCtrl,
    EnemyCtrl
};

// 单位在战斗中的微观状态机。
enum class UnitState {
    Idle,       // 空闲，寻找目标
    Moving,     // 向目标移动
    Attacking,  // 普通攻击
    Casting,    // 释放技能
    Dead        // 死亡，等待 GameManager 清理
};

class GameManager;

class Unit {
public:
    // 初始化一个单位的基础战斗属性和阵营。
    Unit(int hp, int atk, int range, int maxMana, Owner owner);
    virtual ~Unit();

    // 基础属性：PA 文档要求的生命、攻击、射程、法力。
    int hp;
    int maxHp;
    int atk;
    int range;
    int mana;
    int maxMana;

    // 基础原始属性（装备加成后不含羁绊Buff的基准值，用于每回合还原）。
    int baseMaxHp;
    int baseAtk;
    int baseMaxMana;
    int baseAttackInterval;  // 基础攻击间隔（装备攻速加减需要还原基准值）

    // 阵营与羁绊：敌我单位通过 owner 区分，traits 用于职业/羁绊统计。
    Owner owner;
    std::vector<std::string> traits;

    // 逻辑位置：棋盘坐标为 0..7；备战区用 y=-1 表示。
    int x;
    int y;
    bool isBench;

    // 战斗状态：当前状态和锁定目标。
    UnitState state;
    Unit* target;

    // 行动计时器：用帧数控制普攻和移动频率。
    int attackInterval;
    int attackTimer;
    int moveInterval;
    int moveTimer;

    // 眩晕剩余帧数：>0 时 unit 无法普攻，每帧递减。
    int stunFrames = 0;

    // 英雄信息与升星属性。
    std::string name;
    int star = 1;
    int cost = 1;

    // 升星：提高星级，同时提升生命和攻击。
    void evolve() {
        star++;
        maxHp *= 1.8;
        hp = maxHp;
        atk *= 1.8;
    }

    // 装备栏：1 星最多 1 件，2 星及以上最多 2 件。
    std::vector<Item*> equippedItems;

    int getMaxItemSlots() const {
        return (star >= 2) ? 2 : 1;
    }

    // 穿戴装备：成功时应用属性加成；合成时自动检测两件基础装备 → 高级装备。
    bool equipItem(Item* item) {
        // 先检测是否与已装备的基础装备触发合成
        for (size_t i = 0; i < equippedItems.size(); i++) {
            AdvancedItem* result = AdvancedItem::synthesize(equippedItems[i], item);
            if (result) {
                // 撤回被替换的旧装备属性
                atk -= equippedItems[i]->bonusAtk;
                maxHp -= equippedItems[i]->bonusHp;
                delete equippedItems[i];
                equippedItems.erase(equippedItems.begin() + i);
                delete item;  // 消耗进来的基础装备

                // 穿戴合成后的高级装备（递归，不会再触发合成）
                equippedItems.push_back(result);
                atk += result->bonusAtk;
                maxHp += result->bonusHp;
                hp += result->bonusHp;
                if (maxMana > result->manaReduction)
                    maxMana -= result->manaReduction;
                else
                    maxMana = 10;
                // 攻速还原基准值后重新计算
                attackInterval = baseAttackInterval;
                for (Item* eq : equippedItems) {
                    if (eq->bonusSpeed > 0.0)
                        attackInterval = std::max(1, (int)(attackInterval / (1.0 + eq->bonusSpeed)));
                }
                return true;
            }
        }

        // 未触发合成，检查槽位限制
        if ((int)equippedItems.size() >= getMaxItemSlots()) {
            return false;
        }

        equippedItems.push_back(item);
        atk += item->bonusAtk;
        maxHp += item->bonusHp;
        hp += item->bonusHp;

        if (maxMana > item->manaReduction) {
            maxMana -= item->manaReduction;
        } else {
            maxMana = 10;
        }

        if (item->bonusSpeed > 0.0) {
            attackInterval = std::max(1, (int)(attackInterval / (1.0 + item->bonusSpeed)));
        }

        return true;
    }

    // 基础战斗逻辑。
    bool isAlive() const;
    void takeDamage(int damage);
    void addMana(int amount);

    // 每帧由 GameManager 调用，按状态机分发行为。
    void updateAction(GameManager* gameMgr);

    // 技能接口：基类有默认技能，具体英雄在 heroes.h 中重写。
    virtual void castSkill(GameManager* gameMgr);

private:
    // 状态机的具体处理函数。
    void handleIdle(GameManager* gameMgr);
    void handleMoving(GameManager* gameMgr);
    void handleAttacking(GameManager* gameMgr);
    void handleCasting(GameManager* gameMgr);
};

#endif // UNIT_H
