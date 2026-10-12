#include "scenario/RandomNode.hpp"

#include <algorithm>

namespace dnd::scenario
{

    void RandomNode::setChancePercent(int percent)
    {
        m_chance = std::clamp(percent, kMinChance, kMaxChance);
    }

    RollResult RandomNode::roll(utils::Random &rng) const
    {
        RollResult result;
        result.chancePercent = m_chance;
        result.roll = rng.range(1, 100);
        result.triggered = result.roll <= m_chance;
        result.next = result.triggered ? m_encounter : m_fallback;
        return result;
    }

    std::vector<NodeId> RandomNode::targets() const
    {
        std::vector<NodeId> result;
        if (!m_encounter.empty())
            result.push_back(m_encounter);
        if (!m_fallback.empty())
            result.push_back(m_fallback);
        return result;
    }

    NodeId RandomNode::resolveNext(utils::Random &rng, std::size_t) const
    {
        return roll(rng).next;
    }

    std::size_t RandomNode::replaceTarget(const NodeId &from, const NodeId &to)
    {
        std::size_t replaced = 0;
        if (m_encounter == from)
        {
            m_encounter = to;
            ++replaced;
        }
        if (m_fallback == from)
        {
            m_fallback = to;
            ++replaced;
        }
        return replaced;
    }

} // namespace dnd::scenario
