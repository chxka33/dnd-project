#pragma once

#include "scenario/Node.hpp"

namespace dnd::scenario
{

    /// Обязательное сюжетное событие: у него ровно один выход, и он не зависит ни от
    /// выбора игроков, ни от случая. Если следующий узел не задан — это финал сценария.
    class StoryNode final : public Node
    {
    public:
        using Node::Node;

        NodeType type() const override { return NodeType::Story; }

        const NodeId &next() const { return m_next; }
        void setNext(NodeId id) { m_next = std::move(id); }
        bool isEnding() const { return m_next.empty(); }

        std::vector<NodeId> targets() const override;
        NodeId resolveNext(utils::Random &rng, std::size_t choiceIndex = 0) const override;
        std::size_t replaceTarget(const NodeId &from, const NodeId &to) override;

    private:
        NodeId m_next;
    };

} // namespace dnd::scenario
