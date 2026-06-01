#include "gamemanager.h"
#include "advanceditems.h"
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
    board.clear();
    for (int i = 0; i < BENCHSIZE; i++) {
        bench[i] = nullptr;
    }
    for (int i = 0; i < 5; i++) {
        shopSlots[i] = nullptr;
    }

    player = new Player();
    currentState = GameState::Preparation;
    currentround = 1;
    winStreak = 0;
    loseStreak = 0;
    interestGold = 0;
    refreshShop();
}

GameManager::~GameManager() {
    clearAllUnits();
    for (int i = 0; i < 5; i++) {
        delete shopSlots[i];
    }
    for (Item* item : itemBench) {
        delete item;
    }
    delete player;
}

void GameManager::registerUnit(Unit* u) {
    if (!u) return;
    allUnits.push_back(u);
}

void GameManager::unregisterUnit(Unit* u) {
    if (!u) return;
    auto it = std::find(allUnits.begin(), allUnits.end(), u);
    if (it != allUnits.end()) {
        allUnits.erase(it);
    }
}

void GameManager::clearAllUnits() {
    for (Unit* u : allUnits) {
        delete u;
    }
    allUnits.clear();
}

void GameManager::updateUnitPosition(int oldX, int oldY, int newX, int newY) {
    if (!board.isValidPosition(oldX, oldY)) return;
    if (!board.isValidPosition(newX, newY)) return;
    Unit* u = board.getUnitAt(oldX, oldY);
    board.addUnit(newX, newY, u);
    board.removeUnit(oldX, oldY);
}

bool GameManager::canMoveUnit(Unit* target, int nextX, int nextY, bool toBench) {
    if (!target) return false;
    if (nextX < 0 || nextX >= BENCHSIZE) return false;
    if (!toBench && (nextY < 0 || nextY >= BOARD_ROWS)) return false;

    // 玩家单位不可拖拽到敌方半场（0-3 行），准备阶段排兵布阵仅在己方半场（4-7 行）进行。
    if (!toBench && target->owner == Owner::PlayerCtrl && nextY < 4) {
        return false;
    }

    bool oldIsBench = target->isBench;
    Unit* existingUnit = toBench ? bench[nextX] : board.getUnitAt(nextX, nextY);

    if (existingUnit == nullptr) {
        if (!toBench && oldIsBench && !Checkpopulation()) {
            return false;
        }
        return true;
    }
    if (existingUnit->owner == target->owner) {
        return true;
    }
    return false;
}

bool GameManager::MoveUnit(Unit* target, int nextX, int nextY, bool toBench) {
    if (!canMoveUnit(target, nextX, nextY, toBench)) return false;

    int oldX = target->x;
    int oldY = target->y;
    bool oldIsBench = target->isBench;
    Unit* existingUnit = toBench ? bench[nextX] : board.getUnitAt(nextX, nextY);

    if (existingUnit == nullptr) {
        if (oldIsBench) {
            bench[oldX] = nullptr;
        } else {
            board.removeUnit(oldX, oldY);
        }

        if (toBench) {
            bench[nextX] = target;
            target->x = nextX;
            target->y = -1;
            target->isBench = true;
        } else {
            board.addUnit(nextX, nextY, target);
            target->x = nextX;
            target->y = nextY;
            target->isBench = false;
        }
    } else {
        if (oldIsBench) {
            bench[oldX] = existingUnit;
        } else {
            board.addUnit(oldX, oldY, existingUnit);
        }
        existingUnit->x = oldX;
        existingUnit->y = oldIsBench ? -1 : oldY;
        existingUnit->isBench = oldIsBench;

        if (toBench) {
            bench[nextX] = target;
        } else {
            board.addUnit(nextX, nextY, target);
        }
        target->x = nextX;
        target->y = toBench ? -1 : nextY;
        target->isBench = toBench;
    }

    updateActiveTraits();
    return true;
}

