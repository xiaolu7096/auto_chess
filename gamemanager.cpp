#include"gamemanager.h"
#include"heroes.h"
// class GameManager {
// public:
//     GameManager();
//     ~GameManager();

//     // --- 你的任务：定义交互函数 ---
//     // 1. 获取特定位置的单位指针（如果没有则返回 nullptr）
//     Unit*getUnitOnBoard(int x,int y);//拿到（x,y）的指针
//     Unit*getUnitOnBench(int index);
//     // 2. 尝试将单位移动到新位置（处理：目标位为空、目标位有人、非法位置）
//     void MoveUnit(Unit*TargetUnit,int newx,int newy);//把target移到x,y

//     // 3. 移除某个单位（比如英雄被卖掉或战死）
//     void RemoveUnit(Unit*TargetUnit);
//     // --- 你的任务：定义状态查询 ---
//     // 1. 检查当前上阵人数是否超过玩家的人口上限
//     bool Checkpopulation();

// private:
//     // --- 你的任务：定义数据容器 ---
//     // 提示：使用 Unit* board[8][8] 这种形式，或者 std::vector
//     // 别忘了把 Player* 存进来
//     Unit* board[LENGTH][WIDTH];
//     Unit*bench[BENCHSIZE];
//     Player*player;
// };
GameManager::GameManager(){
    for(int i=0;i<LENGTH;i++){
        for(int j=0;j<WIDTH;j++){
            board[i][j]=nullptr;
        }
    }
    for(int i=0;i<BENCHSIZE;i++){
        bench[i]=nullptr;
    }
    for(int i=0;i<5;i++){
        shopSlots[i]=nullptr;
    }
    player=new Player();
    currentState=GameState::Preparation;
    currentround=1;
    refreshShop();

}
GameManager::~GameManager(){
    for(int i=0; i<LENGTH; i++) {
        for(int j=0; j<WIDTH; j++) delete board[i][j];
    }
    for(int i=0; i<BENCHSIZE; i++) delete bench[i];

    delete player; // 释放玩家对象
}
// 在 gamemanager.cpp 中实现：
void GameManager::updateUnitPosition(int oldX, int oldY, int newX, int newY) {
    // 安全检查，防止越界
    if (oldX < 0 || oldX >= 8 || oldY < 0 || oldY >= 8) return;
    if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8) return;

    // 交换棋盘网格中的指针
    board[newX][newY] = board[oldX][oldY];
    board[oldX][oldY] = nullptr;
}
//通用移动接口:备战到战斗，战斗到战斗，战斗到备战
// 建议：使用 bool 返回值告诉 UI 移动是否成功
bool GameManager:: MoveUnit(Unit* target, int nextX, int nextY, bool toBench){
    if (!target) return false;

    // 1. 暂存旧位置信息
    int oldX = target->x;
    int oldY = target->y;
    bool oldIsBench = target->isBench;

    // 2. 获取目标位置现在的单位
    Unit* existingUnit = toBench ? bench[nextX] : board[nextX][nextY];

    // 3. 逻辑判断
    if (existingUnit == nullptr) {
        // 情况 A：目标位为空，直接过去
        // 清理旧位置的指针
        if (toBench) {
            // A-1：如果是移动到备战区（无论是战场下场，还是备战区内左右移动，都不需要卡人口）
            if (oldIsBench) bench[oldX] = nullptr;
            else board[oldX][oldY] = nullptr;

            bench[nextX] = target;
            target->x = nextX; target->y = -1;
            target->isBench = true;
        } else {
            // A-2：如果是准备上阵（从备战区到战场）
            // 💡 【核心修复】：先看能不能上阵，能上阵再清理旧位置！
            if (!oldIsBench || (oldIsBench && Checkpopulation())) {
                // 判定通过，现在可以安全地清除旧位置并写入新位置了
                if (oldIsBench) bench[oldX] = nullptr;
                else board[oldX][oldY] = nullptr;

                board[nextX][nextY] = target;
                target->x = nextX; target->y = nextY;
                target->isBench = false;
            } else {
                std::cout << "❌ 人口已满！当前人口: " << player->getPopulationCap() << "，拒绝上阵！" << std::endl;
                return false; // 此时 bench[oldX] 依然保留着该英雄，它会安然无恙地呆在备战区
            }
        }
    } else if (existingUnit->owner == target->owner) {
        // 情况 B：目标位是队友，执行交换逻辑
        // TODO: 实现交换 board[oldX][oldY] 和 board[nextX][nextY] 的指针
        // 并同时更新这两个 Unit 内部的 x, y, isBench
        Unit*&srcslot=oldIsBench?bench[oldX]:board[oldX][oldY];
        Unit*&desslot=toBench?bench[nextX]:board[nextX][nextY];
        Unit*tmp=srcslot;
        srcslot=desslot;
        desslot=tmp;
        target->x=nextX;
        target->y=toBench?-1:nextY;
        target->isBench=toBench;
        existingUnit->x = oldX;
        existingUnit->y = oldIsBench ? -1 : oldY;
        existingUnit->isBench = oldIsBench;

    } else {
        // 情况 C：目标位是敌人，阶段一通常不允许重叠放置
        return false;
    }
    updateActiveTraits();
    return true;
}
Unit*GameManager::getUnitOnBench(int index){
    return bench[index];
}

