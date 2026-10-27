#include <catch2/catch_test_macros.hpp>

#include "entities/Character.hpp"
#include "entities/ItemPresets.hpp"
#include "entities/Roster.hpp"

using namespace dnd::entities;

TEST_CASE("Модификаторы характеристик округляются вниз", "[entities][abilities]")
{
    CHECK(AbilityScores::modifierForScore(1) == -5);
    CHECK(AbilityScores::modifierForScore(7) == -2);
    CHECK(AbilityScores::modifierForScore(8) == -1);
    CHECK(AbilityScores::modifierForScore(9) == -1);
    CHECK(AbilityScores::modifierForScore(10) == 0);
    CHECK(AbilityScores::modifierForScore(11) == 0);
    CHECK(AbilityScores::modifierForScore(12) == 1);
    CHECK(AbilityScores::modifierForScore(15) == 2);
    CHECK(AbilityScores::modifierForScore(20) == 5);
    CHECK(AbilityScores::modifierForScore(30) == 10);
}

TEST_CASE("Значения характеристик ограничены диапазоном 1..30", "[entities][abilities]")
{
    AbilityScores a;
    a.setScore(Ability::Strength, 99);
    CHECK(a.score(Ability::Strength) == 30);
    a.setScore(Ability::Strength, -4);
    CHECK(a.score(Ability::Strength) == 1);
    a.addScore(Ability::Dexterity, 2);
    CHECK(a.score(Ability::Dexterity) == 12);
}

TEST_CASE("Бонус владения растёт с уровнем", "[entities][character]")
{
    CHECK(Character::proficiencyBonusForLevel(1) == 2);
    CHECK(Character::proficiencyBonusForLevel(4) == 2);
    CHECK(Character::proficiencyBonusForLevel(5) == 3);
    CHECK(Character::proficiencyBonusForLevel(9) == 4);
    CHECK(Character::proficiencyBonusForLevel(13) == 5);
    CHECK(Character::proficiencyBonusForLevel(17) == 6);
    CHECK(Character::proficiencyBonusForLevel(20) == 6);
}

TEST_CASE("Максимум HP считается от кости хитов и Телосложения", "[entities][hp]")
{
    // Воин (d10), ТЕЛ 14 (+2): 1 ур. = 10+2, далее по 6+2.
    Character fighter("Бьорн", CharacterClass::Fighter, 1, AbilityScores(16, 12, 14, 10, 10, 10));
    CHECK(fighter.maxHp() == 12);
    CHECK(fighter.currentHp() == 12);
    fighter.setLevel(3);
    CHECK(fighter.maxHp() == 12 + 2 * 8);

    // Максимум пересчитывается при изменении ТЕЛ.
    fighter.abilities().setScore(Ability::Constitution, 16); // +3
    CHECK(fighter.maxHp() == 13 + 2 * 9);

    // Волшебник (d6) с ТЕЛ 1 (-5): не менее 1 HP за уровень.
    Character weak("Слабак", CharacterClass::Wizard, 3, AbilityScores(10, 10, 1, 10, 10, 10));
    CHECK(weak.maxHp() == 3);
}

TEST_CASE("Урон, лечение и временные HP", "[entities][hp]")
{
    Character c("Тест", CharacterClass::Fighter, 1, AbilityScores(10, 10, 10, 10, 10, 10)); // 10 HP
    REQUIRE(c.maxHp() == 10);

    CHECK(c.applyDamage(3) == 3);
    CHECK(c.currentHp() == 7);

    c.grantTemporaryHp(5);
    c.grantTemporaryHp(3); // не суммируется и не уменьшает
    CHECK(c.temporaryHp() == 5);
    CHECK(c.applyDamage(8) == 3); // 5 поглощено временными, 3 — обычными
    CHECK(c.temporaryHp() == 0);
    CHECK(c.currentHp() == 4);

    CHECK(c.applyDamage(100) == 4);
    CHECK(c.currentHp() == 0);
    CHECK(c.isDown());

    CHECK(c.heal(6) == 6);
    CHECK(c.heal(100) == 4);
    CHECK(c.currentHp() == 10);
    CHECK_FALSE(c.isDown());

    CHECK(c.applyDamage(-5) == 0);
    CHECK(c.heal(-5) == 0);
    CHECK(c.currentHp() == 10);
}

