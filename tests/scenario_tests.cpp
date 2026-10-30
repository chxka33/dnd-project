#include <catch2/catch_test_macros.hpp>

#include "scenario/CharacterSerializer.hpp"
#include "scenario/ChoiceNode.hpp"
#include "scenario/Node.hpp"
#include "scenario/RandomNode.hpp"
#include "scenario/ScenarioGraph.hpp"
#include "scenario/ScenarioSerializer.hpp"
#include "scenario/SessionSerializer.hpp"
#include "scenario/StoryNode.hpp"
#include "utils/Random.hpp"

using namespace dnd::scenario;
using namespace dnd::entities;
using namespace dnd::utils;

TEST_CASE("Node: расширенные метаданные, испытания и награды", "[scenario][node]")
{
    StoryNode node("tavern", "Таверна");
    node.summary = "Партия отдыхает в таверне";
    node.dmNotes = "Трактирщик знает про логово гоблинов";
    node.enemyIds = {"goblin_1", "goblin_2"};

    CheckDefinition check;
    check.type = CheckType::SavingThrow;
    check.ability = Ability::Dexterity;
    check.dc = 14;
    node.check = check;

    Loot loot;
    loot.gold = 50;
    loot.items.push_back({"Зелье лечения", 2});
    node.loot = loot;

    CHECK(node.summary == "Партия отдыхает в таверне");
    CHECK(node.dmNotes == "Трактирщик знает про логово гоблинов");
    CHECK(node.enemyIds.size() == 2);
    REQUIRE(node.check.has_value());
    CHECK(node.check->type == CheckType::SavingThrow);
    CHECK(node.check->dc == 14);

    // Обратная совместимость через skillCheck
    REQUIRE(node.skillCheck.has_value());
    CHECK(node.skillCheck->dc == 14);

    REQUIRE(node.loot.has_value());
    CHECK(node.loot->gold == 50);
    REQUIRE(node.loot->items.size() == 1);
    CHECK(node.loot->items[0].name == "Зелье лечения");
    CHECK(node.loot->items[0].quantity == 2);
}

TEST_CASE("RandomNode: таблица случайных встреч (Roll Table)", "[scenario][random]")
{
    RandomNode rnd("crossroads", "Перекрёсток");
    rnd.setDiceSides(6);
    CHECK(rnd.diceSides() == 6);

    rnd.addEntry(1, 2, "Засада гоблинов", "ambush");
    rnd.addEntry(3, 4, "Бродячий купец", "merchant");
    rnd.addEntry(5, 6, "Тихая дорога", "peaceful");

    CHECK(rnd.entries().size() == 3);
    auto tgts = rnd.targets();
    CHECK(tgts.size() == 3);

    dnd::utils::Random rng(42);
    auto res = rnd.roll(rng);
    CHECK(res.diceSides == 6);
    CHECK(res.roll >= 1);
    CHECK(res.roll <= 6);
    CHECK(res.triggered);
    CHECK_FALSE(res.next.empty());

    // Замена целевого узла
    std::size_t replaced = rnd.replaceTarget("ambush", "new_ambush");
    CHECK(replaced == 1);
    CHECK(rnd.entries()[0].target == "new_ambush");
}

TEST_CASE("ScenarioGraph: каталог противников (бестиарий)", "[scenario][graph]")
{
    ScenarioGraph graph;
    CHECK(graph.enemies.empty());

    EnemyTemplate goblin;
    goblin.id = "goblin_archer";
    goblin.name = "Гоблин-лучник";
    goblin.armorClass = 13;
    goblin.maxHp = 7;
    goblin.currentHp = 7;
    goblin.initiativeBonus = 2;

    graph.addEnemy(goblin);
    CHECK(graph.enemies.size() == 1);

    const EnemyTemplate *found = graph.findEnemy("goblin_archer");
    REQUIRE(found != nullptr);
    CHECK(found->name == "Гоблин-лучник");
    CHECK(found->armorClass == 13);

    CHECK(graph.removeEnemy("goblin_archer"));
    CHECK(graph.enemies.empty());
    CHECK(graph.findEnemy("goblin_archer") == nullptr);
}