Unit*GameManager::getUnitOnBoard(int x,int y){
    return board[x][y];
}
bool GameManager::Checkpopulation(){
    int sum=0;
    for(int i=0;i<LENGTH;i++){
        for(int j=0;j<WIDTH;j++){
            if(board[i][j]!=nullptr&&board[i][j]->owner==Owner::PlayerCtrl)
                sum++;
        }
    }
    if(sum>=player->getPopulationCap()){
        return false;
    }else{
        return true;
    }
}
void GameManager::RemoveUnit(Unit*TargetUnit){
    if(!TargetUnit){
        return;
    }if(TargetUnit->isBench){
        bench[TargetUnit->x]=nullptr;
    }else{
        board[TargetUnit->x][TargetUnit->y]=nullptr;
    }
    delete TargetUnit;
}
void GameManager::spawnEnemyRound(int round){
    //先在前两行按顺序产生与该轮次数量的敌人
    for (int i = 0; i < round && i < LENGTH; i++) {
        if (board[i][0] == nullptr) { // 确保格子是空的
            // 创建一个敌方单位（例如：100血，15攻，1射程，0蓝）
            Unit* enemy = new Unit(100, 15, 1, 0, Owner::EnemyCtrl);
            enemy->x = i;
            enemy->y = 0;
            enemy->isBench = false;
            enemy->traits.push_back("Undead"); // 给敌人打上羁绊标签
            board[i][0] = enemy;
        }
    }
}


//状态机部分
void GameManager::startBattle(){
    if(currentState!=GameState::Preparation){return;}
    // 1. 开战前最后一次精准核算羁绊
    updateActiveTraits();

    // 2. 遍历棋盘，根据羁绊计数，给玩家英雄套上强力的 Buffet 属性改写
    int mageCount = activeTraitsCount["Mage"];
    int vanguardCount = activeTraitsCount["Warrior"];

    for(int i = 0; i < LENGTH; i++) {
        for(int j = 0; j < WIDTH; j++) {
            Unit* u = board[i][j];
            if (u && u->owner == Owner::PlayerCtrl) {

                // 🔥 触发【2法师】羁绊：技能施放频率大提升！
                if (mageCount >= 2 && std::find(u->traits.begin(), u->traits.end(), "Mage") != u->traits.end()) {
                    if(u->maxMana > 30) {
                        u->maxMana -= 20; // 永久减少最大蓝条（仅本轮战斗）
                        std::cout << "🔮 激活【2法师】羁绊！" << u->name << " 最大法力值减少 20！" << std::endl;
                    }
                }

                // 🔥 触发【1重装】羁绊：肉盾顶在前排！
                if (vanguardCount >= 1 && std::find(u->traits.begin(), u->traits.end(), "Vanguard") != u->traits.end()) {
                    u->maxHp += 200;
                    u->hp = u->maxHp; // 撑大血量上限并瞬间满血
                    std::cout << "🛡️ 激活【重装战士】羁绊！" << u->name << " 最大生命值暴涨 200！" << std::endl;
                }
            }
        }
    }
    for(int i = 0; i < LENGTH; i++) {
        for(int j = 0; j < 2; j++) {
            if(board[i][j] != nullptr && board[i][j]->owner == Owner::EnemyCtrl) {
                delete board[i][j];
                board[i][j] = nullptr;
            }
        }
    }
    currentState=GameState::Battle;
    spawnEnemyRound(currentround);
}
void GameManager::updateTick(){
    if(resultDisplayTimer>0){
        resultDisplayTimer--;
    }
    if(currentState==GameState::Battle){
        //遍历所有单位，执行寻路，攻击，释放技能
        for(int i=0;i<LENGTH;i++){
            for(int j=0;j<WIDTH;j++){
                if(board[i][j]!=nullptr){
                    board[i][j]->updateAction(this);
                }
            }
        }
        cleanupDeadUnits();
        checkBattleResult();
    }
}




