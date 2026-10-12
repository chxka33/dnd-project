#pragma once

#include "scenario/Node.hpp"

namespace dnd::scenario
{

    /// Итог броска вероятностного узла.
    struct RollResult
    {
        bool triggered = false; ///< сработала ли случайная встреча / событие
        int roll = 0;           ///< выпавшее значение d100 (для лога симулятора)
        int chancePercent = 0;  ///< шанс, с которым бросали
        NodeId next;            ///< куда идём дальше
    };

    /// Вероятностный узел («случайная встреча»).
    ///
    /// При переходе через узел бросается d100: если выпало <= chancePercent,
    /// срабатывает событие (идём в encounterTarget), иначе сценарий продолжается
    /// по основной ветке (fallbackTarget).
    ///
    ///     P(событие) = chancePercent / 100,   chancePercent ∈ [1, 100]
    class RandomNode final : public Node
    {
    public:
        static constexpr int kMinChance = 1;
        static constexpr int kMaxChance = 100;

        using Node::Node;

        NodeType type() const override { return NodeType::Random; }

        int chancePercent() const { return m_chance; }
        /// Значение вне [1, 100] приводится к ближайшей границе диапазона.
        void setChancePercent(int percent);

        const NodeId &encounterTarget() const { return m_encounter; }
        const NodeId &fallbackTarget() const { return m_fallback; }
        void setEncounterTarget(NodeId id) { m_encounter = std::move(id); }
        void setFallbackTarget(NodeId id) { m_fallback = std::move(id); }

        /// Бросает d100 и выбирает ветку. Единственное место, где живёт логика шанса.
        RollResult roll(utils::Random &rng) const;

        std::vector<NodeId> targets() const override;
        NodeId resolveNext(utils::Random &rng, std::size_t choiceIndex = 0) const override;
        std::size_t replaceTarget(const NodeId &from, const NodeId &to) override;

    private:
        int m_chance = 50;
        NodeId m_encounter; ///< ветка «событие сработало»
        NodeId m_fallback;  ///< ветка «ничего не случилось»
    };

} // namespace dnd::scenario
