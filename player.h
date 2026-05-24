#ifndef PLAYER_H
#define PLAYER_H
//player模块：资源状态：血量、金币、等级、当前人口上限 。  职责：在后续阶段负责处理购买单位和升级人口的逻辑 。

class Player{
public:
    Player();//
    ~Player();

    //行为函数

    void addGold(int amount);
        //钱加
    bool spendGold(int amount);
        //返回 bool，如果金币不足则返回 false 并不扣钱
    void takeDamage(int damage);
    void addXP(int amount);

    //访问器(访问保护起来的private变量）
    int getHp()const{return hp;}
    int getGold(){return gold;}
    int getLevel(){return level;}
    int getPopulationCap()const;


private:
    int hp;//
    int gold;//
    int level;//
    int exp;//当前经验
    int expToNextLevel;//到下一级所需经验值
    void checkLevelUp();//检查是否满足升级
};



#endif // PLAYER_H
