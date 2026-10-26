#pragma once

#include "entities/CharacterClass.hpp"

#include <array>

namespace dnd::entities
{

    /// Ячейки заклинаний персонажа: максимум и текущий остаток по уровням заклинаний 1..9.
    class SpellSlots
    {
    public:
        static constexpr int kMaxSpellLevel = 9;
        using Table = std::array<int, kMaxSpellLevel>; ///< индекс 0 — ячейки 1-го уровня

        SpellSlots() = default;

        /// Максимальные ячейки класса на заданном уровне персонажа (1..20); все заполнены.
        static SpellSlots forClass(CharacterClass cls, int level);

        /// Таблица максимумов без создания объекта (для UI/тестов).
        static Table maxSlotsFor(CharacterClass cls, int level);

        /// spellLevel вне 1..9 -> 0 / false.
        int max(int spellLevel) const;
        int current(int spellLevel) const;
        bool hasSlot(int spellLevel) const { return current(spellLevel) > 0; }
        bool hasAny() const;

        /// Тратит одну ячейку. false, если ячеек этого уровня нет.
        bool use(int spellLevel);
        /// Возвращает до count ячеек (не больше максимума).
        void restore(int spellLevel, int count = 1);
        /// Длинный отдых: все ячейки восстановлены.
        void restoreAll();

        /// Меняет максимумы (при повышении уровня). Новые ячейки появляются заполненными,
        /// остальной остаток сохраняется (но не выше нового максимума).
        void setMaxima(const Table &newMax);

    private:
        Table m_max{};
        Table m_current{};
    };

} // namespace dnd::entities