Unit* GameManager::getUnitOnBench(int index) {
    if (index < 0 || index >= BENCHSIZE) return nullptr;
    return bench[index];
}

Unit* GameManager::getUnitOnBoard(int x, int y) {
    return board.getUnitAt(x, y);
}

bool GameManager::Checkpopulation() {
    int deployed = 0;
    for (Unit* u : allUnits) {
        if (u && !u->isBench && u->owner == Owner::PlayerCtrl) {
            deployed++;
        }
    }
    return deployed < player->getPopulationCap();
}

void GameManager::RemoveUnit(Unit* targetUnit) {
    if (!targetUnit) return;
    if (targetUnit->isBench) {
        bench[targetUnit->x] = nullptr;
    } else {
        board.removeUnit(targetUnit->x, targetUnit->y);
    }
    unregisterUnit(targetUnit);
    delete targetUnit;
    updateActiveTraits();
}

void GameManager::spawnEnemyRound(int round) {
    int enemyCount = std::min(2 + round / 2, 6);
    int bonusHp = (round - 1) * 35;
    int bonusAtk = (round - 1) * 5;

    for (int i = 0; i < enemyCount; i++) {
        int x = i % BOARD_COLS;
        int y = i / BOARD_COLS;
        if (y >= 2 || board.getUnitAt(x, y) != nullptr) {
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
        board.addUnit(x, y, enemy);
        registerUnit(enemy);
    }
}

void GameManager::startBattle() {
    if (currentState != GameState::Preparation) return;

projectiles.clear();
    skillEffects.clear();

    updateActiveTraits();
    int mageCount = activeTraitsCount["Mage"];
    int vanguardCount = activeTraitsCount["Vanguard"];
    int knightCount = activeTraitsCount["Knight"];
    int healerCount = activeTraitsCount["Healer"];

    for (Unit* u : allUnits) {
        if (!u || u->owner != Owner::PlayerCtrl || u->isBench) continue;
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

        if (knightCount >= 2 &&
            std::find(u->traits.begin(), u->traits.end(), "Knight") != u->traits.end()) {
            u->maxHp += 100;
            u->hp = u->maxHp;
            u->atk += 15;
        }

        if (healerCount >= 2) {
            u->maxHp += 80;
            u->hp = u->maxHp;
            u->atk += 5;
        }
    }

    for (int x = 0; x < BOARD_COLS; x++) {
        for (int y = 0; y < 2; y++) {
            Unit* u = board.getUnitAt(x, y);
            if (u && u->owner == Owner::EnemyCtrl) {
                board.removeUnit(x, y);
                unregisterUnit(u);
                delete u;
            }
        }
    }

    spawnEnemyRound(currentround);

    // 触发高级装备的 BattleStart 被动（如大天使之杖初始法力）
    triggerItemCallbacks_BattleStart();

    currentState = GameState::Battle;
    battleFrame = 0;
}

void GameManager::updateTick() {
    if (resultDisplayTimer > 0) {
        resultDisplayTimer--;
        if (resultDisplayTimer == 0 && currentState == GameState::Settlement) {
            currentState = GameState::Preparation;
            refreshShop();
        }
    }

    if (currentState != GameState::Battle) {
        if (!projectiles.empty()) {
            projectiles.clear();
        }
        if (!skillEffects.empty()) {
            skillEffects.clear();
        }
        return;
    }

    for (Unit* u : allUnits) {
        if (u && !u->isBench) {
            u->updateAction(this);
        }
    }

    battleFrame++;
    triggerItemCallbacks_Tick();

    updateProjectiles();

    for (auto& e : skillEffects) { e.timer--; }
    skillEffects.erase(
        std::remove_if(skillEffects.begin(), skillEffects.end(),
            [](const SkillEffect& e) { return e.timer <= 0; }),
        skillEffects.end());

    cleanupDeadUnits();
    checkBattleResult();
}

void GameManager::addProjectile(int fromX, int fromY, int toX, int toY, bool isSkill) {
    Projectile p;
    p.fromX = fromX;
    p.fromY = fromY;
    p.toX = toX;
    p.toY = toY;
    p.isSkill = isSkill;
    p.totalFrames = isSkill ? 16 : 10;
    projectiles.push_back(p);
}

void GameManager::addSkillEffect(int x, int y, const std::string& skillName, int duration) {
    skillEffects.push_back({x, y, skillName, duration});
}

void GameManager::updateProjectiles() {
    for (auto& p : projectiles) {
        if (p.showHit) {
            p.hitTimer--;
            continue;
        }
        p.currentFrame++;
        p.progress = (float)p.currentFrame / p.totalFrames;
        if (p.currentFrame >= p.totalFrames) {
            p.showHit = true;
            p.hitTimer = 8;
        }
    }
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
            [](const Projectile& p) { return p.showHit && p.hitTimer <= 0; }),
        projectiles.end());
}

