#include <catch2/catch_test_macros.hpp>

#include "combat/CheckResolver.hpp"
#include "combat/InitiativeTracker.hpp"
#include "scenario/StoryNode.hpp"

using namespace dnd::combat;
using namespace dnd::entities;
using dnd::scenario::CheckDefinition;
using dnd::scenario::CheckType;

TEST_CASE("CheckResolver: проверка навыка сверяется с DC", "[combat][check]")
{
    Character c("Герой", CharacterClass::Fighter, 5, AbilityScores(16, 10, 14, 10, 10, 10)); // СИЛ +3, влад. +3
    CheckDefinition check;
    check.type = CheckType::SkillCheck;
    check.ability = Ability::Strength;
    check.dc = 15;

    // Без владения: мод +3
    CHECK(resolveCheck(c, check, 12).success);       // 12+3 == 15
    CHECK(resolveCheck(c, check, 13).success);       // 16 > 15
    CHECK_FALSE(resolveCheck(c, check, 11).success); // 14 < 15
    CHECK(std::string(resolveCheck(c, check, 11).verdict()) == "FAILURE");

    // С владением навыком: +3 +3
    const CheckOutcome prof = resolveCheck(c, check, 9, true);
    CHECK(prof.modifier == 6);
    CHECK(prof.total == 15);
    CHECK(prof.success);
}

TEST_CASE("CheckResolver: Восприятие берёт владение с листа персонажа", "[combat][check]")
{
    Character c("Следопыт", CharacterClass::Ranger, 5, AbilityScores(10, 14, 12, 10, 14, 10)); // МДР +2, влад. +3
    CheckDefinition check;
    check.ability = Ability::Wisdom;
    check.skill = "perception";
    check.dc = 15;

    CHECK(modifierForCheck(c, check) == 2);
    c.setPerceptionProficient(true);
    CHECK(modifierForCheck(c, check) == 5);
    // Ручное значение перекрывает автоопределение
    CHECK(modifierForCheck(c, check, false) == 2);
}

TEST_CASE("CheckResolver: спасбросок учитывает владение класса", "[combat][check]")
{
    Character c("Воин", CharacterClass::Fighter, 5, AbilityScores(16, 10, 14, 10, 10, 10));
    CheckDefinition save;
    save.type = CheckType::SavingThrow;
    save.dc = 12;

    save.ability = Ability::Strength; // +3 мод +3 влад. = 6
    CHECK(resolveCheck(c, save, 6).success);
    CHECK_FALSE(resolveCheck(c, save, 5).success);

    save.ability = Ability::Dexterity; // 0
    CHECK(savingThrowModifier(c, Ability::Dexterity) == 0);
    CHECK_FALSE(resolveCheck(c, save, 11).success);
    CHECK(resolveCheck(c, save, 12).success);
}

TEST_CASE("CheckResolver: доп. бонус, натуральные 20 и 1, форматирование", "[combat][check]")
{
    Character c("Thorin", CharacterClass::Fighter, 1, AbilityScores(10, 16, 10, 10, 10, 10)); // ЛОВ +3
    CheckDefinition save;
    save.type = CheckType::SavingThrow;
    save.ability = Ability::Dexterity;
    save.dc = 15;

    const CheckOutcome o = resolveCheck(c, save, 14, std::nullopt, 1);
    CHECK(o.modifier == 4);
    CHECK(o.total == 18);
    CHECK(o.success);
    CHECK(formatCheckOutcome(o) == "Thorin - DEX Saving Throw: roll 14 +4 = 18 vs DC 15 -> SUCCESS");

    CHECK(resolveCheck(c, save, 20).natural20);
    CHECK(resolveCheck(c, save, 1).natural1);
    CHECK_FALSE(resolveCheck(c, save, 10).natural20);
}

TEST_CASE("CheckResolver: бросок ГСЧ детерминирован сидом", "[combat][check]")
{
    Character c("Герой", CharacterClass::Fighter);
    CheckDefinition check;
    check.dc = 10;

    dnd::utils::Random a(7), b(7);
    const int expected = b.range(1, 20);
    const CheckOutcome o = rollAndResolveCheck(c, check, a);
    CHECK(o.diceRoll == expected);
    CHECK(o.diceRoll >= 1);
    CHECK(o.diceRoll <= 20);
}

