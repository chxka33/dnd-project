#include "entities/Roster.hpp"

#include "entities/ItemPresets.hpp"

#include <algorithm>
#include <utility>

namespace dnd::entities
{

    const char *toString(Side side)
    {
        return side == Side::Player ? "Player" : "Enemy";
    }

    std::size_t Roster::add(Character character, Side side)
    {
        m_entries.push_back({std::move(character), side});
        return m_entries.size() - 1;
    }

    bool Roster::remove(std::size_t index)
    {
        if (index >= m_entries.size())
            return false;
        m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    std::size_t Roster::count(Side side) const
    {
        return static_cast<std::size_t>(std::count_if(
            m_entries.begin(), m_entries.end(),
            [side](const RosterEntry &e) { return e.side == side; }));
    }

    std::size_t Roster::countAlive(Side side) const
    {
        return static_cast<std::size_t>(std::count_if(
            m_entries.begin(), m_entries.end(),
            [side](const RosterEntry &e) { return e.side == side && !e.character.isDown(); }));
    }

    void Roster::longRestPlayers()
    {
        for (RosterEntry &e : m_entries)
            if (e.side == Side::Player)
                e.character.longRest();
    }

    namespace
    {
        // Берёт предмет из каталога по названию и кладёт в инвентарь; возвращает индекс.
        std::size_t give(Character &c, const char *name, int quantity = 1)
        {
            for (const Item &preset : itemPresets())
            {
                if (preset.name == name)
                {
                    Item item = preset;
                    item.quantity = quantity;
                    return c.inventory().add(std::move(item));
                }
            }
            return c.inventory().size();
        }

        void giveAndEquip(Character &c, const char *name)
        {
            const std::size_t i = give(c, name);
            if (i < c.inventory().size())
                c.inventory().equip(i);
        }
    } // namespace

    void Roster::addDemoParty()
    {
        {
            Character c("Thorin", CharacterClass::Fighter, 3, AbilityScores(16, 12, 15, 10, 12, 8));
            giveAndEquip(c, "Chain Mail");
            giveAndEquip(c, "Shield");
            giveAndEquip(c, "Longsword");
            give(c, "Potion of Healing", 2);
            give(c, "Rations (1 day)", 5);
            add(std::move(c), Side::Player);
        }
        {
            Character c("Elara", CharacterClass::Wizard, 3, AbilityScores(8, 14, 13, 17, 12, 10));
            giveAndEquip(c, "Quarterstaff");
            give(c, "Backpack");
            give(c, "Rations (1 day)", 3);
            add(std::move(c), Side::Player);
        }
        {
            Character c("Mira", CharacterClass::Cleric, 3, AbilityScores(14, 10, 14, 10, 16, 12));
            giveAndEquip(c, "Scale Mail");
            giveAndEquip(c, "Shield");
            giveAndEquip(c, "Warhammer");
            give(c, "Potion of Healing", 1);
            add(std::move(c), Side::Player);
        }
        {
            Character c("Finn", CharacterClass::Rogue, 3, AbilityScores(10, 17, 12, 13, 12, 14));
            giveAndEquip(c, "Leather Armor");
            giveAndEquip(c, "Rapier");
            give(c, "Dagger", 2);
            give(c, "Thieves' Tools");
            add(std::move(c), Side::Player);
        }
        {
            Character c("Goblin Boss", CharacterClass::Fighter, 2, AbilityScores(10, 14, 10, 10, 8, 10));
            giveAndEquip(c, "Studded Leather");
            giveAndEquip(c, "Shortsword");
            add(std::move(c), Side::Enemy);
        }
        {
            Character c("Dark Acolyte", CharacterClass::Warlock, 2, AbilityScores(10, 12, 12, 10, 12, 15));
            giveAndEquip(c, "Dagger");
            add(std::move(c), Side::Enemy);
        }
    }

} // namespace dnd::entities
