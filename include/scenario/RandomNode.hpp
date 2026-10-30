#pragma once

#include "scenario/Node.hpp"

namespace dnd::scenario
{

    /// Запись в таблице случайных встреч / исходов (Roll Table Entry).
    struct RollTableEntry
    {
        int minRoll = 1;   ///< начальное значение диапазона включительно
        int maxRoll = 1;   ///< конечное значение диапазона включительно
        std::string label; ///< описание исхода: "1-2: Засада разбойников"
        NodeId target;     ///< целевой узел при выпадении значения в диапазоне
    };

    /// Итог броска по таблице случайных встреч.
    struct RollResult
    {
        bool triggered = false; ///< найдено ли совпадение в таблице
        int roll = 0;           ///< выпавшее значение кубика (для лога симулятора)
        int diceSides = 100;    ///< кость, которую бросали (d6, d20, d100)
        std::string label;      ///< описание сработавшего исхода
        NodeId next;            ///< куда идём дальше
        int chancePercent = 0;  ///< для обратной совместимости
    };

    /// Вероятностный узел (Таблица случайных встреч / Roll Table).
    class RandomNode final : public Node
    {
    public:
        static constexpr int kDefaultDiceSides = 100;

        using Node::Node;

        NodeType type() const override { return NodeType::Random; }

        int diceSides() const { return m_diceSides; }
        void setDiceSides(int sides);

        const std::vector<RollTableEntry> &entries() const { return m_entries; }
        std::vector<RollTableEntry> &entries() { return m_entries; }

        /// Добавляет строку в таблицу исходов.
        void addEntry(int minRoll, int maxRoll, std::string label, NodeId target);
        bool removeEntry(std::size_t index);
        void clearEntries();

        /// Бросает кубик 1..diceSides и выбирает ветку по таблице.
        RollResult roll(utils::Random &rng) const;

        std::vector<NodeId> targets() const override;
        NodeId resolveNext(utils::Random &rng, std::size_t choiceIndex = 0) const override;
        std::size_t replaceTarget(const NodeId &from, const NodeId &to) override;

        // --- Совместимость со старым бинарным API (P в [1, 100]%) ---
        int chancePercent() const { return m_legacyChance; }
        void setChancePercent(int percent);

        const NodeId &encounterTarget() const { return m_legacyEncounter; }
        const NodeId &fallbackTarget() const { return m_legacyFallback; }
        void setEncounterTarget(NodeId id) { m_legacyEncounter = std::move(id); }
        void setFallbackTarget(NodeId id) { m_legacyFallback = std::move(id); }

    private:
        int m_diceSides = kDefaultDiceSides;
        std::vector<RollTableEntry> m_entries;

        // Для обратной совместимости
        int m_legacyChance = 50;
        NodeId m_legacyEncounter;
        NodeId m_legacyFallback;
    };

} // namespace dnd::scenario