void GameManager::checkBattleResult() {
    // 只有在战斗进行中，才需要判断胜负
    if (currentState != GameState::Battle) return;

    bool playerAlive = false;
    bool enemyAlive = false;

    // 1. 扫描整个 8x8 棋盘，看看两边是否还有活人
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Unit* u = board[i][j];
            if (u && u->isAlive()) {
                if (u->owner == Owner::PlayerCtrl) {
                    playerAlive = true;
                } else if (u->owner == Owner::EnemyCtrl) {
                    enemyAlive = true;
                }
            }
        }
    }

    // 2. 根据扫描结果判断胜负
    // 情况 A：双方都有活人 -> 战斗继续，直接返回
    if (playerAlive && enemyAlive) {
        return;
    }

    // 接下来是胜负已分的情况，准备打印或通知 UI
    std::cout << "--- 战斗结束 ---" << std::endl;

    if (!playerAlive && !enemyAlive) {
        // 情况 B：罕见的同归于尽（平局）
        std::cout << "居然是平局！双方相安无事。" << std::endl;
        player->addGold(2) ; // 平局低保
        battleResultStr="DRAW";
        resultDisplayTimer=60;
        currentround++;
    }
    else if (playerAlive && !enemyAlive) {
        // 情况 C：玩家胜利（敌人全灭）
        std::cout << "回合胜利！" << std::endl;

        // 👑 【TODO 任务 H-1】: 玩家胜利的奖励经济
        // 规则：
        // 1. 基础获胜奖励：固定获得 5 金币
        // 2. 胜利额外加成：再额外获得 1 金币
        // 请在此处更新 playerGold 的值：
        player->addGold(6);
        battleResultStr="VICTORY";
        resultDisplayTimer=60;
        currentround++;
    }
    else if (!playerAlive && enemyAlive) {
        // 情况 D：玩家失败（我方全灭）
        std::cout << "回合失败！基地受到伤害！" << std::endl;

        // 👑 【TODO 任务 H-2】: 玩家失败的惩罚与金币
        // 规则：
        // 1. 扣除玩家基地血量：playerHp 减少 10 点
        // 2. 失败安慰奖（连败低保）：玩家依然能获得 5 金币
        // 请在此处更新 playerHp 和 playerGold 的值：
        playerHp-=10;
        player->addGold(5);
        battleResultStr="DEFEAT";
        resultDisplayTimer=60;
    }

    // 3. ✨ 战斗清理与状态重置（非常重要！）
    // 战斗结束了，我们需要把幸存的英雄重置回满血满蓝、Idle 状态，准备打下一轮
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Unit* u = board[i][j];
            if (u && u->isAlive()) {
                u->hp = u->maxHp;       // 满血复原
                u->mana = 0;            // 蓝条清空
                u->state = UnitState::Idle;
                u->target = nullptr;
            }
        }
    }

    // 4. 将游戏阶段切回准备阶段，允许玩家刷牌、调整站位
    player->addXP(1);
    currentState = GameState::Preparation;
    std::cout << "当前玩家资产: HP = " << playerHp << ", Gold = " << player->getGold() << std::endl;
}


