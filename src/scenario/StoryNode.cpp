#include "scenario/StoryNode.hpp"

namespace dnd::scenario
{

    std::vector<NodeId> StoryNode::targets() const
    {
        if (m_next.empty())
            return {};
        return {m_next};
    }

    NodeId StoryNode::resolveNext(utils::Random &, std::size_t) const
    {
        return m_next;
    }

    std::size_t StoryNode::replaceTarget(const NodeId &from, const NodeId &to)
    {
        if (m_next != from)
            return 0;
        m_next = to;
        return 1;
    }

} // namespace dnd::scenario
