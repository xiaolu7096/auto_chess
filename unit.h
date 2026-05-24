#ifndef UNIT_H
#define UNIT_H

#include <string>
#include <vector>
#include<Item.h>

// 使用枚举 (Enum) 来表示阵营
enum class Owner {
    PlayerCtrl, // 玩家
    EnemyCtrl   // 敌人
};
//单位微观状态机
enum class UnitState{
    Idle,//空闲索敌
    Moving,//移动
    Attacking,//攻击
    Casting,//施法大招
    Dead,//寄
};
class GameManager;//前置声明，避免循环引用
class Unit {
public:
    // --- 构造函数 ---
    // 构造函数用于初始化一个单位的所有数值
    Unit(int hp, int atk, int range, int maxMana, Owner owner);

    // 虚析构函数：由于后面会有英雄继承 Unit，基类的析构函数必须是虚的
    virtual ~Unit();

    // --- 基础属性 (根据文档要求) ---
    int hp;         // 当前生命值
    int maxHp;      // 最大生命值
    int atk;        // 攻击力
    int range;      // 攻击距离 (1代表近战，>1代表远程)
    int mana;       // 当前法力值
    int maxMana;    // 最大法力值

    Owner owner;    // 归属方
    std::vector<std::string> traits; // 羁绊标签 (例如: "Warrior", "Undead")

    // --- 位置属性 (用于逻辑坐标) ---
    int x, y;       // 在棋盘上的坐标 (0~M-1, 0~N-1)
    bool isBench;   // 标记：true 表示在备战区，false 表示在战场

    // ---战斗与状态机相关属性
    UnitState state;          // 当前状态
    Unit* target;             // 当前锁定的敌方目标指针

    // ✨ 新增：帧率控制常数与计数器
    int attackInterval;       // 普攻总冷却帧数（例如 30 帧，即 1 秒一次）
    int attackTimer;          // 普攻计时器（每帧减 1，到 0 可普攻）

    int moveInterval;         // 移动总冷却帧数（例如 20 帧走一格）
    int moveTimer;            // 移动计时器（每帧减 1，到 0 可挪动）

    //阶段三星级属性
    std::string name;
    int star=3;
    int cost=1;
    void evolve(){
        star++;
        maxHp*=1.8;
            hp=maxHp;
        atk*=1.8;

    }


    //装备槽
    std::vector<Item*> equippedItems; // 存放已穿戴的装备

    // 动态限制装备槽：普通英雄最多 1 件，2星及以上扩展为 2 件
    int getMaxItemSlots() const {
        return (star >= 2) ? 2 : 1;
    }

    // 穿戴装备逻辑（成功返回 true，满槽返回 false）
    bool equipItem(Item* item) {
        if ((int)equippedItems.size() >= getMaxItemSlots()) {
            return false;
        }

        equippedItems.push_back(item);

        // 实时应用装备属性变更
        atk += item->bonusAtk;
        maxHp += item->bonusHp;
        hp += item->bonusHp; // 撑大上限的同时按数额回血

        if (maxMana > item->manaReduction) {
            maxMana -= item->manaReduction; // 蓝条变短，技能释放更快
        } else {
            maxMana = 10; // 留个保底低保蓝条
        }

        // 💡 提示：关于攻速加成（bonusSpeed）
        // 如果你的 updateAction 内部有通过 attackCooldown 计时器控制普攻间隔，
        // 可以在执行重置计时逻辑时，将间隔时间除以 (1.0 + item->bonusSpeed)

        return true;
    }
    // --- 基础逻辑函数 ---

    // TODO: 实现一个函数，用于判断该单位是否还活着 (hp > 0)
    bool isAlive() const;

    // TODO: 实现一个受击函数，减少 hp，并确保 hp 不会变成负数
    void takeDamage(int damage);

    // TODO: 实现一个增加法力值的函数，确保 mana 不会超过 maxMana
    void addMana(int amount);

    // ✨ 新增：状态机核心分发器（每帧被 GameManager 驱动）
    void updateAction(GameManager* gameMgr);
    virtual void castSkill(GameManager*gameMgr);

private:
    // ✨ 新增：模块化行为处理器
    void handleIdle(GameManager* gameMgr);
    void handleMoving(GameManager* gameMgr);
    void handleAttacking(GameManager* gameMgr);
    void handleCasting(GameManager* gameMgr);
};


#endif // UNIT_H
