#include "gamemanager.h"
#include "heroes.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

GameManager::GameManager() {
    // 初始化棋盘、备战区和商店槽位，所有指针先置空。
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            board[i][j] = nullptr;
        }
    }
    for (int i = 0; i < BENCHSIZE; i++) {
        bench[i] = nullptr;
    }
    for (int i = 0; i < 5; i++) {
        shopSlots[i] = nullptr;
    }

    player = new Player();
    currentState = GameState::Preparation;
    currentround = 1;
    refreshShop();
}

GameManager::~GameManager() {
    // GameManager 拥有棋盘、备战区、商店和装备栏中的动态对象，退出时统一释放。
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            delete board[i][j];
        }
    }
    for (int i = 0; i < BENCHSIZE; i++) {
        delete bench[i];
    }
    for (int i = 0; i < 5; i++) {
        delete shopSlots[i];
    }
    for (Item* item : itemBench) {
        delete item;
    }
    delete player;
}

void GameManager::updateUnitPosition(int oldX, int oldY, int newX, int newY) {
    // 战斗中移动单位时，同步棋盘数组里的指针。
    if (oldX < 0 || oldX >= LENGTH || oldY < 0 || oldY >= WIDTH) return;
    if (newX < 0 || newX >= LENGTH || newY < 0 || newY >= WIDTH) return;
    board[newX][newY] = board[oldX][oldY];
    board[oldX][oldY] = nullptr;
}

bool GameManager::MoveUnit(Unit* target, int nextX, int nextY, bool toBench) {
    // 准备阶段拖拽单位：支持棋盘/备战区移动、同阵营交换、人口限制。
    if (!target) return false;
    if (nextX < 0 || nextX >= BENCHSIZE) return false;
    if (!toBench && (nextY < 0 || nextY >= WIDTH)) return false;

    int oldX = target->x;
    int oldY = target->y;
    bool oldIsBench = target->isBench;
    Unit* existingUnit = toBench ? bench[nextX] : board[nextX][nextY];

    if (existingUnit == nullptr) {
        if (!toBench && oldIsBench && !Checkpopulation()) {
            std::cout << "Population is full, cannot deploy more units." << std::endl;
            return false;
        }

        if (oldIsBench) {
            bench[oldX] = nullptr;
        } else {
            board[oldX][oldY] = nullptr;
        }

        if (toBench) {
            bench[nextX] = target;
            target->x = nextX;
            target->y = -1;
            target->isBench = true;
        } else {
            board[nextX][nextY] = target;
            target->x = nextX;
            target->y = nextY;
            target->isBench = false;
        }
    } else if (existingUnit->owner == target->owner) {
        Unit*& srcSlot = oldIsBench ? bench[oldX] : board[oldX][oldY];
        Unit*& dstSlot = toBench ? bench[nextX] : board[nextX][nextY];
        std::swap(srcSlot, dstSlot);

        target->x = nextX;
        target->y = toBench ? -1 : nextY;
        target->isBench = toBench;

        existingUnit->x = oldX;
        existingUnit->y = oldIsBench ? -1 : oldY;
        existingUnit->isBench = oldIsBench;
    } else {
        return false;
    }

    updateActiveTraits();
    return true;
}

Unit* GameManager::getUnitOnBench(int index) {
    // 获取备战区指定格子的单位。
    if (index < 0 || index >= BENCHSIZE) return nullptr;
    return bench[index];
}

Unit* GameManager::getUnitOnBoard(int x, int y) {
    // 获取棋盘指定格子的单位。
    if (x < 0 || x >= LENGTH || y < 0 || y >= WIDTH) return nullptr;
    return board[x][y];
}

bool GameManager::Checkpopulation() {
    // 判断当前上阵人数是否低于人口上限。
    int deployed = 0;
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (board[i][j] && board[i][j]->owner == Owner::PlayerCtrl) {
                deployed++;
            }
        }
    }
    return deployed < player->getPopulationCap();
}

void GameManager::RemoveUnit(Unit* targetUnit) {
    // 从棋盘或备战区移除一个单位，并释放内存。
    if (!targetUnit) return;
    if (targetUnit->isBench) {
        bench[targetUnit->x] = nullptr;
    } else {
        board[targetUnit->x][targetUnit->y] = nullptr;
    }
    delete targetUnit;
    updateActiveTraits();
}

