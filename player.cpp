#include"player.h"
#include<iostream>
#include<algorithm>


Player::Player() {
    // TODO: 初始化数值
    //
    hp=100;
    gold=10;
    level=5;
    exp=0;
    expToNextLevel=4;
}
Player::~Player(){

}
void Player::addGold(int amount) {
    // TODO: 增加金币
    gold+=amount;
}

bool Player::spendGold(int amount) {
    // TODO: 如果 gold >= amount，扣除金币并返回 true
    // 否则返回 false（提示：这在后面买英雄时非常有用）
    if(gold>=amount){
        gold-=amount;
        return true;
    }else{
        return false;
    }
}

void Player::takeDamage(int damage) {
    // TODO: 扣除血量，使用 std::max(0, hp - damage) 确保不为负数
    hp=std::max(0,hp-damage);//max,min函数在有底线的数值计算非常方便
}

void Player::addXP(int amount) {
    // TODO: 增加 exp，然后调用 checkLevelUp()
    exp+=amount;
    checkLevelUp();
}

int Player::getPopulationCap() const {
    // TODO: 根据等级返回人口上限
    // 简单的逻辑可以是：人口上限 = 当前等级

    return level;
}

void Player::checkLevelUp() {
    // TODO: 这是一个循环逻辑
    while(exp>=expToNextLevel){
        exp-=expToNextLevel;
        level++;
    }
}