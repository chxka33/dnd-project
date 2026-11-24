#include <catch2/catch_test_macros.hpp>

#include "combat/InitiativeTracker.hpp"
#include "scenario/ChoiceNode.hpp"
#include "scenario/RandomNode.hpp"
#include "scenario/ScenarioSerializer.hpp"
#include "scenario/StoryNode.hpp"
#include "simulator/SessionRunner.hpp"

#include <filesystem>

using namespace dnd::simulator;
using namespace dnd::scenario;
using namespace dnd::entities;

namespace
{
    /// intro(Story) -> fork(Choice) -> {cave(Story), table(Random) -> {bats, chest}}
    ScenarioGraph makeGraph()
    {
        ScenarioGraph g;
        g.title = "Test adventure";

        auto &intro = g.emplace<StoryNode>("intro", "Intro");
        intro.setNext("fork");
        Loot loot;
        loot.gold = 30;
        loot.items.push_back({"Potion of Healing", 2});
        intro.loot = loot;

        auto &fork = g.emplace<ChoiceNode>("fork", "Fork");
        fork.addChoice("Enter the cave", "cave");
        fork.addChoice("Take the road", "table");
        fork.addChoice("Nowhere", "");

        g.emplace<StoryNode>("cave", "Cave"); // финал

        auto &table = g.emplace<RandomNode>("table", "Crossroads");
        table.setDiceSides(6);
        table.addEntry(1, 6, "Anything", "chest");

        g.emplace<StoryNode>("chest", "Chest");
        g.emplace<StoryNode>("bats", "Bats");
        return g;
    }

    std::filesystem::path tempFile(const char *name)
    {
        return std::filesystem::temp_directory_path() / name;
    }
} // namespace

TEST_CASE("SessionRunner: загрузка сценария и старт с начального узла", "[simulator]")
{
    SessionRunner runner;
    CHECK_FALSE(runner.hasScenario());
    CHECK_FALSE(runner.isAtEnd());

    runner.setScenario(makeGraph());
    CHECK(runner.hasScenario());
    CHECK(runner.currentNodeId() == "intro");
    REQUIRE(runner.currentNode() != nullptr);
    CHECK(runner.currentNode()->title == "Intro");
    REQUIRE_FALSE(runner.eventLog().empty());
    CHECK(runner.eventLog().back() == "Entered scene: Intro");

    // Загрузка из файла .dndsc
    const auto path = tempFile("dnd_sim_test.dndsc");
    std::string error;
    REQUIRE(ScenarioSerializer::saveToFile(makeGraph(), path, &error));
    SessionRunner fromFile;
    REQUIRE(fromFile.loadScenario(path, &error));
    CHECK(fromFile.currentNodeId() == "intro");
    CHECK(fromFile.scenarioPath() == path.string());
    CHECK_FALSE(fromFile.loadScenario(tempFile("dnd_sim_missing.dndsc"), &error));
    std::filesystem::remove(path);
}

TEST_CASE("SessionRunner: линейный переход по Story и финал", "[simulator]")
{
    SessionRunner runner;
    runner.setScenario(makeGraph());

    CHECK_FALSE(runner.isAtEnd());
    REQUIRE(runner.advanceStory());
    CHECK(runner.currentNodeId() == "fork");
    CHECK_FALSE(runner.advanceStory()); // развилка — не Story

    runner.moveToNode("cave");
    CHECK(runner.isAtEnd());
    CHECK_FALSE(runner.advanceStory());

    CHECK(runner.restartScenario());
    CHECK(runner.currentNodeId() == "intro");
}

TEST_CASE("SessionRunner: выбор на развилке и принудительный переход", "[simulator]")
{
    SessionRunner runner;
    runner.setScenario(makeGraph());
    runner.moveToNode("fork");

    CHECK_FALSE(runner.chooseOption(7)); // нет такого варианта
    CHECK_FALSE(runner.chooseOption(2)); // вариант никуда не ведёт
    CHECK(runner.currentNodeId() == "fork");

    REQUIRE(runner.chooseOption(0));
    CHECK(runner.currentNodeId() == "cave");
    CHECK(runner.eventLog().size() >= 2);

    CHECK_FALSE(runner.moveToNode("missing"));
    CHECK(runner.currentNodeId() == "cave");
    REQUIRE(runner.moveToNode("bats", "DM override"));
    CHECK(runner.currentNodeId() == "bats");
    CHECK(runner.eventLog().back() == "Entered scene: Bats (DM override)");
}

