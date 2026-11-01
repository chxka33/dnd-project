#include "entities/ItemPresets.hpp"

namespace dnd::entities
{

    namespace
    {
        Item armor(const char *name, ArmorCategory cat, int baseAc, double weight,
                   int strReq = 0, bool stealthDis = false)
        {
            return Item::makeArmor(name, {cat, baseAc, strReq, stealthDis}, weight);
        }

        Item weapon(const char *name, const char *damage, const char *type, double weight,
                    bool finesse = false, bool ranged = false)
        {
            return Item::makeWeapon(name, {damage, type, finesse, ranged}, weight);
        }

        std::vector<Item> buildPresets()
        {
            using C = ArmorCategory;
            std::vector<Item> v;

            // Доспехи
            v.push_back(armor("Leather Armor", C::Light, 11, 10));
            v.push_back(armor("Studded Leather", C::Light, 12, 13));
            v.push_back(armor("Chain Shirt", C::Medium, 13, 20));
            v.push_back(armor("Scale Mail", C::Medium, 14, 45, 0, true));
            v.push_back(armor("Breastplate", C::Medium, 14, 20));
            v.push_back(armor("Half Plate", C::Medium, 15, 40, 0, true));
            v.push_back(armor("Chain Mail", C::Heavy, 16, 55, 13, true));
            v.push_back(armor("Splint", C::Heavy, 17, 60, 15, true));
            v.push_back(armor("Plate", C::Heavy, 18, 65, 15, true));
            v.push_back(Item::makeShield("Shield", 2, 6.0));

            // Оружие
            v.push_back(weapon("Dagger", "1d4", "piercing", 1, true));
            v.push_back(weapon("Shortsword", "1d6", "piercing", 2, true));
            v.push_back(weapon("Rapier", "1d8", "piercing", 2, true));
            v.push_back(weapon("Longsword", "1d8", "slashing", 3));
            v.push_back(weapon("Greataxe", "1d12", "slashing", 7));
            v.push_back(weapon("Warhammer", "1d8", "bludgeoning", 2));
            v.push_back(weapon("Quarterstaff", "1d6", "bludgeoning", 4));
            v.push_back(weapon("Shortbow", "1d6", "piercing", 2, false, true));
            v.push_back(weapon("Longbow", "1d8", "piercing", 2, false, true));

            // Расходники и снаряжение
            v.push_back(Item::consumable("Potion of Healing", "Restores 2d4+2 HP", 0.5));
            v.push_back(Item::consumable("Rations (1 day)", "Food for one day", 2.0));
            v.push_back(Item::consumable("Torch", "Burns for 1 hour", 1.0));
            v.push_back(Item::misc("Backpack", 5.0));
            v.push_back(Item::misc("Rope, hempen (50 ft)", 10.0));
            v.push_back(Item::misc("Arrows (20)", 1.0));
            v.push_back(Item::misc("Thieves' Tools", 1.0));
            return v;
        }
    } // namespace

    const std::vector<Item> &itemPresets()
    {
        static const std::vector<Item> presets = buildPresets();
        return presets;
    }

} // namespace dnd::entities
