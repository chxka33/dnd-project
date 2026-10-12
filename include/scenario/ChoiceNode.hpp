#pragma once

#include "scenario/Node.hpp"

namespace dnd::scenario
{

    /// Один вариант выбора на развилке.
    struct Choice
    {
        std::string label; ///< текст варианта: «Войти в пещеру»
        NodeId target;     ///< узел, в который ведёт вариант
    };

    /// Развилка: игроки выбирают один из вариантов, каждый ведёт в свой узел.
    class ChoiceNode final : public Node
    {
    public:
        using Node::Node;

        NodeType type() const override { return NodeType::Choice; }

        const std::vector<Choice> &choices() const { return m_choices; }
        std::vector<Choice> &choices() { return m_choices; } ///< для редактирования в GUI

        /// Добавляет вариант и возвращает его индекс.
        std::size_t addChoice(std::string label, NodeId target = {});
        /// Удаляет вариант по индексу; false, если индекс вне диапазона.
        bool removeChoice(std::size_t index);

        std::vector<NodeId> targets() const override;
        /// choiceIndex — индекс выбранного варианта. Вне диапазона -> std::out_of_range.
        NodeId resolveNext(utils::Random &rng, std::size_t choiceIndex = 0) const override;
        std::size_t replaceTarget(const NodeId &from, const NodeId &to) override;

    private:
        std::vector<Choice> m_choices;
    };

} // namespace dnd::scenario
