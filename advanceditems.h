#ifndef ADVANCEDITEMS_H
#define ADVANCEDITEMS_H

#include "Item.h"
#include <algorithm>
#include <cstdlib>

class Unit;
class GameManager;

// 高级装备基类：所有特殊被动效果的钩子接口
class AdvancedItem : public Item {
public:
    AdvancedItem(const std::string& name, int bonusAtk, int bonusHp,
                 double bonusSpeed, int manaReduction);

    // 战斗开始：重置一次性被动状态
    virtual void onBattleStart(Unit* /*owner*/, GameManager* /*mgr*/) {}

    // 攻击命中前：修改伤害值（用于暴击等）
    virtual void onAttack(Unit* /*owner*/, Unit* /*target*/,
                          int& /*damage*/, GameManager* /*mgr*/) {}

    // 受到攻击前：修改伤害值（用于反弹等）
    virtual void onDamaged(Unit* /*owner*/, Unit* /*attacker*/,
                           int& /*damage*/, GameManager* /*mgr*/) {}

    // 释放技能后：额外效果
    virtual void onSkillCast(Unit* /*owner*/, Unit* /*target*/,
                             GameManager* /*mgr*/) {}

    // 死亡时：设置为复活（revived=true）则取消死亡
    virtual void onDeath(Unit* /*owner*/, GameManager* /*mgr*/,
                         bool& /*revived*/) {}

    // 每帧周期触发：frameCount 为战斗开始以来的帧计数
    virtual void onTick(Unit* /*owner*/, GameManager* /*mgr*/,
                        int /*frameCount*/) {}

    // 传入两件基础装备，匹配合成配方；不匹配返回 nullptr
    static AdvancedItem* synthesize(Item* a, Item* b);

    bool used = false;  // 一次性效果标记（复活甲每回合只能触发一次）
};

// 复活甲：铁剑 + 锁子甲 —— 阵亡时满血复活一次
class ReviveArmor : public AdvancedItem {
public:
    ReviveArmor();
    void onBattleStart(Unit* owner, GameManager* mgr) override;
    void onDeath(Unit* owner, GameManager* mgr, bool& revived) override;
};

// 无尽之刃：铁剑 + 急速手套 —— 25% 概率暴击 2 倍伤害
class InfinityEdge : public AdvancedItem {
public:
    InfinityEdge();
    void onAttack(Unit* owner, Unit* target, int& damage, GameManager* mgr) override;
};

// 大天使之杖：铁剑 + 蓝水晶 —— 战斗开始时额外获得 30 法力
class ArchangelStaff : public AdvancedItem {
public:
    ArchangelStaff();
    void onBattleStart(Unit* owner, GameManager* mgr) override;
};

// 荆棘之甲：锁子甲 + 急速手套 —— 受到普攻反弹 30% 伤害
class ThornmailArmor : public AdvancedItem {
public:
    ThornmailArmor();
    void onDamaged(Unit* owner, Unit* attacker, int& damage, GameManager* mgr) override;
};

// 狂徒铠甲：锁子甲 + 蓝水晶 —— 每 60 帧恢复 5% 最大生命值
class WarmogArmor : public AdvancedItem {
public:
    WarmogArmor();
    void onTick(Unit* owner, GameManager* mgr, int frameCount) override;
};

// 卢登的回声：急速手套 + 蓝水晶 —— 施放技能时对目标 3×3 范围造成 60 点 AOE
class LudenEcho : public AdvancedItem {
public:
    LudenEcho();
    void onSkillCast(Unit* owner, Unit* target, GameManager* mgr) override;
};

#endif // ADVANCEDITEMS_H