TEST_CASE("Класс Доспеха", "[entities][ac]")
{
    const AbilityScores scores(10, 16, 14, 10, 14, 10); // ЛОВ +3, ТЕЛ +2, МДР +2

    SECTION("Без доспехов: 10 + ЛОВ")
    {
        Character c("Плут", CharacterClass::Rogue, 1, scores);
        CHECK(c.armorClass() == 13);
    }
    SECTION("Лёгкий доспех: база + ЛОВ")
    {
        Character c("Плут", CharacterClass::Rogue, 1, scores);
        c.inventory().add(Item::makeArmor("Кожаный", {ArmorCategory::Light, 11}));
        c.inventory().equip(0);
        CHECK(c.armorClass() == 14);
    }
    SECTION("Средний доспех: ЛОВ максимум +2")
    {
        Character c("Воин", CharacterClass::Fighter, 1, scores);
        c.inventory().add(Item::makeArmor("Полукираса", {ArmorCategory::Medium, 15}));
        c.inventory().equip(0);
        CHECK(c.armorClass() == 17);
    }
    SECTION("Тяжёлый доспех: ЛОВ не учитывается, щит добавляет +2")
    {
        Character c("Воин", CharacterClass::Fighter, 1, scores);
        c.inventory().add(Item::makeArmor("Латы", {ArmorCategory::Heavy, 18}));
        c.inventory().add(Item::makeShield("Щит"));
        c.inventory().equip(0);
        CHECK(c.armorClass() == 18);
        c.inventory().equip(1);
        CHECK(c.armorClass() == 20);
        c.inventory().unequip(1);
        CHECK(c.armorClass() == 18);
    }
    SECTION("Отрицательный ЛОВ снижает КД в среднем доспехе, но не в тяжёлом")
    {
        const AbilityScores clumsy(10, 6, 10, 10, 10, 10); // ЛОВ -2
        Character medium("М", CharacterClass::Fighter, 1, clumsy);
        medium.inventory().add(Item::makeArmor("Полукираса", {ArmorCategory::Medium, 15}));
        medium.inventory().equip(0);
        CHECK(medium.armorClass() == 13);

        Character heavy("Т", CharacterClass::Fighter, 1, clumsy);
        heavy.inventory().add(Item::makeArmor("Латы", {ArmorCategory::Heavy, 18}));
        heavy.inventory().equip(0);
        CHECK(heavy.armorClass() == 18);
    }
    SECTION("Защита без доспехов: варвар и монах")
    {
        Character barb("Варвар", CharacterClass::Barbarian, 1, scores);
        CHECK(barb.armorClass() == 10 + 3 + 2);

        Character monk("Монах", CharacterClass::Monk, 1, scores);
        CHECK(monk.armorClass() == 10 + 3 + 2);
        monk.inventory().add(Item::makeShield("Щит"));
        monk.inventory().equip(0);
        CHECK(monk.armorClass() == 10 + 3 + 2); // щит: монах теряет защиту, +2 от щита
    }
    SECTION("Прочий бонус")
    {
        Character c("Плут", CharacterClass::Rogue, 1, scores);
        c.setMiscArmorClassBonus(1);
        CHECK(c.armorClass() == 14);
    }
}

TEST_CASE("Инвентарь: стопки, экипировка, вес", "[entities][inventory]")
{
    Inventory inv;
    inv.add(Item::consumable("Зелье лечения", "2к4+2", 0.5, 2));
    inv.add(Item::consumable("Зелье лечения", "2к4+2", 0.5, 1));
    REQUIRE(inv.size() == 1);
    CHECK(inv.items()[0].quantity == 3);

    // Оружие не складывается в стопку.
    inv.add(Item::makeWeapon("Кинжал", {"1d4", "piercing", true, false}, 1.0));
    inv.add(Item::makeWeapon("Кинжал", {"1d4", "piercing", true, false}, 1.0));
    CHECK(inv.size() == 3);

    CHECK(inv.totalWeight() == 3 * 0.5 + 2 * 1.0);
    CHECK(inv.find("Кинжал") == 1);
    CHECK(inv.find("Нет такого") == inv.size());

    CHECK(inv.consume(0));
    CHECK(inv.items()[0].quantity == 2);
    CHECK_FALSE(inv.consume(1)); // кинжал не расходуется
    CHECK_FALSE(inv.remove(0, 5));
    CHECK(inv.remove(0, 2));
    CHECK(inv.size() == 2);

    CHECK(inv.equip(0));
    CHECK(inv.items()[0].equipped);
    CHECK(inv.unequip(0));
    CHECK_FALSE(inv.unequip(0));
    CHECK_FALSE(inv.equip(99));

    inv.add(Item::misc("Верёвка", 10.0));
    CHECK_FALSE(inv.equip(inv.size() - 1));
}