void GameManager::checkBattleResult() {
    if (currentState != GameState::Battle) return;

    bool playerAlive = false;
    bool enemyAlive = false;
    for (Unit* u : allUnits) {
        if (!u || u->isBench || !u->isAlive()) continue;
        if (u->owner == Owner::PlayerCtrl) {
            playerAlive = true;
        } else {
            enemyAlive = true;
        }
    }

    if (playerAlive && enemyAlive) return;

    currentState = GameState::Settlement;
    resultDisplayTimer = 60;
    settlementBreakdown.clear();

    int baseGold = 0;
    int streakBonus = 0;

    if (!playerAlive && !enemyAlive) {
        battleResultStr = "DRAW";
        baseGold = 2;
        player->addGold(baseGold);
        currentround++;
        winStreak = 0;
        loseStreak = 0;
    } else if (playerAlive) {
        battleResultStr = "VICTORY";
        baseGold = 6;
        player->addGold(baseGold);
        currentround++;
        winStreak++;
        loseStreak = 0;
    } else {
        battleResultStr = "DEFEAT";
        baseGold = 5;
        playerHp = std::max(0, playerHp - 10);
        player->addGold(baseGold);
        loseStreak++;
        winStreak = 0;
    }

    // ==================== 阶段四：高级经济系统 ====================
    // 利息：每 10 金币额外获得 1 金币，最多 5 金币。
    int interest = std::min(5, player->getGold() / 10);
    interestGold = interest;
    player->addGold(interest);

    // 连胜连败奖励：3 连击起每轮额外 1 金币。
    if (winStreak >= 3) {
        int bonus = std::min(winStreak - 2, 5);
        streakBonus = bonus;
        player->addGold(bonus);
    }
    if (loseStreak >= 3) {
        int bonus = std::min(loseStreak - 2, 5);
        streakBonus = bonus;
        player->addGold(bonus);
    }

    // 组装结算明细
    settlementBreakdown = QString("基础奖励: +%1G\n利息: +%2G").arg(baseGold).arg(interest);
    if (streakBonus > 0) {
        settlementBreakdown += QString("\n%1%2奖励: +%3G")
            .arg(winStreak >= 3 ? "连胜" : "连败")
            .arg(std::max(winStreak, loseStreak))
            .arg(streakBonus);
    }
    settlementBreakdown += "\n经验: +1XP";

    std::vector<Unit*> toDelete;
    for (Unit* u : allUnits) {
        if (!u || u->isBench) continue;
        if (u->owner == Owner::EnemyCtrl) {
            board.removeUnit(u->x, u->y);
            toDelete.push_back(u);
        } else if (u->owner == Owner::PlayerCtrl && u->isAlive()) {
            u->maxHp = u->baseMaxHp;
            u->atk = u->baseAtk;
            u->maxMana = u->baseMaxMana;
            for (Item* item : u->equippedItems) {
                u->atk += item->bonusAtk;
                u->maxHp += item->bonusHp;
                if (u->maxMana > item->manaReduction) {
                    u->maxMana -= item->manaReduction;
                } else {
                    u->maxMana = 10;
                }
            }
            u->hp = u->maxHp;
            u->mana = 0;
            u->state = UnitState::Idle;
            u->target = nullptr;
        }
    }

    for (Unit* u : toDelete) {
        unregisterUnit(u);
        delete u;
    }

    // 战斗结束后，存活玩家单位自动归位到己方半场（4-7 行），从左到右、从上到下排列。
    std::vector<Unit*> surviving;
    for (Unit* u : allUnits) {
        if (!u || u->isBench || u->owner != Owner::PlayerCtrl || !u->isAlive()) continue;
        surviving.push_back(u);
        board.removeUnit(u->x, u->y);
    }
    int slotIdx = 0;
    for (int y = 4; y < BOARD_ROWS && slotIdx < (int)surviving.size(); y++) {
        for (int x = 0; x < BOARD_COLS && slotIdx < (int)surviving.size(); x++) {
            if (board.getUnitAt(x, y) == nullptr) {
                Unit* u = surviving[slotIdx++];
                board.addUnit(x, y, u);
                u->x = x;
                u->y = y;
                u->isBench = false;
            }
        }
    }

    projectiles.clear();

    player->addXP(1);
    updateActiveTraits();
}

