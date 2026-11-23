#include "combat/InitiativeTracker.hpp"

#include <utility>

namespace dnd::combat
{

    void InitiativeTracker::clear()
    {
        m_combatants.clear();
        m_round = 1;
        m_activeTurnIndex = 0;
    }

    std::size_t InitiativeTracker::findByRosterIndex(std::size_t rosterIndex) const
    {
        for (std::size_t i = 0; i < m_combatants.size(); ++i)
            if (m_combatants[i].rosterIndex == rosterIndex)
                return i;
        return m_combatants.size();
    }

    bool InitiativeTracker::isDown(const entities::Roster *roster, const Combatant &c) const
    {
        return roster && c.rosterIndex < roster->size() && roster->at(c.rosterIndex).character.isDown();
    }

    bool InitiativeTracker::anyAlive(const entities::Roster *roster) const
    {
        if (!roster)
            return !m_combatants.empty();
        return std::any_of(m_combatants.begin(), m_combatants.end(),
                           [&](const Combatant &c) { return !isDown(roster, c); });
    }

    void InitiativeTracker::syncWithRoster(const entities::Roster &roster)
    {
        for (std::size_t i = m_combatants.size(); i-- > 0;)
            if (m_combatants[i].rosterIndex >= roster.size())
                removeCombatant(i);
        for (Combatant &c : m_combatants)
            c.initiativeBonus = roster.at(c.rosterIndex).character.initiativeModifier();
    }

    std::size_t InitiativeTracker::addFromRoster(const entities::Roster &roster, std::size_t rosterIndex,
                                                 int initiative, std::string sourceEnemyId)
    {
        const std::size_t existing = findByRosterIndex(rosterIndex);
        const int activeRoster = activeCombatant() ? static_cast<int>(activeCombatant()->rosterIndex) : -1;

        if (existing < m_combatants.size())
        {
            m_combatants[existing].initiative = initiative;
        }
        else
        {
            Combatant c;
            c.rosterIndex = rosterIndex;
            c.initiative = initiative;
            c.initiativeBonus = rosterIndex < roster.size() ? roster.at(rosterIndex).character.initiativeModifier() : 0;
            c.sourceEnemyId = std::move(sourceEnemyId);
            m_combatants.push_back(std::move(c));
        }

        sort(&roster);
        if (activeRoster >= 0)
        {
            const std::size_t pos = findByRosterIndex(static_cast<std::size_t>(activeRoster));
            if (pos < m_combatants.size())
                m_activeTurnIndex = static_cast<int>(pos);
        }
        return findByRosterIndex(rosterIndex);
    }

    void InitiativeTracker::addPlayersFromRoster(const entities::Roster &roster, utils::Random *rng)
    {
        for (std::size_t i = 0; i < roster.size(); ++i)
        {
            const entities::RosterEntry &e = roster.at(i);
            if (e.side != entities::Side::Player || findByRosterIndex(i) < m_combatants.size())
                continue;
            const int mod = e.character.initiativeModifier();
            const int init = (rng ? rng->range(1, 20) : 10) + mod;
            addFromRoster(roster, i, init);
        }
    }

    std::size_t InitiativeTracker::spawnEnemyFromTemplate(entities::Roster &roster,
                                                          const scenario::EnemyTemplate &enemyTemplate,
                                                          utils::Random *rng, std::optional<int> fixedInitiative)
    {
        entities::Character character = enemyTemplate.toCharacter();

        const std::string base = enemyTemplate.name;
        auto nameTaken = [&](const std::string &name)
        {
            for (const entities::RosterEntry &e : roster.entries())
                if (e.side == entities::Side::Enemy && e.character.name == name)
                    return true;
            return false;
        };
        if (nameTaken(base))
        {
            int n = 2;
            while (nameTaken(base + " #" + std::to_string(n)))
                ++n;
            character.name = base + " #" + std::to_string(n);
        }

        const std::size_t index = roster.add(std::move(character), entities::Side::Enemy);
        const int init = fixedInitiative.value_or((rng ? rng->range(1, 20) : 10) + enemyTemplate.initiativeBonus);
        addFromRoster(roster, index, init, enemyTemplate.id);
        return index;
    }

    std::size_t InitiativeTracker::loadNodeEnemies(entities::Roster &roster, const scenario::ScenarioGraph &graph,
                                                   const scenario::Node &node, utils::Random *rng,
                                                   bool clearExistingEnemies)
    {
        if (clearExistingEnemies)
            removeEnemies(roster);
        addPlayersFromRoster(roster, rng);

        std::size_t added = 0;
        for (const std::string &enemyId : node.enemyIds)
        {
            const scenario::EnemyTemplate *tpl = graph.findEnemy(enemyId);
            if (!tpl)
                continue;
            spawnEnemyFromTemplate(roster, *tpl, rng);
            ++added;
        }
        return added;
    }

    bool InitiativeTracker::removeCombatant(std::size_t trackerIndex)
    {
        if (trackerIndex >= m_combatants.size())
            return false;
        m_combatants.erase(m_combatants.begin() + static_cast<std::ptrdiff_t>(trackerIndex));
        if (static_cast<int>(trackerIndex) < m_activeTurnIndex)
            --m_activeTurnIndex;
        if (m_activeTurnIndex >= static_cast<int>(m_combatants.size()) || m_activeTurnIndex < 0)
            m_activeTurnIndex = 0;
        return true;
    }