void GameManager::cleanupDeadUnits() {
    if (currentState != GameState::Battle) return;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Unit* u = board[i][j];

            // 如果这个格子里有人，而且它已经死了（hp <= 0 或者 state == UnitState::Dead）
            if (u && (!u->isAlive() || u->state == UnitState::Dead)) {
                // ✨【新加爆装逻辑】：如果是敌方怪物阵亡，30%概率爆基础装备
                if (u->owner == Owner::EnemyCtrl) {
                    if ((rand() % 100) < 30) {
                        // 随机 roll 一件四种基础装备之一
                        ItemType randomType = static_cast<ItemType>(rand() % 4);
                        Item* droppedItem = new Item(randomType);

                        // 尝试放入装备栏
                        if ((int)itemBench.size() < MAX_ITEM_BENCH) {
                            itemBench.push_back(droppedItem);
                            std::cout << "🎁 啪嗒！怪物掉落了装备：【" << droppedItem->name
                                      << "】，已收入装备栏！" << std::endl;
                        } else {
                            std::cout << "🎁 怪物掉落了装备，但由于你的装备栏已满，装备碎掉了..." << std::endl;
                            delete droppedItem;
                        }
                std::cout << "💀 裁判清理了位于 (" << i << ", " << j << ") 的阵亡单位。" << std::endl;
                delete board[i][j];
                u=nullptr;
                board[i][j]=nullptr;
                // 👑 【TODO 任务 I】: 安全地清除尸体
                // 规则：
                // 1. 动用 C++ 的 delete 关键字，释放该单位占用的动态内存，防止内存泄漏：delete u;
                // 2. 极其重要！把棋盘该格子的指针彻底置为 nullptr，否则下一帧就会发生野指针崩溃！
                // 请在下方写下这两行清理逻辑：
                    }
                }
            }
        }
    }
}
void GameManager::refreshShop(){
    for(int i=0;i<5;i++){
        if(shopSlots[i]!=nullptr){
            delete shopSlots[i];

        }
        shopSlots[i]=nullptr;
        int control=rand()%3;
        switch (control) {
        case 1:
            shopSlots[i]=new Garen(Owner::PlayerCtrl);
            break;
        case 2:
            shopSlots[i]=new Ryze(Owner::PlayerCtrl);
            break;
        case 0:
            shopSlots[i]=new Soraka(Owner::PlayerCtrl);
            break;
        default:
            break;
        }
    }
    std::cout<<"商店刷新成功"<<std::endl;

}
void GameManager::buyXP() {
    if (currentState != GameState::Preparation) {
        std::cout << "❌ 只有在准备阶段才能购买经验！" << std::endl;
        return;
    }

    // 规则：每次花费 4 金币，购买 4 点经验值
    if (player->spendGold(4)) {
        player->addXP(4);
        std::cout << "🔮 成功花费 4G 购买了 4 点经验值。当前等级: "
                  << player->getPopulationCap() << "级" << std::endl;
    } else {
        std::cout << "❌ 金币不足，无法购买经验！" << std::endl;
    }

} // 如果有声明可以先留空
int GameManager::getpoplulation(){
    return player->getPopulationCap();
}
// 手动花钱刷新商店
void GameManager::refreshShopManual() {
    if (currentState != GameState::Preparation) return; // 只有准备阶段能刷

    // 👑 调用 player 的消费接口扣除 2 金币
    if (player->spendGold(2)) {
        refreshShop(); // 扣钱成功，触发刷新
    } else {
        std::cout << "❌ 金币不足，无法刷新商店！" << std::endl;
    }
}
bool GameManager::buyHeroFromShop(int shopIndex) {
    if (shopIndex < 0 || shopIndex >= 5) return false;

    // 1. 检查该格子的商品是否已被买走
    Unit* hero = shopSlots[shopIndex];
    if (hero == nullptr) {
        std::cout << "❌ 该商品已售罄！" << std::endl;
        return false;
    }

    // 2. 检查备战区是否有空位，并找到第一个空位的索引
    int emptyBenchIndex = -1;
    for (int i = 0; i < BENCHSIZE; i++) {
        if (bench[i] == nullptr) {
            emptyBenchIndex = i;
            break;
        }
    }

    if (emptyBenchIndex == -1) {
        std::cout << "❌ 备战区已满，无法购买新英雄！" << std::endl;
        return false;
    }

    // 3. 检查并扣除金币
    if (!player->spendGold(hero->cost)) {
        std::cout << "❌ 金币不足，无法购买该英雄！" << std::endl;
        return false;
    }

    // 4. 成功扣钱且有空位，开始转移指针
    bench[emptyBenchIndex] = hero;    // 放入备战区
    hero->x=emptyBenchIndex;
    hero->y=-1;
    hero->isBench=true;
    shopSlots[shopIndex] = nullptr;   // 商店该格子置为空（显示已售罄）

    // 5. 极其重要：更新英雄的内部逻辑坐标，使其与备战区格子挂钩
    // 假设你的备战区逻辑坐标是 y = -1，x = 0~7（或者根据你之前阶段一的定义来定）
    hero->x = emptyBenchIndex;
    hero->y = -1;

    std::cout << "💰 成功购买 " << hero->name << "，放入备战区第 " << emptyBenchIndex + 1 << " 个格子！" << std::endl;

    // ✨ 暂留：这里后面我们要加上“自动触发三合一升星检查”
    checkAndCombineStars();

    return true;
}
void GameManager::checkAndCombineStars() {
    // 这里的逻辑比较复杂，我们采用“名字+星级”计数法
    // 遍历所有可能的英雄组合（Garen, Ryze, Soraka）
    std::vector<std::string> heroNames = {"Garen", "Ryze", "Soraka"};

    for (const std::string& name : heroNames) {
        // 分别检查 1星升2星，2星升3星
        for (int s = 1; s <= 2; s++) {
            std::vector<Unit*> matches;

            // 1. 扫描备战区
            for (int i = 0; i < BENCHSIZE; i++) {
                if (bench[i] && bench[i]->owner == Owner::PlayerCtrl &&
                    bench[i]->name == name && bench[i]->star == s) {
                    matches.push_back(bench[i]);
                }
            }

            // 2. 扫描棋盘
            for (int i = 0; i < LENGTH; i++) {
                for (int j = 0; j < WIDTH; j++) {
                    if (board[i][j] && board[i][j]->owner == Owner::PlayerCtrl &&
                        board[i][j]->name == name && board[i][j]->star == s) {
                        matches.push_back(board[i][j]);
                    }
                }
            }

            // 3. ✨ 如果凑齐了 3 个，开始“三合一”
            if (matches.size() >= 3) {
                std::cout << "✨ 检测到三连！正在合成 " << s + 1 << " 星 " << name << std::endl;

                // 规则：保留第一个单位并升级，销毁后两个单位
                Unit* survivor = matches[0];
                Unit* sacrifice1 = matches[1];
                Unit* sacrifice2 = matches[2];

                // 属性进化
                survivor->evolve();

                // 从棋盘或备战区把祭品抹除并删除内存
                auto remove = [&](Unit* target) {
                    if (target->isBench) bench[target->x] = nullptr;
                    else board[target->x][target->y] = nullptr;
                    delete target;
                };

                remove(sacrifice1);
                remove(sacrifice2);

                // 递归检查：合成 2 星后，可能又凑齐了 3 个 2 星，需要继续升 3 星
                checkAndCombineStars();
                return; // 合成一次后跳出，防止指针混乱
            }
        }
    }
}
void GameManager::updateActiveTraits() {
    activeTraitsCount.clear(); // 每次扫描前先清空旧数据

    // 用一个 Map 来记录每种羁绊里，有哪些独特的英雄名字上阵了
    // ✨【核心修复】：将 set 的类型由 string 改为 Unit* 指针
    // 因为每个 new 出来的盖伦指针都是唯一的，这样 3 个三星盖伦就会被算作 3 个独立战力！
    std::map<std::string, std::set<Unit*>> traitToHeroUnits;

    // 1. 扫描 8x8 棋盘上所有属于玩家的活人
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Unit* u = board[i][j];
            if (u && u->isAlive() && u->owner == Owner::PlayerCtrl) {
                // 遍历这个英雄拥有的所有羁绊标签
                for (const std::string& trait : u->traits) {
                    traitToHeroUnits[trait].insert(u); // 利用 set 的唯一性自动去重
                }
            }
        }
    }

    // 2. 将去重后的数量转存到 activeTraitsCount 中，并在控制台打印
    std::cout << "=== 当前实时上阵羁绊 ===" << std::endl;
    for (auto const& [trait, heroesSet] : traitToHeroUnits) {
        int count = heroesSet.size();
        activeTraitsCount[trait] = count;
        std::cout << "✦ " << trait << " : " << count << " 个独特英雄在场" << std::endl;
    }
}