void GameManager::spawnEnemyRound(int round) {
    // 每回合按轮数生成更强的敌人，数量最多 6 个，避免棋盘过满。
    int enemyCount = std::min(2 + round / 2, 6);
    int bonusHp = (round - 1) * 35;
    int bonusAtk = (round - 1) * 5;

    for (int i = 0; i < enemyCount; i++) {
        int x = i % LENGTH;
        int y = i / LENGTH;
        if (y >= 2 || board[x][y] != nullptr) {
            continue;
        }

        Unit* enemy = nullptr;
        if (round >= 3 && i % 3 == 2) {
            enemy = new Ryze(Owner::EnemyCtrl);
        } else if (round >= 2 && i % 3 == 1) {
            enemy = new Garen(Owner::EnemyCtrl);
        } else {
            enemy = new Unit(120, 18, 1, 0, Owner::EnemyCtrl);
            enemy->name = "Enemy";
            enemy->traits.push_back("Monster");
        }

        enemy->maxHp += bonusHp;
        enemy->hp = enemy->maxHp;
        enemy->atk += bonusAtk;
        enemy->x = x;
        enemy->y = y;
        enemy->isBench = false;
        board[x][y] = enemy;
    }
}

void GameManager::startBattle() {
    // 从准备阶段切入战斗：清理旧敌人、应用羁绊、生成本轮敌人。
    if (currentState != GameState::Preparation) return;

    updateActiveTraits();
    int mageCount = activeTraitsCount["Mage"];
    int vanguardCount = activeTraitsCount["Vanguard"];

    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Unit* u = board[i][j];
            if (!u || u->owner != Owner::PlayerCtrl) continue;

            u->state = UnitState::Idle;
            u->target = nullptr;
            u->mana = 0;

            if (mageCount >= 2 &&
                std::find(u->traits.begin(), u->traits.end(), "Mage") != u->traits.end()) {
                u->maxMana = std::max(10, u->maxMana - 20);
            }

            if (vanguardCount >= 1 &&
                std::find(u->traits.begin(), u->traits.end(), "Vanguard") != u->traits.end()) {
                u->maxHp += 120;
                u->hp = u->maxHp;
            }
        }
    }

    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < 2; j++) {
            if (board[i][j] && board[i][j]->owner == Owner::EnemyCtrl) {
                delete board[i][j];
                board[i][j] = nullptr;
            }
        }
    }

    spawnEnemyRound(currentround);
    currentState = GameState::Battle;
}

void GameManager::updateTick() {
    // 主时钟入口：战斗中驱动单位；结算展示结束后自动回到准备阶段。
    if (resultDisplayTimer > 0) {
        resultDisplayTimer--;
        if (resultDisplayTimer == 0 && currentState == GameState::Settlement) {
            currentState = GameState::Preparation;
            refreshShop();
        }
    }

    if (currentState != GameState::Battle) return;

    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (board[i][j]) {
                board[i][j]->updateAction(this);
            }
        }
    }

    cleanupDeadUnits();
    checkBattleResult();
}

void GameManager::checkBattleResult() {
    // 扫描双方是否还有存活单位，若一方清空则进入结算阶段。
    if (currentState != GameState::Battle) return;

    bool playerAlive = false;
    bool enemyAlive = false;
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Unit* u = board[i][j];
            if (!u || !u->isAlive()) continue;
            if (u->owner == Owner::PlayerCtrl) {
                playerAlive = true;
            } else {
                enemyAlive = true;
            }
        }
    }

    if (playerAlive && enemyAlive) return;

    currentState = GameState::Settlement;
    resultDisplayTimer = 60;

    if (!playerAlive && !enemyAlive) {
        battleResultStr = "DRAW";
        player->addGold(2);
        currentround++;
    } else if (playerAlive) {
        battleResultStr = "VICTORY";
        player->addGold(6);
        currentround++;
    } else {
        battleResultStr = "DEFEAT";
        playerHp = std::max(0, playerHp - 10);
        player->addGold(5);
    }

    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Unit* u = board[i][j];
            if (!u) continue;
            if (u->owner == Owner::EnemyCtrl) {
                delete u;
                board[i][j] = nullptr;
                continue;
            }
            if (u->owner == Owner::PlayerCtrl && u->isAlive()) {
                u->hp = u->maxHp;
                u->mana = 0;
                u->state = UnitState::Idle;
                u->target = nullptr;
            }
        }
    }

    player->addXP(1);
    updateActiveTraits();
}

