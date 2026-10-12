#include "scenario/ChoiceNode.hpp"

#include <stdexcept>

namespace dnd::scenario
{

    std::size_t ChoiceNode::addChoice(std::string label, NodeId target)
    {
        m_choices.push_back({std::move(label), std::move(target)});
        return m_choices.size() - 1;
    }

    bool ChoiceNode::removeChoice(std::size_t index)
    {
        if (index >= m_choices.size())
            return false;
        m_choices.erase(m_choices.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    std::vector<NodeId> ChoiceNode::targets() const
    {
        std::vector<NodeId> result;
        for (const Choice &c : m_choices)
            if (!c.target.empty())
                result.push_back(c.target);
        return result;
    }

    NodeId ChoiceNode::resolveNext(utils::Random &, std::size_t choiceIndex) const
    {
        if (choiceIndex >= m_choices.size())
            throw std::out_of_range("ChoiceNode '" + id() + "': no such choice");
        return m_choices[choiceIndex].target;
    }

    std::size_t ChoiceNode::replaceTarget(const NodeId &from, const NodeId &to)
    {
        std::size_t replaced = 0;
        for (Choice &c : m_choices)
        {
            if (c.target == from)
            {
                c.target = to;
                ++replaced;
            }
        }
        return replaced;
    }

} // namespace dnd::scenario
