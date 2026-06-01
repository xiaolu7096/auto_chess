#include "advanceditems.h"
#include "gamemanager.h"
#include "unit.h"
#include <algorithm>

// ==================== AdvancedItem 基类 ====================

AdvancedItem::AdvancedItem(const std::string& name, int bonusAtk, int bonusHp,
                           double bonusSpeed, int manaReduction)
    : Item(ItemType::Advanced)
{
    this->name = name;
    this->bonusAtk = bonusAtk;
    this->bonusHp = bonusHp;
    this->bonusSpeed = bonusSpeed;
    this->manaReduction = manaReduction;
}

// 合成配方表：无序匹配两件基础装备，返回对应高级装备或 nullptr
AdvancedItem* AdvancedItem::synthesize(Item* a, Item* b) {
    if (!a || !b) return nullptr;
    if (a->type == ItemType::Advanced || b->type == ItemType::Advanced) return nullptr;

    // 顺序无关，统一排序以简化匹配
    ItemType t1 = a->type, t2 = b->type;
    if (static_cast<int>(t1) > static_cast<int>(t2)) std::swap(t1, t2);

    if (t1 == ItemType::Sword && t2 == ItemType::Armor)  return new ReviveArmor();
    if (t1 == ItemType::Sword && t2 == ItemType::Glove)  return new InfinityEdge();
    if (t1 == ItemType::Sword && t2 == ItemType::Crystal) return new ArchangelStaff();
    if (t1 == ItemType::Armor && t2 == ItemType::Glove)  return new ThornmailArmor();
    if (t1 == ItemType::Armor && t2 == ItemType::Crystal) return new WarmogArmor();
    if (t1 == ItemType::Glove && t2 == ItemType::Crystal) return new LudenEcho();

    return nullptr;
}

// ==================== 复活甲 ====================

ReviveArmor::ReviveArmor()
    : AdvancedItem("复活甲", 15, 150, 0.0, 0) {}

// 每回合开始重置复活标记
void ReviveArmor::onBattleStart(Unit* /*owner*/, GameManager* /*mgr*/) {
    used = false;
}

// 阵亡时触发：满血复活，每回合限一次
void ReviveArmor::onDeath(Unit* owner, GameManager* /*mgr*/, bool& revived) {
    if (!used) {
        used = true;
        revived = true;
        owner->hp = owner->maxHp;
        owner->state = UnitState::Idle;
        owner->target = nullptr;
    }
}

// ==================== 无尽之刃 ====================

InfinityEdge::InfinityEdge()
    : AdvancedItem("无尽之刃", 25, 0, 0.20, 0) {}

// 25% 概率暴击，伤害翻倍
void InfinityEdge::onAttack(Unit* /*owner*/, Unit* /*target*/,
                            int& damage, GameManager* /*mgr*/) {
    if ((std::rand() % 100) < 25) {
        damage *= 2;
    }
}

// ==================== 大天使之杖 ====================

ArchangelStaff::ArchangelStaff()
    : AdvancedItem("大天使之杖", 15, 0, 0.0, 30) {}

// 战斗开始时额外获得 30 法力
void ArchangelStaff::onBattleStart(Unit* owner, GameManager* /*mgr*/) {
    owner->addMana(30);
}

// ==================== 荆棘之甲 ====================

ThornmailArmor::ThornmailArmor()
    : AdvancedItem("荆棘之甲", 0, 200, 0.10, 0) {}

// 受到普攻反弹 30% 伤害给攻击者
void ThornmailArmor::onDamaged(Unit* /*owner*/, Unit* attacker,
                               int& damage, GameManager* /*mgr*/) {
    if (attacker && attacker->isAlive()) {
        attacker->takeDamage(static_cast<int>(damage * 0.30));
    }
}

// ==================== 狂徒铠甲 ====================

WarmogArmor::WarmogArmor()
    : AdvancedItem("狂徒铠甲", 0, 250, 0.0, 15) {}

// 每 60 帧（约 2 秒）恢复 5% 最大生命值
void WarmogArmor::onTick(Unit* owner, GameManager* /*mgr*/, int frameCount) {
    if (frameCount % 60 == 0 && owner->isAlive()) {
        int heal = std::max(1, owner->maxHp / 20);  // 5%
        owner->hp = std::min(owner->maxHp, owner->hp + heal);
    }
}

// ==================== 卢登的回声 ====================

LudenEcho::LudenEcho()
    : AdvancedItem("卢登的回声", 0, 0, 0.10, 20) {}

// 施放技能时对目标 3×3 范围造成 60 点 AOE 伤害
void LudenEcho::onSkillCast(Unit* owner, Unit* target, GameManager* mgr) {
    if (!target) return;

    int startX = target->x - 1;
    int startY = target->y - 1;
    for (int dx = 0; dx < 3; dx++) {
        for (int dy = 0; dy < 3; dy++) {
            int nx = startX + dx;
            int ny = startY + dy;
            Unit* enemy = mgr->getUnitOnBoard(nx, ny);
            if (enemy && enemy->owner != owner->owner && enemy->isAlive()) {
                enemy->takeDamage(60);
            }
        }
    }
}