void GameManager::cleanupDeadUnits() {
    // 清理战斗中阵亡的单位；敌人死亡时有概率掉落基础装备。
    if (currentState != GameState::Battle) return;

    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Unit* u = board[i][j];
            if (!u || (u->isAlive() && u->state != UnitState::Dead)) {
                continue;
            }

            if (u->owner == Owner::EnemyCtrl && (rand() % 100) < 30) {
                Item* droppedItem = new Item(static_cast<ItemType>(rand() % 4));
                if ((int)itemBench.size() < MAX_ITEM_BENCH) {
                    itemBench.push_back(droppedItem);
                } else {
                    delete droppedItem;
                }
            }

            delete u;
            board[i][j] = nullptr;
        }
    }
}

void GameManager::refreshShop() {
    // 刷新 5 个商店格子，随机放入当前可购买的英雄。
    for (int i = 0; i < 5; i++) {
        delete shopSlots[i];
        shopSlots[i] = nullptr;

        int control = rand() % 3;
        if (control == 0) {
            shopSlots[i] = new Garen(Owner::PlayerCtrl);
        } else if (control == 1) {
            shopSlots[i] = new Ryze(Owner::PlayerCtrl);
        } else {
            shopSlots[i] = new Soraka(Owner::PlayerCtrl);
        }
    }
}

void GameManager::buyXP() {
    // 花费金币购买经验，提升等级后人口上限随之提高。
    if (currentState != GameState::Preparation) return;
    if (player->spendGold(4)) {
        player->addXP(4);
    }
}

int GameManager::getpoplulation() {
    // 返回当前人口上限，用于 UI 展示。
    return player->getPopulationCap();
}

void GameManager::refreshShopManual() {
    // 玩家手动刷新商店，准备阶段花费 2 金币。
    if (currentState != GameState::Preparation) return;
    if (player->spendGold(2)) {
        refreshShop();
    }
}

bool GameManager::buyHeroFromShop(int shopIndex) {
    // 从商店购买英雄，成功后放入第一个空备战格。
    if (shopIndex < 0 || shopIndex >= 5) return false;
    Unit* hero = shopSlots[shopIndex];
    if (!hero) return false;

    int emptyBenchIndex = -1;
    for (int i = 0; i < BENCHSIZE; i++) {
        if (!bench[i]) {
            emptyBenchIndex = i;
            break;
        }
    }
    if (emptyBenchIndex == -1) return false;
    if (!player->spendGold(hero->cost)) return false;

    bench[emptyBenchIndex] = hero;
    hero->x = emptyBenchIndex;
    hero->y = -1;
    hero->isBench = true;
    shopSlots[shopIndex] = nullptr;

    checkAndCombineStars();
    updateActiveTraits();
    return true;
}

void GameManager::checkAndCombineStars() {
    // 三个同名同星级英雄自动合成一个高一星英雄，保留第一个找到的单位。
    std::vector<std::string> heroNames = {"Garen", "Ryze", "Soraka"};

    for (const std::string& name : heroNames) {
        for (int s = 1; s <= 2; s++) {
            std::vector<Unit*> matches;

            for (int i = 0; i < BENCHSIZE; i++) {
                if (bench[i] && bench[i]->owner == Owner::PlayerCtrl &&
                    bench[i]->name == name && bench[i]->star == s) {
                    matches.push_back(bench[i]);
                }
            }

            for (int i = 0; i < LENGTH; i++) {
                for (int j = 0; j < WIDTH; j++) {
                    if (board[i][j] && board[i][j]->owner == Owner::PlayerCtrl &&
                        board[i][j]->name == name && board[i][j]->star == s) {
                        matches.push_back(board[i][j]);
                    }
                }
            }

            if (matches.size() >= 3) {
                Unit* survivor = matches[0];
                Unit* sacrifice1 = matches[1];
                Unit* sacrifice2 = matches[2];
                survivor->evolve();

                auto removeUnitOnly = [&](Unit* target) {
                    if (target->isBench) {
                        bench[target->x] = nullptr;
                    } else {
                        board[target->x][target->y] = nullptr;
                    }
                    delete target;
                };

                removeUnitOnly(sacrifice1);
                removeUnitOnly(sacrifice2);
                checkAndCombineStars();
                return;
            }
        }
    }
}

void GameManager::updateActiveTraits() {
    // 统计当前棋盘上玩家单位的羁绊数量，供 UI 和开战 buff 使用。
    activeTraitsCount.clear();
    std::map<std::string, std::set<Unit*>> traitToHeroUnits;

    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            Unit* u = board[i][j];
            if (!u || !u->isAlive() || u->owner != Owner::PlayerCtrl) continue;
            for (const std::string& trait : u->traits) {
                traitToHeroUnits[trait].insert(u);
            }
        }
    }

    for (auto const& [trait, heroesSet] : traitToHeroUnits) {
        activeTraitsCount[trait] = (int)heroesSet.size();
    }
}

