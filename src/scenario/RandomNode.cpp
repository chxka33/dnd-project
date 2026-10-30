#include "scenario/RandomNode.hpp"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace dnd::scenario
{

    void RandomNode::setDiceSides(int sides)
    {
        m_diceSides = std::max(2, sides);
    }

    void RandomNode::setChancePercent(int percent)
    {
        m_legacyChance = std::clamp(percent, 1, 100);
    }

    void RandomNode::addEntry(int minRoll, int maxRoll, std::string label, NodeId target)
    {
        m_entries.push_back({minRoll, maxRoll, std::move(label), std::move(target)});
    }

    bool RandomNode::removeEntry(std::size_t index)
    {
        if (index >= m_entries.size())
            return false;
        m_entries.erase(m_entries.begin() + index);
        return true;
    }

    void RandomNode::clearEntries()
    {
        m_entries.clear();
    }

    RollResult RandomNode::roll(utils::Random &rng) const
    {
        RollResult result;

        if (!m_entries.empty())
        {
            result.diceSides = m_diceSides;
            result.roll = rng.range(1, m_diceSides);
            result.chancePercent = 0;

            for (const auto &entry : m_entries)
            {
                if (result.roll >= entry.minRoll && result.roll <= entry.maxRoll)
                {
                    result.triggered = true;
                    result.label = entry.label;
                    result.next = entry.target;
                    return result;
                }
            }

            // Если не попало ни в один диапазон
            result.triggered = false;
            result.label = "Без происшествий";
            result.next = m_legacyFallback;
            return result;
        }

        // Обратная совместимость с бинарной логикой
        result.diceSides = 100;
        result.chancePercent = m_legacyChance;
        result.roll = rng.range(1, 100);
        result.triggered = (result.roll <= m_legacyChance);
        result.label = result.triggered ? "Случайная встреча" : "Обычный путь";
        result.next = result.triggered ? m_legacyEncounter : m_legacyFallback;
        return result;
    }

    std::vector<NodeId> RandomNode::targets() const
    {
        std::vector<NodeId> result;
        std::unordered_set<NodeId> seen;

        for (const auto &entry : m_entries)
        {
            if (!entry.target.empty() && seen.insert(entry.target).second)
                result.push_back(entry.target);
        }

        if (!m_legacyEncounter.empty() && seen.insert(m_legacyEncounter).second)
            result.push_back(m_legacyEncounter);

        if (!m_legacyFallback.empty() && seen.insert(m_legacyFallback).second)
            result.push_back(m_legacyFallback);

        return result;
    }

    NodeId RandomNode::resolveNext(utils::Random &rng, std::size_t) const
    {
        return roll(rng).next;
    }

    std::size_t RandomNode::replaceTarget(const NodeId &from, const NodeId &to)
    {
        std::size_t replaced = 0;

        for (auto &entry : m_entries)
        {
            if (entry.target == from)
            {
                entry.target = to;
                ++replaced;
            }
        }

        if (m_legacyEncounter == from)
        {
            m_legacyEncounter = to;
            ++replaced;
        }

        if (m_legacyFallback == from)
        {
            m_legacyFallback = to;
            ++replaced;
        }

        return replaced;
    }

} // namespace dnd::scenario