void GameManager::cleanupDeadUnits() {
    if (currentState != GameState::Battle) return;

    std::vector<Unit*> toDelete;
    for (Unit* u : allUnits) {
        if (!u || u->isBench || (u->isAlive() && u->state != UnitState::Dead)) {
            continue;
        }

        // 检查高级装备复活钩子（如复活甲）
        if (triggerItemCallbacks_Death(u)) {
            u->hp = u->maxHp;
            u->state = UnitState::Idle;
            u->target = nullptr;
            continue;  // 复活成功，跳过删除
        }

        if (u->owner == Owner::EnemyCtrl && (rand() % 100) < 30) {
            Item* droppedItem = new Item(static_cast<ItemType>(rand() % 4));
            if ((int)itemBench.size() < MAX_ITEM_BENCH) {
                itemBench.push_back(droppedItem);
            } else {
                delete droppedItem;
            }
        }

        board.removeUnit(u->x, u->y);
        toDelete.push_back(u);
    }

    for (Unit* u : toDelete) {
        // 清除其他单位持有的悬垂 target 指针，防止下一帧 use-after-free 崩溃。
        for (Unit* other : allUnits) {
            if (other && other->target == u) {
                other->target = nullptr;
                other->state = UnitState::Idle;
            }
        }
        unregisterUnit(u);
        delete u;
    }
}

void GameManager::refreshShop() {
    for (int i = 0; i < 5; i++) {
        delete shopSlots[i];
        shopSlots[i] = nullptr;

        int control = rand() % 6;
        if (control == 0) {
            shopSlots[i] = new Garen(Owner::PlayerCtrl);
        } else if (control == 1) {
            shopSlots[i] = new Ryze(Owner::PlayerCtrl);
        } else if (control == 2) {
            shopSlots[i] = new Soraka(Owner::PlayerCtrl);
        } else if (control == 3) {
            shopSlots[i] = new Leona(Owner::PlayerCtrl);
        } else if (control == 4) {
            shopSlots[i] = new Ashe(Owner::PlayerCtrl);
        } else {
            shopSlots[i] = new Jhin(Owner::PlayerCtrl);
        }
    }
}

void GameManager::buyXP() {
    if (currentState != GameState::Preparation) return;
    if (player->spendGold(4)) {
        player->addXP(4);
    }
}

int GameManager::getpoplulation() {
    return player->getPopulationCap();
}

void GameManager::refreshShopManual() {
    if (currentState != GameState::Preparation) return;
    if (player->spendGold(2)) {
        refreshShop();
    }
}

bool GameManager::buyHeroFromShop(int shopIndex) {
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

    registerUnit(hero);

    checkAndCombineStars();
    updateActiveTraits();
    return true;
}