TEST_CASE("Надевается только один доспех и один щит", "[entities][inventory]")
{
    Inventory inv;
    inv.add(Item::makeArmor("Кожаный", {ArmorCategory::Light, 11}));
    inv.add(Item::makeArmor("Кольчуга", {ArmorCategory::Heavy, 16}));
    inv.equip(0);
    inv.equip(1);
    CHECK_FALSE(inv.items()[0].equipped);
    REQUIRE(inv.equippedArmor() != nullptr);
    CHECK(inv.equippedArmor()->name == "Кольчуга");
}

TEST_CASE("Спасброски: владение даёт бонус", "[entities][character]")
{
    // Воин владеет СИЛ и ТЕЛ.
    Character c("Воин", CharacterClass::Fighter, 5, AbilityScores(16, 10, 14, 10, 10, 10));
    CHECK(c.proficiencyBonus() == 3);
    CHECK(c.savingThrowBonus(Ability::Strength) == 3 + 3);
    CHECK(c.savingThrowBonus(Ability::Constitution) == 2 + 3);
    CHECK(c.savingThrowBonus(Ability::Dexterity) == 0);
    CHECK(c.initiativeModifier() == 0);
}

TEST_CASE("Ячейки заклинаний по классам и уровням", "[entities][spells]")
{
    SECTION("Полный заклинатель")
    {
        auto s1 = SpellSlots::forClass(CharacterClass::Wizard, 1);
        CHECK(s1.max(1) == 2);
        CHECK(s1.max(2) == 0);

        auto s5 = SpellSlots::forClass(CharacterClass::Wizard, 5);
        CHECK(s5.max(1) == 4);
        CHECK(s5.max(2) == 3);
        CHECK(s5.max(3) == 2);

        auto s20 = SpellSlots::forClass(CharacterClass::Cleric, 20);
        CHECK(s20.max(5) == 3);
        CHECK(s20.max(6) == 2);
        CHECK(s20.max(7) == 2);
        CHECK(s20.max(8) == 1);
        CHECK(s20.max(9) == 1);
    }
    SECTION("Полузаклинатель: заклинания со 2-го уровня")
    {
        CHECK_FALSE(SpellSlots::forClass(CharacterClass::Paladin, 1).hasAny());
        CHECK(SpellSlots::forClass(CharacterClass::Paladin, 2).max(1) == 2);
        CHECK(SpellSlots::forClass(CharacterClass::Paladin, 3).max(1) == 3);
        auto p5 = SpellSlots::forClass(CharacterClass::Ranger, 5);
        CHECK(p5.max(1) == 4);
        CHECK(p5.max(2) == 2);
        auto p20 = SpellSlots::forClass(CharacterClass::Paladin, 20);
        CHECK(p20.max(5) == 2);
        CHECK(p20.max(6) == 0);
    }
    SECTION("Магия договора колдуна")
    {
        auto w1 = SpellSlots::forClass(CharacterClass::Warlock, 1);
        CHECK(w1.max(1) == 1);
        auto w5 = SpellSlots::forClass(CharacterClass::Warlock, 5);
        CHECK(w5.max(3) == 2);
        CHECK(w5.max(1) == 0);
        auto w17 = SpellSlots::forClass(CharacterClass::Warlock, 17);
        CHECK(w17.max(5) == 4);
    }
    SECTION("Не колдующие классы")
    {
        CHECK_FALSE(SpellSlots::forClass(CharacterClass::Fighter, 20).hasAny());
        CHECK_FALSE(SpellSlots::forClass(CharacterClass::Monk, 20).hasAny());
    }
}