    bool InitiativeTracker::removeByRosterIndex(std::size_t rosterIndex, bool rosterEntryRemoved)
    {
        const std::size_t pos = findByRosterIndex(rosterIndex);
        const bool found = pos < m_combatants.size();
        if (found)
            removeCombatant(pos);
        if (rosterEntryRemoved)
            for (Combatant &c : m_combatants)
                if (c.rosterIndex > rosterIndex)
                    --c.rosterIndex;
        return found;
    }

    void InitiativeTracker::removeEnemies(entities::Roster &roster)
    {
        for (std::size_t i = roster.size(); i-- > 0;)
        {
            if (roster.at(i).side != entities::Side::Enemy)
                continue;
            roster.remove(i);
            removeByRosterIndex(i, true);
        }
    }

    bool InitiativeTracker::setInitiative(std::size_t trackerIndex, int initiative, const entities::Roster *roster)
    {
        if (trackerIndex >= m_combatants.size())
            return false;
        const std::size_t activeRoster = activeCombatant() ? activeCombatant()->rosterIndex : 0;
        const bool hadActive = activeCombatant() != nullptr;

        m_combatants[trackerIndex].initiative = initiative;
        sort(roster);
        if (hadActive)
        {
            const std::size_t pos = findByRosterIndex(activeRoster);
            if (pos < m_combatants.size())
                m_activeTurnIndex = static_cast<int>(pos);
        }
        return true;
    }

    void InitiativeTracker::rollInitiativeForEnemies(const entities::Roster &roster, utils::Random &rng)
    {
        for (Combatant &c : m_combatants)
            if (c.rosterIndex < roster.size() && roster.at(c.rosterIndex).side == entities::Side::Enemy)
                c.initiative = rng.range(1, 20) + c.initiativeBonus;
        sort(&roster);
    }

    void InitiativeTracker::rollInitiativeForAll(const entities::Roster &roster, utils::Random &rng)
    {
        for (Combatant &c : m_combatants)
            c.initiative = rng.range(1, 20) + c.initiativeBonus;
        sort(&roster);
    }

    void InitiativeTracker::sort(const entities::Roster *roster)
    {
        const std::size_t activeRoster = activeCombatant() ? activeCombatant()->rosterIndex : 0;
        const bool hadActive = activeCombatant() != nullptr;

        auto isPlayer = [&](const Combatant &c)
        {
            return roster && c.rosterIndex < roster->size() && roster->at(c.rosterIndex).side == entities::Side::Player;
        };
        std::stable_sort(m_combatants.begin(), m_combatants.end(), [&](const Combatant &a, const Combatant &b)
                         {
            if (a.initiative != b.initiative)
                return a.initiative > b.initiative;
            if (a.initiativeBonus != b.initiativeBonus)
                return a.initiativeBonus > b.initiativeBonus;
            const bool pa = isPlayer(a), pb = isPlayer(b);
            if (pa != pb)
                return pa;
            return a.rosterIndex < b.rosterIndex; });

        if (hadActive)
        {
            const std::size_t pos = findByRosterIndex(activeRoster);
            if (pos < m_combatants.size())
                m_activeTurnIndex = static_cast<int>(pos);
        }
    }

    void InitiativeTracker::setActiveTurnIndex(int index)
    {
        if (m_combatants.empty())
        {
            m_activeTurnIndex = 0;
            return;
        }
        m_activeTurnIndex = std::clamp(index, 0, static_cast<int>(m_combatants.size()) - 1);
    }

    const Combatant *InitiativeTracker::activeCombatant() const
    {
        if (m_activeTurnIndex < 0 || m_activeTurnIndex >= static_cast<int>(m_combatants.size()))
            return nullptr;
        return &m_combatants[static_cast<std::size_t>(m_activeTurnIndex)];
    }

    void InitiativeTracker::startCombat(const entities::Roster *roster)
    {
        m_round = 1;
        m_activeTurnIndex = 0;
        if (roster && anyAlive(roster))
        {
            while (m_activeTurnIndex < static_cast<int>(m_combatants.size()) &&
                   isDown(roster, m_combatants[static_cast<std::size_t>(m_activeTurnIndex)]))
                ++m_activeTurnIndex;
        }
    }

    const Combatant *InitiativeTracker::nextTurn(const entities::Roster *roster, bool skipDown)
    {
        if (m_combatants.empty())
            return nullptr;
        const bool skipping = skipDown && roster && anyAlive(roster);
        const int count = static_cast<int>(m_combatants.size());
        for (int step = 0; step < count; ++step)
        {
            ++m_activeTurnIndex;
            if (m_activeTurnIndex >= count)
            {
                m_activeTurnIndex = 0;
                ++m_round;
            }
            if (!skipping || !isDown(roster, m_combatants[static_cast<std::size_t>(m_activeTurnIndex)]))
                break;
        }
        return activeCombatant();
    }

    const Combatant *InitiativeTracker::previousTurn(const entities::Roster *roster, bool skipDown)
    {
        if (m_combatants.empty())
            return nullptr;
        const bool skipping = skipDown && roster && anyAlive(roster);
        const int count = static_cast<int>(m_combatants.size());
        for (int step = 0; step < count; ++step)
        {
            --m_activeTurnIndex;
            if (m_activeTurnIndex < 0)
            {
                m_activeTurnIndex = count - 1;
                m_round = std::max(1, m_round - 1);
            }
            if (!skipping || !isDown(roster, m_combatants[static_cast<std::size_t>(m_activeTurnIndex)]))
                break;
        }
        return activeCombatant();
    }

} // namespace dnd::combat
