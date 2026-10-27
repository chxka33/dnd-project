#pragma once

#include "scenario/Node.hpp"

#include <array>
#include <cstddef>

namespace dnd::entities
{

    /// Шесть характеристик D&D — тот же enum, что и в проверках навыков сценария,
    /// чтобы SkillCheck узла напрямую сопоставлялся с характеристикой персонажа.
    using Ability = scenario::Ability;

    inline constexpr std::size_t kAbilityCount = 6;

    /// Все характеристики в стандартном порядке листа персонажа (СИЛ, ЛОВ, ТЕЛ, ИНТ, МДР, ХАР).
    inline constexpr std::array<Ability, kAbilityCount> kAllAbilities = {
        Ability::Strength, Ability::Dexterity, Ability::Constitution,
        Ability::Intelligence, Ability::Wisdom, Ability::Charisma};

    /// Короткое обозначение для UI: "STR", "DEX", ... (латиницей: шрифт ImGui по умолчанию без кириллицы)
    /// Полное английское название: "Strength", "Dexterity", ...
    const char *shortName(Ability ability);
    const char *fullName(Ability ability);

    /// Блок характеристик персонажа: значения (1..30) и автоматический расчёт модификаторов.
    class AbilityScores
    {
    public:
        static constexpr int kMinScore = 1;
        static constexpr int kMaxScore = 30;
        static constexpr int kDefaultScore = 10;

        /// Все характеристики = 10 (модификатор 0).
        AbilityScores();
        AbilityScores(int strength, int dexterity, int constitution,
                      int intelligence, int wisdom, int charisma);

        int score(Ability ability) const;
        /// Значение обрезается до [kMinScore, kMaxScore].
        void setScore(Ability ability, int value);
        /// Прибавляет (или отнимает) delta, результат обрезается до [kMinScore, kMaxScore].
        void addScore(Ability ability, int delta);

        /// Модификатор характеристики: floor((значение - 10) / 2).
        int modifier(Ability ability) const { return modifierForScore(score(ability)); }

        /// Модификатор для произвольного значения (с корректным округлением вниз для значений < 10).
        static int modifierForScore(int score);

    private:
        std::array<int, kAbilityCount> m_scores;
    };

} // namespace dnd::entities