TEST_CASE("Трата и восстановление ячеек", "[entities][spells]")
{
    Character w("Маг", CharacterClass::Wizard, 1, AbilityScores(10, 10, 10, 16, 10, 10));
    REQUIRE(w.spellSlots().current(1) == 2);
    CHECK(w.spellSlots().use(1));
    CHECK(w.spellSlots().use(1));
    CHECK_FALSE(w.spellSlots().use(1));
    CHECK_FALSE(w.spellSlots().use(2));
    CHECK_FALSE(w.spellSlots().use(0));
    CHECK_FALSE(w.spellSlots().use(10));

    w.spellSlots().restore(1);
    CHECK(w.spellSlots().current(1) == 1);
    w.spellSlots().restore(1, 50);
    CHECK(w.spellSlots().current(1) == 2);

    // Повышение уровня: новые ячейки приходят заполненными, потраченные остаются потраченными.
    w.spellSlots().use(1);
    w.spellSlots().use(1);
    w.setLevel(3); // 4 + 2 ячейки
    CHECK(w.spellSlots().max(1) == 4);
    CHECK(w.spellSlots().current(1) == 2);
    CHECK(w.spellSlots().current(2) == 2);

    w.longRest();
    CHECK(w.spellSlots().current(1) == 4);
}

TEST_CASE("СЛ заклинаний и бонус атаки", "[entities][spells]")
{
    Character w("Маг", CharacterClass::Wizard, 5, AbilityScores(10, 10, 10, 18, 10, 10)); // ИНТ +4, влад. +3
    CHECK(w.spellSaveDc() == 8 + 3 + 4);
    CHECK(w.spellAttackBonus() == 3 + 4);
    CHECK(w.isSpellcaster());

    Character f("Воин", CharacterClass::Fighter);
    CHECK_FALSE(f.spellSaveDc().has_value());
    CHECK_FALSE(f.spellAttackBonus().has_value());
    CHECK_FALSE(f.isSpellcaster());
}

TEST_CASE("Повышение уровня увеличивает HP", "[entities][character]")
{
    Character c("Воин", CharacterClass::Fighter, 1, AbilityScores(10, 10, 12, 10, 10, 10)); // ТЕЛ +1
    REQUIRE(c.maxHp() == 11);
    c.applyDamage(5);
    REQUIRE(c.currentHp() == 6);

    CHECK(c.levelUp());
    CHECK(c.level() == 2);
    CHECK(c.maxHp() == 11 + 7);
    CHECK(c.currentHp() == 6 + 7);

    c.setLevel(1); // понижение обрезает HP
    CHECK(c.currentHp() == 11);

    c.setLevel(20);
    CHECK_FALSE(c.levelUp());
    c.setLevel(99);
    CHECK(c.level() == 20);
    c.setLevel(-3);
    CHECK(c.level() == 1);
}

TEST_CASE("Длинный и короткий отдых", "[entities][character]")
{
    Character lock("Колдун", CharacterClass::Warlock, 3, AbilityScores(10, 10, 12, 10, 10, 16));
    lock.spellSlots().use(2);
    lock.spellSlots().use(2);
    REQUIRE(lock.spellSlots().current(2) == 0);
    lock.shortRest();
    CHECK(lock.spellSlots().current(2) == 2);

    Character wiz("Маг", CharacterClass::Wizard, 1);
    wiz.spellSlots().use(1);
    wiz.shortRest(); // волшебник на коротком отдыхе ячейки не получает
    CHECK(wiz.spellSlots().current(1) == 1);

    wiz.applyDamage(3);
    wiz.grantTemporaryHp(4);
    wiz.longRest();
    CHECK(wiz.currentHp() == wiz.maxHp());
    CHECK(wiz.temporaryHp() == 0);
    CHECK(wiz.spellSlots().current(1) == 2);
}

TEST_CASE("Грузоподъёмность", "[entities][inventory]")
{
    Character c("Тест", CharacterClass::Fighter, 1, AbilityScores(10, 10, 10, 10, 10, 10));
    CHECK(c.carryCapacity() == 150.0);
    c.inventory().add(Item::misc("Камни", 1.0, 151));
    CHECK(c.isEncumbered());
}

