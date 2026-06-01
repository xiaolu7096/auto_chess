#ifndef ITEM_H
#define ITEM_H
#pragma once
#include <string>

enum class ItemType {
    Sword,      // 铁剑：攻击力 +15
    Armor,      // 锁子甲：生命值 +150
    Glove,      // 急速手套：攻速提升 20%
    Crystal,    // 蓝水晶：最大法力值 -30
    Advanced    // 高级合成装备
};

class Item {
public:
    ItemType type;
    std::string name;
    int bonusAtk = 0;
    int bonusHp = 0;
    double bonusSpeed = 0.0;
    int manaReduction = 0;

    Item(ItemType t) : type(t) {
        switch (t) {
        case ItemType::Sword:
            name = "铁剑"; bonusAtk = 15; break;
        case ItemType::Armor:
            name = "锁子甲"; bonusHp = 150; break;
        case ItemType::Glove:
            name = "急速手套"; bonusSpeed = 0.20; break;
        case ItemType::Crystal:
            name = "蓝水晶"; manaReduction = 30; break;
        case ItemType::Advanced:
            break;  // 由子类构造函数自行设置属性
        }
    }

    virtual ~Item() = default;
};
#endif // ITEM_H
