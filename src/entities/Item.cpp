#include "entities/Item.hpp"

#include <utility>

namespace dnd::entities
{

    const char *toString(ItemType type)
    {
        switch (type)
        {
        case ItemType::Misc:       return "misc";
        case ItemType::Weapon:     return "weapon";
        case ItemType::Armor:      return "armor";
        case ItemType::Shield:     return "shield";
        case ItemType::Consumable: return "consumable";
        }
        return "misc";
    }

    const char *toString(ArmorCategory category)
    {
        switch (category)
        {
        case ArmorCategory::Light:  return "light";
        case ArmorCategory::Medium: return "medium";
        case ArmorCategory::Heavy:  return "heavy";
        }
        return "light";
    }

    std::optional<ItemType> itemTypeFromString(const std::string &s)
    {
        for (ItemType t : {ItemType::Misc, ItemType::Weapon, ItemType::Armor,
                           ItemType::Shield, ItemType::Consumable})
            if (s == toString(t))
                return t;
        return std::nullopt;
    }

    std::optional<ArmorCategory> armorCategoryFromString(const std::string &s)
    {
        for (ArmorCategory c : {ArmorCategory::Light, ArmorCategory::Medium, ArmorCategory::Heavy})
            if (s == toString(c))
                return c;
        return std::nullopt;
    }

    Item Item::misc(std::string name, double weight, int quantity)
    {
        Item item;
        item.name = std::move(name);
        item.type = ItemType::Misc;
        item.weight = weight;
        item.quantity = quantity;
        return item;
    }

    Item Item::consumable(std::string name, std::string description, double weight, int quantity)
    {
        Item item;
        item.name = std::move(name);
        item.type = ItemType::Consumable;
        item.description = std::move(description);
        item.weight = weight;
        item.quantity = quantity;
        return item;
    }

    Item Item::makeWeapon(std::string name, WeaponStats stats, double weight)
    {
        Item item;
        item.name = std::move(name);
        item.type = ItemType::Weapon;
        item.weight = weight;
        item.weapon = std::move(stats);
        return item;
    }

    Item Item::makeArmor(std::string name, ArmorStats stats, double weight)
    {
        Item item;
        item.name = std::move(name);
        item.type = ItemType::Armor;
        item.weight = weight;
        item.armor = stats;
        return item;
    }

    Item Item::makeShield(std::string name, int acBonus, double weight)
    {
        Item item;
        item.name = std::move(name);
        item.type = ItemType::Shield;
        item.weight = weight;
        item.shieldBonus = acBonus;
        return item;
    }

    bool Item::isStackable() const
    {
        return type == ItemType::Misc || type == ItemType::Consumable;
    }

    bool Item::isEquippable() const
    {
        return type == ItemType::Weapon || type == ItemType::Armor || type == ItemType::Shield;
    }

} // namespace dnd::entities