TEST_CASE("Классы: данные и строковые преобразования", "[entities][class]")
{
    for (CharacterClass cls : kAllClasses)
    {
        const auto parsed = characterClassFromString(toString(cls));
        REQUIRE(parsed.has_value());
        CHECK(*parsed == cls);
        CHECK(classInfo(cls).hitDie >= 6);
        CHECK(classInfo(cls).hitDie <= 12);
    }
    CHECK_FALSE(characterClassFromString("jedi").has_value());
    CHECK(classInfo(CharacterClass::Barbarian).hitDie == 12);
    CHECK(classInfo(CharacterClass::Wizard).hitDie == 6);
}

TEST_CASE("Каталог предметов: параметры доспехов и оружия", "[entities][items]")
{
    const auto &presets = itemPresets();
    REQUIRE_FALSE(presets.empty());

    bool foundChainMail = false;
    for (const Item &item : presets)
    {
        CHECK_FALSE(item.name.empty());
        CHECK(item.quantity == 1);
        if (item.type == ItemType::Armor)
            CHECK(item.armor.has_value());
        if (item.type == ItemType::Weapon)
            CHECK(item.weapon.has_value());
        if (item.name == "Chain Mail")
        {
            foundChainMail = true;
            CHECK(item.armor->baseAc == 16);
            CHECK(item.armor->category == ArmorCategory::Heavy);
        }
    }
    CHECK(foundChainMail);
}

TEST_CASE("Roster: добавление, удаление, подсчёт сторон", "[entities][roster]")
{
    Roster roster;
    CHECK(roster.empty());

    const auto a = roster.add(Character("A", CharacterClass::Fighter), Side::Player);
    const auto b = roster.add(Character("B", CharacterClass::Wizard), Side::Enemy);
    CHECK(a == 0);
    CHECK(b == 1);
    CHECK(roster.count(Side::Player) == 1);
    CHECK(roster.count(Side::Enemy) == 1);

    roster.at(1).character.applyDamage(1000);
    CHECK(roster.countAlive(Side::Enemy) == 0);
    CHECK(roster.countAlive(Side::Player) == 1);

    CHECK_FALSE(roster.remove(5));
    CHECK(roster.remove(0));
    CHECK(roster.size() == 1);
    CHECK(roster.at(0).character.name == "B");
}

TEST_CASE("Roster: длинный отдых только для игроков", "[entities][roster]")
{
    Roster roster;
    roster.add(Character("Hero", CharacterClass::Fighter), Side::Player);
    roster.add(Character("Orc", CharacterClass::Barbarian), Side::Enemy);
    roster.at(0).character.applyDamage(5);
    roster.at(1).character.applyDamage(5);

    roster.longRestPlayers();
    CHECK(roster.at(0).character.currentHp() == roster.at(0).character.maxHp());
    CHECK(roster.at(1).character.currentHp() < roster.at(1).character.maxHp());
}

TEST_CASE("Демонстрационная группа: расчёты согласованы", "[entities][roster]")
{
    Roster roster;
    roster.addDemoParty();
    CHECK(roster.count(Side::Player) == 4);
    CHECK(roster.count(Side::Enemy) == 2);

    // Thorin: кольчуга (16, тяжёлый) + щит (+2) = 18
    const Character &thorin = roster.at(0).character;
    REQUIRE(thorin.name == "Thorin");
    CHECK(thorin.armorClass() == 18);

    // Elara: волшебник 3 ур., ИНТ 17 (+3), владение +2 -> СЛ 13; слоты 4/2
    const Character &elara = roster.at(1).character;
    CHECK(*elara.spellSaveDc() == 13);
    CHECK(elara.spellSlots().max(1) == 4);
    CHECK(elara.spellSlots().max(2) == 2);

    // Finn: кожаный доспех (11) + ЛОВ 17 (+3) = 14
    CHECK(roster.at(3).character.armorClass() == 14);

    for (const RosterEntry &e : roster.entries())
        CHECK_FALSE(e.character.isEncumbered());
}