Unit* GameManager::createUnitByName(const std::string& name, Owner owner) {
    // 读档辅助：根据存档中的英雄名重新创建正确的子类对象。
    if (name == "Garen") return new Garen(owner);
    if (name == "Ryze") return new Ryze(owner);
    if (name == "Soraka") return new Soraka(owner);

    Unit* enemy = new Unit(120, 18, 1, 0, owner);
    enemy->name = name.empty() ? "Enemy" : name;
    if (enemy->traits.empty()) {
        enemy->traits.push_back("Monster");
    }
    return enemy;
}

Item* GameManager::createItemByName(const std::string& name) {
    // 读档辅助：根据装备名恢复装备对象；兼容中英文/乱码名时优先按存档类型字段保存。
    if (name == "Sword") return new Item(ItemType::Sword);
    if (name == "Armor") return new Item(ItemType::Armor);
    if (name == "Glove") return new Item(ItemType::Glove);
    if (name == "Crystal") return new Item(ItemType::Crystal);
    return new Item(ItemType::Sword);
}

static QString ownerToString(Owner owner) {
    // 把阵营枚举转成 JSON 中可读的字符串。
    return owner == Owner::PlayerCtrl ? "Player" : "Enemy";
}

static Owner ownerFromString(const QString& owner) {
    // 把 JSON 字符串还原成阵营枚举。
    return owner == "Enemy" ? Owner::EnemyCtrl : Owner::PlayerCtrl;
}

static QString stateToString(GameState state) {
    // 把主阶段转成 JSON 字符串。
    if (state == GameState::Battle) return "Battle";
    if (state == GameState::Settlement) return "Settlement";
    return "Preparation";
}

static GameState stateFromString(const QString& state) {
    // 把 JSON 字符串还原成主阶段。
    if (state == "Battle") return GameState::Battle;
    if (state == "Settlement") return GameState::Settlement;
    return GameState::Preparation;
}

static QString itemTypeToString(ItemType type) {
    // 把装备类型转成稳定英文，避免中文显示编码影响读档。
    switch (type) {
    case ItemType::Sword: return "Sword";
    case ItemType::Armor: return "Armor";
    case ItemType::Glove: return "Glove";
    case ItemType::Crystal: return "Crystal";
    }
    return "Sword";
}

static QJsonObject itemToJson(Item* item) {
    // 把单件装备写成 JSON 对象。
    QJsonObject obj;
    obj["type"] = itemTypeToString(item->type);
    return obj;
}

static QJsonObject unitToJson(Unit* unit, int slotX, int slotY, const QString& area) {
    // 把一个单位的核心状态、位置、羁绊和已穿装备写入 JSON。
    QJsonObject obj;
    obj["area"] = area;
    obj["x"] = slotX;
    obj["y"] = slotY;
    obj["name"] = QString::fromStdString(unit->name);
    obj["owner"] = ownerToString(unit->owner);
    obj["hp"] = unit->hp;
    obj["maxHp"] = unit->maxHp;
    obj["atk"] = unit->atk;
    obj["range"] = unit->range;
    obj["mana"] = unit->mana;
    obj["maxMana"] = unit->maxMana;
    obj["star"] = unit->star;
    obj["cost"] = unit->cost;

    QJsonArray traits;
    for (const std::string& trait : unit->traits) {
        traits.append(QString::fromStdString(trait));
    }
    obj["traits"] = traits;

    QJsonArray equipped;
    for (Item* item : unit->equippedItems) {
        equipped.append(itemToJson(item));
    }
    obj["equippedItems"] = equipped;
    return obj;
}

bool GameManager::saveGame(const QString& filePath) {
    // 保存完整游戏状态：玩家资源、回合、阶段、棋盘、备战区、装备库存和商店。
    QJsonObject root;
    root["version"] = 1;
    root["state"] = stateToString(currentState);
    root["round"] = currentround;
    root["playerHp"] = playerHp;
    root["enemyHp"] = enemyHp;
    root["battleResult"] = battleResultStr;

    QJsonObject playerObj;
    playerObj["hp"] = player->getHp();
    playerObj["gold"] = player->getGold();
    playerObj["level"] = player->getLevel();
    playerObj["exp"] = player->getExp();
    playerObj["expToNextLevel"] = player->getExpToNextLevel();
    root["player"] = playerObj;

    QJsonArray units;
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (board[i][j]) {
                units.append(unitToJson(board[i][j], i, j, "board"));
            }
        }
    }
    for (int i = 0; i < BENCHSIZE; i++) {
        if (bench[i]) {
            units.append(unitToJson(bench[i], i, -1, "bench"));
        }
    }
    root["units"] = units;

    QJsonArray items;
    for (Item* item : itemBench) {
        items.append(itemToJson(item));
    }
    root["itemBench"] = items;

    QJsonArray shop;
    for (int i = 0; i < 5; i++) {
        if (shopSlots[i]) {
            shop.append(unitToJson(shopSlots[i], i, -2, "shop"));
        } else {
            QJsonObject emptySlot;
            emptySlot["area"] = "shop";
            emptySlot["x"] = i;
            emptySlot["empty"] = true;
            shop.append(emptySlot);
        }
    }
    root["shop"] = shop;

    std::ofstream file(filePath.toStdString(), std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    file.write(json.constData(), json.size());
    return true;
}