TEST_CASE("SessionRunner: бросок по таблице случайных встреч", "[simulator]")
{
    SessionRunner runner;
    runner.setScenario(makeGraph());
    dnd::utils::Random rng(5);

    CHECK_FALSE(runner.rollRandomEncounter(rng).has_value()); // intro — не таблица

    runner.moveToNode("table");
    const auto manual = runner.rollRandomEncounter(rng, false);
    REQUIRE(manual.has_value());
    CHECK(manual->roll >= 1);
    CHECK(manual->roll <= 6);
    CHECK(runner.currentNodeId() == "table"); // без автоперехода
    REQUIRE(runner.lastRollResult().has_value());
    CHECK(runner.lastRollResult()->next == "chest");

    REQUIRE(runner.followLastRoll());
    CHECK(runner.currentNodeId() == "chest");
    CHECK_FALSE(runner.lastRollResult().has_value());

    runner.moveToNode("table");
    REQUIRE(runner.rollRandomEncounter(rng, true).has_value());
    CHECK(runner.currentNodeId() == "chest");
}

TEST_CASE("SessionRunner: выдача награды узла герою", "[simulator]")
{
    SessionRunner runner;
    runner.setScenario(makeGraph());

    Roster roster;
    roster.add(Character("Hero", CharacterClass::Fighter), Side::Player);

    const LootResult got = runner.giveCurrentNodeLoot(roster, 0);
    CHECK(got.gold == 30);
    CHECK(got.itemUnits == 2);
    CHECK(roster.at(0).character.inventory().gold() == 30);
    CHECK(runner.eventLog().back() == "Loot given to Hero: 30 gp, 2 item(s).");

    CHECK(runner.giveCurrentNodeLoot(roster, 5).gold == 0); // нет героя
    runner.moveToNode("fork");
    CHECK(runner.giveCurrentNodeLoot(roster, 0).gold == 0); // нет награды
    CHECK(roster.at(0).character.inventory().gold() == 30);
}

TEST_CASE("SessionRunner: сохранение и загрузка сессии (.dndsav)", "[simulator]")
{
    const auto scenarioPath = tempFile("dnd_sim_session.dndsc");
    const auto savePath = tempFile("dnd_sim_session.dndsav");
    REQUIRE(ScenarioSerializer::saveToFile(makeGraph(), scenarioPath));

    SessionRunner runner;
    REQUIRE(runner.loadScenario(scenarioPath));
    runner.moveToNode("fork");
    runner.logEvent("Custom DM note");

    Roster roster;
    roster.add(Character("Thorin", CharacterClass::Fighter, 2), Side::Player);
    roster.add(Character("Orc", CharacterClass::Barbarian), Side::Enemy);
    roster.at(0).character.applyDamage(4);
    roster.at(0).character.addCondition("Poisoned");
    roster.at(0).character.inventory().setGold(77);
    const int hpBefore = roster.at(0).character.currentHp();

    dnd::combat::InitiativeTracker tracker;
    tracker.addFromRoster(roster, 0, 15);
    tracker.addFromRoster(roster, 1, 12);
    tracker.startCombat(&roster);
    tracker.nextTurn(&roster);
    tracker.nextTurn(&roster); // раунд 2
    const int round = tracker.round();
    const int turn = tracker.activeTurnIndex();
    REQUIRE(round == 2);

    std::string error;
    REQUIRE(runner.saveSessionToFile(savePath, roster, tracker, &error));

    SessionRunner restored;
    Roster restoredRoster;
    dnd::combat::InitiativeTracker restoredTracker;
    REQUIRE(restored.loadSessionFromFile(savePath, restoredRoster, restoredTracker, &error));

    CHECK(restored.currentNodeId() == "fork");
    CHECK(restored.hasScenario());
    CHECK(restored.eventLog() == runner.eventLog());
    REQUIRE(restoredRoster.size() == 2);
    CHECK(restoredRoster.at(0).character.currentHp() == hpBefore);
    CHECK(restoredRoster.at(0).character.hasCondition("Poisoned"));
    CHECK(restoredRoster.at(0).character.inventory().gold() == 77);
    CHECK(restoredRoster.at(1).side == Side::Enemy);
    CHECK(restoredTracker.size() == 2);
    CHECK(restoredTracker.round() == round);
    CHECK(restoredTracker.activeTurnIndex() == turn);

    // exportSaveData отражает состояние напрямую
    const SessionSaveData data = restored.exportSaveData(restoredRoster, restoredTracker);
    CHECK(data.currentNodeId == "fork");
    CHECK(data.combatRound == round);

    std::filesystem::remove(scenarioPath);
    std::filesystem::remove(savePath);
}
