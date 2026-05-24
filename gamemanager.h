#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#define BENCHSIZE 8
#define LENGTH 8
#define WIDTH 8
/*
下一步行动指南阶段二是一个大工程，切忌眉毛胡子一把抓。我们的最佳开发顺序应当是：
第 1 步：在 GameManager 里引入 QTimer，让游戏有“时间”概念，先实现全局的
【准备 $\rightarrow$ 战斗 $\rightarrow$ 结算】大循环。
第 2 步：丰富 Unit 类，引入状态枚举，写出最核心的“索敌(寻找最近敌人)”函数。
第 3 步：实现基础的移动与站桩普攻。第 4 步：利用多态写出 3 个不同的英雄类和大招。你对这个整体架构感觉如何？我们是先从引入 QTimer 搭建全局的三个阶段循环开始，还是先去重构 Unit 的状态机和子类多态？*/



#include"unit.h"
#include"player.h"
#include"Qstring"
#include<map>
#include<set>
#include<iostream>
//定义全局游戏状态
enum class GameState{
    Preparation,//备战
    Battle,//战斗
    Settlement,//计算
};

class GameManager {
public:
    GameManager();
    ~GameManager();

//阶段一//
    // --- 你的任务：定义交互函数 ---
    // 1. 获取特定位置的单位指针（如果没有则返回 nullptr）
    Unit*getUnitOnBoard(int x,int y);//拿到（x,y）的指针
    Unit*getUnitOnBench(int index);
    // 在 gamemanager.h 的 public 区域加入：
    void updateUnitPosition(int oldX, int oldY, int newX, int newY);
    // 2. 尝试将单位移动到新位置（处理：目标位为空、目标位有人、非法位置）
    bool MoveUnit(Unit* target, int nextX, int nextY, bool toBench);//把target移到x,y
    // 3. 移除某个单位（比如英雄被卖掉或战死）
    void RemoveUnit(Unit*TargetUnit);
    // --- 你的任务：定义状态查询 ---
    // 1. 检查当前上阵人数是否超过玩家的人口上限
    bool Checkpopulation();
    //---敌人生成与移除
    void spawnEnemyRound(int round);
//阶段二//
    GameState getState(){return currentState; };
    int getCurrentRound(){return currentround;};
    void startBattle();//切换到战斗状态
    void updateTick();

    //胜负判断
    // ✨ 新增：全局经济与基地血量
    int playerHp = 100;    // 玩家初始 100 血
    int enemyHp = 100;     // 敌方初始 100 血（或者叫关卡血量）
    int resultDisplayTimer=0;//
    QString battleResultStr="NONE";
        // 玩家初始 5 金币
    // ✨ 核心裁判函数
    void checkBattleResult();
    void cleanupDeadUnits();

    //阶段三
    void refreshShop();
    void refreshShopManual();
    bool buyHeroFromShop(int shopIndex);//买第index个英雄
    void buyXP();//买经验值
    int getpoplulation();
    Unit*getShopSlot(int index){
        return shopSlots[index];
    }
    int getPlayerGold(){return player->getGold();}
    //升星系统
    void checkAndCombineStars();
    //羁绊
    std::map<std::string,int>activeTraitsCount;
    void updateActiveTraits();

    //装备容器
    std::vector<Item*> itemBench; // 玩家的装备库存栏（最大存放8件）
    const int MAX_ITEM_BENCH = 8;
private:
    // --- 你的任务：定义数据容器 ---
    // 提示：使用 Unit* board[8][8] 这种形式，或者 std::vector
    // 别忘了把 Player* 存进来
    Unit* board[LENGTH][WIDTH];
    Unit*bench[BENCHSIZE];
    Player*player;
    GameState currentState;
    int currentround;
    Unit*shopSlots[5];
};
#endif // GAMEMANAGER_H