TEST_CASE("ScenarioSerializer: сохранение и загрузка сценария (.dndsc)", "[scenario][serializer]")
{
    ScenarioGraph original;
    original.title = "Поход в пещеры";
    original.description = "Тестовое приключение";

    EnemyTemplate orc;
    orc.id = "orc_warrior";
    orc.name = "Орк-воин";
    orc.armorClass = 13;
    orc.maxHp = 15;
    orc.currentHp = 15;
    orc.initiativeBonus = 1;
    original.addEnemy(orc);

    auto &story = original.emplace<StoryNode>("start", "Вход в пещеру");
    story.summary = "Лагерь у входа";
    story.dmNotes = "Тайный проход за кустом";
    story.enemyIds = {"orc_warrior"};

    CheckDefinition check;
    check.type = CheckType::SkillCheck;
    check.ability = Ability::Wisdom;
    check.dc = 12;
    check.skill = "Perception";
    story.check = check;

    Loot loot;
    loot.gold = 25;
    loot.items.push_back({"Факел", 3});
    story.loot = loot;
    story.setNext("table_node");

    auto &rnd = original.emplace<RandomNode>("table_node", "Пещерная развилка");
    rnd.setDiceSides(20);
    rnd.addEntry(1, 10, "Летучие мыши", "bats");
    rnd.addEntry(11, 20, "Сундук с золотом", "chest");

    original.emplace<StoryNode>("bats", "Стая мышей");
    original.emplace<StoryNode>("chest", "Сокровищница");

    nlohmann::json jsonRep = ScenarioSerializer::toJson(original);
    CHECK(jsonRep["formatVersion"] == ScenarioSerializer::kCurrentFormatVersion);
    CHECK(jsonRep["enemies"].is_array());
    CHECK(jsonRep["enemies"].size() == 1);

    std::string error;
    auto loaded = ScenarioSerializer::fromJson(jsonRep, &error);
    REQUIRE(loaded != nullptr);
    CHECK(error.empty());
    CHECK(loaded->title == "Поход в пещеры");
    REQUIRE(loaded->enemies.size() == 1);
    CHECK(loaded->enemies[0].id == "orc_warrior");

    const Node *loadedStart = loaded->find("start");
    REQUIRE(loadedStart != nullptr);
    CHECK(loadedStart->summary == "Лагерь у входа");
    CHECK(loadedStart->dmNotes == "Тайный проход за кустом");
    REQUIRE(loadedStart->check.has_value());
    CHECK(loadedStart->check->type == CheckType::SkillCheck);
    CHECK(loadedStart->check->skill == "Perception");
    REQUIRE(loadedStart->loot.has_value());
    CHECK(loadedStart->loot->gold == 25);
    CHECK(loadedStart->enemyIds.size() == 1);

    const auto *loadedRnd = dynamic_cast<const RandomNode *>(loaded->find("table_node"));
    REQUIRE(loadedRnd != nullptr);
    CHECK(loadedRnd->diceSides() == 20);
    REQUIRE(loadedRnd->entries().size() == 2);
    CHECK(loadedRnd->entries()[0].label == "Летучие мыши");
}

TEST_CASE("CharacterSerializer: сохранение и загрузка персонажей (.dndch)", "[scenario][character_serializer]")
{
    Character hero("Эльфийка", CharacterClass::Ranger, 3, AbilityScores(10, 16, 14, 12, 16, 8));
    hero.addCondition("Poisoned");
    hero.addCondition("Blessed");
    hero.inventory().add(Item::misc("Лечебная трава", 0.5, 4));

    CHECK(hero.passivePerception() == 13); // 10 + WIS(+3)
    CHECK(hero.hasCondition("Poisoned"));

    nlohmann::json j = CharacterSerializer::toJson(hero);
    std::string error;
    auto loaded = CharacterSerializer::fromJson(j, &error);
    REQUIRE(loaded != nullptr);
    CHECK(error.empty());
    CHECK(loaded->name == "Эльфийка");
    CHECK(loaded->characterClass() == CharacterClass::Ranger);
    CHECK(loaded->level() == 3);
    CHECK(loaded->hasCondition("Poisoned"));
    CHECK(loaded->hasCondition("Blessed"));
    CHECK(loaded->inventory().size() == 1);
    CHECK(loaded->passivePerception() == 13);
}

TEST_CASE("SessionSerializer: сохранение и загрузка сессии (.dndsav)", "[scenario][session_serializer]")
{
    SessionSaveData session;
    session.scenarioFilePath = "campaigns/cave.dndsc";
    session.currentNodeId = "room_5";
    session.combatRound = 2;
    session.activeTurnIndex = 1;
    session.eventLog = {
        "Сессия началась в лагере",
        "Варвар выбил дверь (бросок 18 vs DC 15)",
        "Начался бой с орками"};

    Character fighter("Торин", CharacterClass::Fighter, 2, AbilityScores(16, 12, 14, 8, 12, 10));
    fighter.applyDamage(5);
    fighter.addCondition("Frightened");
    session.roster.add(std::move(fighter), Side::Player);

    nlohmann::json j = SessionSerializer::toJson(session);
    CHECK(j["formatVersion"] == SessionSerializer::kCurrentFormatVersion);
    CHECK(j["currentNodeId"] == "room_5");

    SessionSaveData loadedSession;
    std::string error;
    REQUIRE(SessionSerializer::fromJson(j, loadedSession, &error));
    CHECK(error.empty());
    CHECK(loadedSession.scenarioFilePath == "campaigns/cave.dndsc");
    CHECK(loadedSession.currentNodeId == "room_5");
    CHECK(loadedSession.combatRound == 2);
    CHECK(loadedSession.activeTurnIndex == 1);
    CHECK(loadedSession.eventLog.size() == 3);

    REQUIRE(loadedSession.roster.size() == 1);
    const Character &loadedFighter = loadedSession.roster.at(0).character;
    CHECK(loadedFighter.name == "Торин");
    CHECK(loadedFighter.hasCondition("Frightened"));
    CHECK(loadedFighter.currentHp() < loadedFighter.maxHp());
}