void GameManager::checkAndCombineStars() {
    std::vector<std::string> heroNames = {"Garen", "Ryze", "Soraka", "Leona", "Ashe", "Jhin"};

    for (const std::string& name : heroNames) {
        for (int s = 1; s <= 2; s++) {
            std::vector<Unit*> matches;

            for (Unit* u : allUnits) {
                if (u && u->owner == Owner::PlayerCtrl &&
                    u->name == name && u->star == s) {
                    matches.push_back(u);
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
                        board.removeUnit(target->x, target->y);
                    }
                    unregisterUnit(target);
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
    activeTraitsCount.clear();
    std::map<std::string, std::set<Unit*>> traitToHeroUnits;

    for (Unit* u : allUnits) {
        if (!u || !u->isAlive() || u->owner != Owner::PlayerCtrl || u->isBench) continue;
        for (const std::string& trait : u->traits) {
            traitToHeroUnits[trait].insert(u);
        }
    }

    for (auto const& [trait, heroesSet] : traitToHeroUnits) {
        activeTraitsCount[trait] = (int)heroesSet.size();
    }
}

Unit* GameManager::createUnitByName(const std::string& name, Owner owner) {
    if (name == "Garen") return new Garen(owner);
    if (name == "Ryze") return new Ryze(owner);
    if (name == "Soraka") return new Soraka(owner);
    if (name == "Leona") return new Leona(owner);
    if (name == "Ashe") return new Ashe(owner);
    if (name == "Jhin") return new Jhin(owner);

    Unit* enemy = new Unit(120, 18, 1, 0, owner);
    enemy->name = name.empty() ? "Enemy" : name;
    if (enemy->traits.empty()) {
        enemy->traits.push_back("Monster");
    }
    return enemy;
}

Item* GameManager::createItemByName(const std::string& name) {
    if (name == "Sword") return new Item(ItemType::Sword);
    if (name == "Armor") return new Item(ItemType::Armor);
    if (name == "Glove") return new Item(ItemType::Glove);
    if (name == "Crystal") return new Item(ItemType::Crystal);
    // 高级合成装备
    if (name == "ReviveArmor") return new ReviveArmor();
    if (name == "InfinityEdge") return new InfinityEdge();
    if (name == "ArchangelStaff") return new ArchangelStaff();
    if (name == "ThornmailArmor") return new ThornmailArmor();
    if (name == "WarmogArmor") return new WarmogArmor();
    if (name == "LudenEcho") return new LudenEcho();
    return new Item(ItemType::Sword);
}

static QString ownerToString(Owner owner) {
    return owner == Owner::PlayerCtrl ? "Player" : "Enemy";
}

static Owner ownerFromString(const QString& owner) {
    return owner == "Enemy" ? Owner::EnemyCtrl : Owner::PlayerCtrl;
}

static QString stateToString(GameState state) {
    if (state == GameState::Battle) return "Battle";
    if (state == GameState::Settlement) return "Settlement";
    return "Preparation";
}

static GameState stateFromString(const QString& state) {
    if (state == "Battle") return GameState::Battle;
    if (state == "Settlement") return GameState::Settlement;
    return GameState::Preparation;
}

static QString itemTypeToString(ItemType type) {
    switch (type) {
    case ItemType::Sword: return "Sword";
    case ItemType::Armor: return "Armor";
    case ItemType::Glove: return "Glove";
    case ItemType::Crystal: return "Crystal";
    case ItemType::Advanced: return "Advanced";
    }
    return "Sword";
}

static QJsonObject itemToJson(Item* item) {
    QJsonObject obj;
    obj["type"] = itemTypeToString(item->type);
    // 高级装备额外保存名称，用于读档时重建正确的子类
    if (item->type == ItemType::Advanced) {
        obj["name"] = QString::fromStdString(item->name);
    }
    return obj;
}

static QJsonObject unitToJson(Unit* unit, int slotX, int slotY, const QString& area) {
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
    QJsonObject root;
    root["version"] = 1;
    root["state"] = stateToString(currentState);
    root["round"] = currentround;
    root["playerHp"] = playerHp;
    root["enemyHp"] = enemyHp;
    root["battleResult"] = battleResultStr;
    root["winStreak"] = winStreak;
    root["loseStreak"] = loseStreak;

    QJsonObject playerObj;
    playerObj["hp"] = player->getHp();
    playerObj["gold"] = player->getGold();
    playerObj["level"] = player->getLevel();
    playerObj["exp"] = player->getExp();
    playerObj["expToNextLevel"] = player->getExpToNextLevel();
    root["player"] = playerObj;

    QJsonArray units;
    for (int x = 0; x < BOARD_COLS; x++) {
        for (int y = 0; y < BOARD_ROWS; y++) {
            Unit* u = board.getUnitAt(x, y);
            if (u) {
                units.append(unitToJson(u, x, y, "board"));
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

    clearAllUnits();
    board.clear();
    for (int i = 0; i < BENCHSIZE; i++) {
        bench[i] = nullptr;
    }
    for (int i = 0; i < 5; i++) {
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
    winStreak = root["winStreak"].toInt(0);
    loseStreak = root["loseStreak"].toInt(0);
    interestGold = 0;
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
            QJsonObject itemObj = value.toObject();
            QString typeStr = itemObj["type"].toString();
            // 高级装备从 "name" 字段获取具体子类名，基础装备用 "type" 字段
            QString lookupKey = (typeStr == "Advanced" && itemObj.contains("name"))
                                ? itemObj["name"].toString() : typeStr;
            Item* item = createItemByName(lookupKey.toStdString());
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

        if (area == "board" && board.isValidPosition(x, y)) {
            unit->x = x;
            unit->y = y;
            unit->isBench = false;
            board.addUnit(x, y, unit);
            registerUnit(unit);
        } else if (area == "bench" && x >= 0 && x < BENCHSIZE) {
            unit->x = x;
            unit->y = -1;
            unit->isBench = true;
            bench[x] = unit;
            registerUnit(unit);
        } else {
            delete unit;
        }
    }

    for (const QJsonValue& value : root["itemBench"].toArray()) {
        if ((int)itemBench.size() >= MAX_ITEM_BENCH) break;
        QJsonObject itemObj = value.toObject();
        QString typeStr = itemObj["type"].toString();
        QString lookupKey = (typeStr == "Advanced" && itemObj.contains("name"))
                            ? itemObj["name"].toString() : typeStr;
        itemBench.push_back(createItemByName(lookupKey.toStdString()));
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

// ==================== 高级装备钩子实现 ====================

// 战斗开始时，遍历我方上阵单位的装备，触发 onBattleStart
void GameManager::triggerItemCallbacks_BattleStart() {
    for (Unit* u : allUnits) {
        if (!u || u->isBench || u->owner != Owner::PlayerCtrl) continue;
        for (Item* item : u->equippedItems) {
            AdvancedItem* adv = dynamic_cast<AdvancedItem*>(item);
            if (adv) adv->onBattleStart(u, this);
        }
    }
}

// 每帧遍历所有存活单位装备，触发 onTick（传入当前 battleFrame）
void GameManager::triggerItemCallbacks_Tick() {
    for (Unit* u : allUnits) {
        if (!u || u->isBench || !u->isAlive()) continue;
        for (Item* item : u->equippedItems) {
            AdvancedItem* adv = dynamic_cast<AdvancedItem*>(item);
            if (adv) adv->onTick(u, this, battleFrame);
        }
    }
}

// 单位死亡时检查装备复活钩子（如复活甲），返回 true 表示复活成功
bool GameManager::triggerItemCallbacks_Death(Unit* u) {
    if (!u) return false;
    bool revived = false;
    for (Item* item : u->equippedItems) {
        AdvancedItem* adv = dynamic_cast<AdvancedItem*>(item);
        if (adv) adv->onDeath(u, this, revived);
        if (revived) break;
    }
    return revived;
}
