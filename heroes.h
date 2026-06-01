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
            target->takeDamage(150);
            gameMgr->addSkillEffect(target->x, target->y, "ryze_blast", 16);
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
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                Unit* ally = gameMgr->getUnitOnBoard(i, j);
                if (ally && ally->owner == this->owner && ally->isAlive()) {
                    ally->hp = std::min(ally->maxHp, ally->hp + 80);
                }
            }
        }
        gameMgr->addSkillEffect(this->x, this->y, "soraka_heal", 24);
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
                    gameMgr->addSkillEffect(i, j, "garen_spin", 18);
                }
            }
        }
    }
};

// ==================== 4. 蕾欧娜（前排坦克 / 眩晕控制） ====================
class Leona : public Unit {
public:
    Leona(Owner _owner = Owner::PlayerCtrl)
        : Unit(480, 28, 1, 70, _owner) {
        name = "Leona";
        cost = 2;
        traits.push_back("Knight");
    }

    void castSkill(GameManager* gameMgr) override {
        if (target && target->isAlive()) {
            target->takeDamage(120);
            target->stunFrames = target->attackInterval * 2;
            gameMgr->addSkillEffect(target->x, target->y, "leona_shield", 16);
        }
    }
};

// ==================== 5. 艾希（后排射手 / 直线 AOE） ====================
class Ashe : public Unit {
public:
    Ashe(Owner _owner = Owner::PlayerCtrl)
        : Unit(260, 42, 5, 50, _owner) {
        name = "Ashe";
        cost = 2;
        traits.push_back("Knight");
    }

    void castSkill(GameManager* gameMgr) override {
        bool hitAny = false;
        int lastHitY = target ? target->y : y;
        for (int d = 0; d < 3; d++) {
            int ty = target ? target->y : y;
            int nx = x;
            int ny = ty + d;
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                Unit* enemy = gameMgr->getUnitOnBoard(nx, ny);
                if (enemy && enemy->owner == Owner::EnemyCtrl && enemy->isAlive()) {
                    enemy->takeDamage(100);
                    hitAny = true;
                }
            }
        }
        if (hitAny && target) {
            gameMgr->addSkillEffect(x, lastHitY, "ashe_arrow", 14);
        } else if (target) {
            gameMgr->addSkillEffect(x, target->y, "ashe_arrow", 14);
        }
    }
};

// ==================== 6. 烬（远程收割 / 锁定最低血量） ====================
class Jhin : public Unit {
public:
    Jhin(Owner _owner = Owner::PlayerCtrl)
        : Unit(240, 60, 4, 80, _owner) {
        name = "Jhin";
        cost = 2;
        traits.push_back("Knight");
    }

    void castSkill(GameManager* gameMgr) override {
        Unit* lowestHpEnemy = nullptr;
        int lowestHp = 999999;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                Unit* enemy = gameMgr->getUnitOnBoard(i, j);
                if (enemy && enemy->owner == Owner::EnemyCtrl && enemy->isAlive()
                    && enemy->hp < lowestHp) {
                    lowestHp = enemy->hp;
                    lowestHpEnemy = enemy;
                }
            }
        }
        if (lowestHpEnemy) {
            lowestHpEnemy->takeDamage(200);
            gameMgr->addSkillEffect(lowestHpEnemy->x, lowestHpEnemy->y, "jhin_snipe", 20);
        }
    }
};

#endif // HEROES_H

