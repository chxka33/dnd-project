#pragma once

#include <optional>
#include <string>

namespace dnd::entities
{

    enum class ItemType
    {
        Misc,       ///< прочее снаряжение, боеприпасы, сокровища
        Weapon,     ///< оружие
        Armor,      ///< доспех
        Shield,     ///< щит
        Consumable  ///< расходуемый предмет (зелье, свиток, паёк)
    };

    enum class ArmorCategory
    {
        Light,  ///< КД = база + модификатор Ловкости
        Medium, ///< КД = база + модификатор Ловкости (максимум +2)
        Heavy   ///< КД = база, Ловкость не учитывается
    };

    const char *toString(ItemType type);
    const char *toString(ArmorCategory category);
    std::optional<ItemType> itemTypeFromString(const std::string &s);
    std::optional<ArmorCategory> armorCategoryFromString(const std::string &s);

    struct WeaponStats
    {
        std::string damage = "1d6"; ///< формула урона: "1d8", "2d6"
        std::string damageType;     ///< "slashing", "piercing", ...
        bool finesse = false;       ///< фехтовальное: можно использовать Ловкость вместо Силы
        bool ranged = false;        ///< дальнобойное: атака использует Ловкость
    };

    struct ArmorStats
    {
        ArmorCategory category = ArmorCategory::Light;
        int baseAc = 10;               ///< базовый КД доспеха (кожаный — 11, кольчуга — 16)
        int strengthRequirement = 0;   ///< 0 — требований нет
        bool stealthDisadvantage = false;
    };

    /// Предмет инвентаря. Поля публичные — их напрямую редактирует GUI листа персонажа.
    struct Item
    {
        std::string name;
        ItemType type = ItemType::Misc;
        std::string description;
        double weight = 0.0; ///< вес ОДНОГО предмета, в фунтах
        int quantity = 1;
        bool equipped = false; ///< имеет смысл только для Weapon / Armor / Shield

        std::optional<WeaponStats> weapon; ///< заполнено для Weapon
        std::optional<ArmorStats> armor;   ///< заполнено для Armor
        int shieldBonus = 0;               ///< бонус к КД для Shield (обычный щит — +2)

        // Фабрики для типовых предметов.
        static Item misc(std::string name, double weight = 0.0, int quantity = 1);
        static Item consumable(std::string name, std::string description = {},
                               double weight = 0.0, int quantity = 1);
        static Item makeWeapon(std::string name, WeaponStats stats, double weight = 0.0);
        static Item makeArmor(std::string name, ArmorStats stats, double weight = 0.0);
        static Item makeShield(std::string name, int acBonus = 2, double weight = 6.0);

        /// Предметы этого типа складываются в стопку (quantity > 1); экипируемые — всегда по одному.
        bool isStackable() const;
        bool isEquippable() const;
        double totalWeight() const { return weight * quantity; }
    };

} // namespace dnd::entities
