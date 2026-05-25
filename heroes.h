#ifndef HEROES_H
#define HEROES_H
#include "unit.h"
#include "gamemanager.h"
#include <algorithm>

// ==================== 1. 流浪法师（单体高爆发） ====================
class Ryze : public Unit {
public:
    // 调用基类构造函数：HP=300, ATK=45, Range=4(远程), MaxMana=50, 属于玩家
    Ryze(Owner _owner = Owner::PlayerCtrl)
        : Unit(300, 45, 4, 50, _owner) {
        name="Ryze";
        cost=2;
        traits.push_back("Mage");
    }

    // 重写大招：超负荷法球
    void castSkill(GameManager* gameMgr) override {
        if (target && target->isAlive()) {
            // 对当前锁定的目标造成 150 点巨额魔法伤害
            target->takeDamage(150);
        }
    }
};

// ==================== 2. 众星之子（全场友军大回血） ====================
class Soraka : public Unit {
public:
    // HP=250, ATK=20, Range=3, MaxMana=60
    Soraka(Owner _owner = Owner::PlayerCtrl)
        : Unit(250, 20, 3, 60, _owner) {
        name="Soraka";
        cost=2;
        traits.push_back("Healer");
    }

    // 重写大招：祈愿
    void castSkill(GameManager* gameMgr) override {
        // 扫描全场 8x8 棋盘，拯救所有和自己同一阵营的活着的队友！
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                Unit* ally = gameMgr->getUnitOnBoard(i, j);
                if (ally && ally->owner == this->owner && ally->isAlive()) {
                    // 回复 80 点生命值，但不能超过最大生命值上限
                    ally->hp = std::min(ally->maxHp, ally->hp + 80);
                }
            }
        }
    }
};
//3.战士盖伦
class Garen:public Unit{
public:
    //HP 较高（如 450），ATK 中等（如 35），Range 必须是 1（近战），最大法力值 MaxMana=60。
    Garen(Owner _owner=Owner::PlayerCtrl):Unit(450,35,1,60,_owner){
        name="Garen";
        cost=1;
        traits.push_back("Vanguard");
    }
    //大招：旋风斩
    void castSkill(GameManager*gameMgr) override{
        int startX=this->x-1;
        int startY=this->y-1;
        for(int i=startX;i<=startX+2;i++){
            for(int j=startY;j<=startY+2;j++){
                if(i>=0&&i<8&&j>=0&&j<8){
                    Unit*targetUnit=gameMgr->getUnitOnBoard(i,j);
                    if(targetUnit!=nullptr&&targetUnit->owner==Owner::EnemyCtrl&&targetUnit->isAlive()){
                        targetUnit->takeDamage(80);
                    }
                }
            }
        }
    }
};

#endif // HEROES_H