bool GameManager::loadGame(const QString& filePath) {
    // 从 JSON 存档恢复游戏；先清空当前对象，再按存档内容重建。
    std::ifstream file(filePath.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(content), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();

    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < WIDTH; j++) {
            delete board[i][j];
            board[i][j] = nullptr;
        }
    }
    for (int i = 0; i < BENCHSIZE; i++) {
        delete bench[i];
        bench[i] = nullptr;
    }
    for (int i = 0; i < 5; i++) {
        delete shopSlots[i];
        shopSlots[i] = nullptr;
    }
    for (Item* item : itemBench) {
        delete item;
    }
    itemBench.clear();

    currentState = stateFromString(root["state"].toString());
    currentround = root["round"].toInt(1);
    playerHp = root["playerHp"].toInt(100);
    enemyHp = root["enemyHp"].toInt(100);
    battleResultStr = root["battleResult"].toString("NONE");
    resultDisplayTimer = 0;

    QJsonObject playerObj = root["player"].toObject();
    player->restoreState(
        playerObj["hp"].toInt(100),
        playerObj["gold"].toInt(10),
        playerObj["level"].toInt(3),
        playerObj["exp"].toInt(0),
        playerObj["expToNextLevel"].toInt(4));

    auto restoreUnit = [&](const QJsonObject& obj) -> Unit* {
        Unit* unit = createUnitByName(obj["name"].toString().toStdString(), ownerFromString(obj["owner"].toString()));
        unit->hp = obj["hp"].toInt(unit->hp);
        unit->maxHp = obj["maxHp"].toInt(unit->maxHp);
        unit->atk = obj["atk"].toInt(unit->atk);
        unit->range = obj["range"].toInt(unit->range);
        unit->mana = obj["mana"].toInt(unit->mana);
        unit->maxMana = obj["maxMana"].toInt(unit->maxMana);
        unit->star = obj["star"].toInt(unit->star);
        unit->cost = obj["cost"].toInt(unit->cost);
        unit->state = UnitState::Idle;
        unit->target = nullptr;

        unit->traits.clear();
        for (const QJsonValue& value : obj["traits"].toArray()) {
            unit->traits.push_back(value.toString().toStdString());
        }

        for (const QJsonValue& value : obj["equippedItems"].toArray()) {
            Item* item = createItemByName(value.toObject()["type"].toString().toStdString());
            unit->equippedItems.push_back(item);
        }
        return unit;
    };

    for (const QJsonValue& value : root["units"].toArray()) {
        QJsonObject obj = value.toObject();
        Unit* unit = restoreUnit(obj);
        QString area = obj["area"].toString();
        int x = obj["x"].toInt();
        int y = obj["y"].toInt();

        if (area == "board" && x >= 0 && x < LENGTH && y >= 0 && y < WIDTH) {
            unit->x = x;
            unit->y = y;
            unit->isBench = false;
            board[x][y] = unit;
        } else if (area == "bench" && x >= 0 && x < BENCHSIZE) {
            unit->x = x;
            unit->y = -1;
            unit->isBench = true;
            bench[x] = unit;
        } else {
            delete unit;
        }
    }

    for (const QJsonValue& value : root["itemBench"].toArray()) {
        if ((int)itemBench.size() >= MAX_ITEM_BENCH) break;
        itemBench.push_back(createItemByName(value.toObject()["type"].toString().toStdString()));
    }

    for (const QJsonValue& value : root["shop"].toArray()) {
        QJsonObject obj = value.toObject();
        int x = obj["x"].toInt();
        if (x < 0 || x >= 5 || obj["empty"].toBool(false)) continue;
        Unit* unit = restoreUnit(obj);
        unit->x = -1;
        unit->y = -1;
        unit->isBench = true;
        shopSlots[x] = unit;
    }

    updateActiveTraits();
    return true;
}