namespace
{
    Roster makeRoster()
    {
        Roster r;
        r.add(Character("A", CharacterClass::Fighter, 1, AbilityScores(10, 10, 10, 10, 10, 10)), Side::Player); // 0
        r.add(Character("B", CharacterClass::Fighter, 1, AbilityScores(10, 14, 10, 10, 10, 10)), Side::Player); // 1, +2
        r.add(Character("Orc", CharacterClass::Fighter, 1, AbilityScores(10, 10, 10, 10, 10, 10)), Side::Enemy); // 2
        return r;
    }
} // namespace

TEST_CASE("InitiativeTracker: сортировка и тай-брейк", "[combat][initiative]")
{
    Roster roster = makeRoster();
    InitiativeTracker t;
    t.addFromRoster(roster, 0, 15);
    t.addFromRoster(roster, 1, 15);
    t.addFromRoster(roster, 2, 15);

    REQUIRE(t.size() == 3);
    CHECK(t.combatants()[0].rosterIndex == 1); // больший бонус
    CHECK(t.combatants()[1].rosterIndex == 0); // игрок перед врагом
    CHECK(t.combatants()[2].rosterIndex == 2);

    t.setInitiative(2, 20, &roster); // орк уходит на первое место
    CHECK(t.combatants()[0].rosterIndex == 2);
}

TEST_CASE("InitiativeTracker: загрузка врагов узла и бестиария", "[combat][initiative]")
{
    Roster roster;
    roster.add(Character("Hero", CharacterClass::Fighter), Side::Player);

    dnd::scenario::ScenarioGraph graph;
    dnd::scenario::EnemyTemplate gob;
    gob.id = "gob";
    gob.name = "Goblin";
    gob.armorClass = 13;
    gob.maxHp = 10;
    gob.currentHp = 7;
    gob.initiativeBonus = 2;
    graph.addEnemy(gob);

    dnd::scenario::StoryNode node("n");
    node.enemyIds = {"gob", "gob", "missing"};

    InitiativeTracker t;
    CHECK(t.loadNodeEnemies(roster, graph, node) == 2);
    CHECK(roster.count(Side::Enemy) == 2);
    CHECK(t.size() == 3); // герой + 2 врага
    CHECK(roster.at(1).character.name == "Goblin");
    CHECK(roster.at(2).character.name == "Goblin #2");
    CHECK(roster.at(1).character.armorClass() == 13);
    CHECK(roster.at(1).character.currentHp() == 7);

    // враг в разгар боя
    const std::size_t idx = t.spawnEnemyFromTemplate(roster, gob, nullptr, 18);
    CHECK(roster.at(idx).character.name == "Goblin #3");
    CHECK(t.combatants()[0].rosterIndex == idx);

    t.removeEnemies(roster);
    CHECK(roster.size() == 1);
    CHECK(t.size() == 1);
    CHECK(t.combatants()[0].rosterIndex == 0);
}

TEST_CASE("InitiativeTracker: ходы, раунды и павшие участники", "[combat][initiative]")
{
    Roster roster;
    for (const char *n : {"P1", "P2", "P3"})
        roster.add(Character(n, CharacterClass::Fighter), Side::Player);
    InitiativeTracker t;
    t.addFromRoster(roster, 0, 20);
    t.addFromRoster(roster, 1, 15);
    t.addFromRoster(roster, 2, 10);
    roster.at(1).character.applyDamage(1000);

    t.startCombat(&roster);
    CHECK(t.round() == 1);
    CHECK(t.activeTurnIndex() == 0);

    t.nextTurn(&roster);
    CHECK(t.activeTurnIndex() == 2); // павший P2 пропущен
    CHECK(t.round() == 1);

    t.nextTurn(&roster);
    CHECK(t.activeTurnIndex() == 0);
    CHECK(t.round() == 2);

    t.previousTurn(&roster);
    CHECK(t.activeTurnIndex() == 2);
    CHECK(t.round() == 1);

    // без пропуска павших
    t.setActiveTurnIndex(0);
    t.nextTurn(&roster, false);
    CHECK(t.activeTurnIndex() == 1);

    // смена инициативы сохраняет активного персонажа
    t.setInitiative(1, 5, &roster); // P2 -> последний
    CHECK(t.activeCombatant()->rosterIndex == 1);

    // удаление участника сдвигает индексы
    CHECK(t.removeByRosterIndex(0, true));
    CHECK(t.size() == 2);
}
