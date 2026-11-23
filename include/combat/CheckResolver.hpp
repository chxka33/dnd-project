#pragma once

#include "entities/Character.hpp"
#include "scenario/Node.hpp"
#include "utils/Random.hpp"

#include <optional>
#include <string>

/// Модуль 3.3: универсальный калькулятор испытаний (Check & Save Resolver). Без ImGui и SFML.
namespace dnd::combat
{

    /// Итог проверки навыка или спасброска.
    struct CheckOutcome
    {
        scenario::CheckType type = scenario::CheckType::SkillCheck;
        scenario::Ability ability = scenario::Ability::Strength;
        std::string skill;
        std::string characterName;
        int diceRoll = 0;  ///< число на d20 (физический бросок игрока или бросок ГСЧ)
        int modifier = 0;  ///< применённый модификатор героя
        int total = 0;     ///< diceRoll + modifier
        int dc = 10;
        bool proficient = false;
        bool success = false;
        bool natural20 = false;
        bool natural1 = false;

        const char *verdict() const noexcept { return success ? "SUCCESS" : "FAILURE"; }
    };

    /// Модификатор проверки: мод. характеристики (+ бонус владения навыком) + доп. бонус.
    int skillCheckModifier(const entities::Character &character, scenario::Ability ability,
                           bool skillProficient = false, int extraBonus = 0);

    /// Модификатор спасброска героя (с владением от класса) + доп. бонус.
    int savingThrowModifier(const entities::Character &character, scenario::Ability ability, int extraBonus = 0);

    /// Владеет ли герой навыком проверки (с учётом ручного значения и автоопределения Восприятия).
    bool isSkillProficient(const entities::Character &character, const scenario::CheckDefinition &check,
                           std::optional<bool> skillProficientOverride = std::nullopt);

    int modifierForCheck(const entities::Character &character, const scenario::CheckDefinition &check,
                         std::optional<bool> skillProficientOverride = std::nullopt, int extraBonus = 0);

    /// Сверяет брошенное игроком число с DC: playerRoll + модификатор >= DC.
    CheckOutcome resolveCheck(const entities::Character &character, const scenario::CheckDefinition &check,
                              int playerRoll, std::optional<bool> skillProficientOverride = std::nullopt,
                              int extraBonus = 0);

    /// То же, но d20 бросает ГСЧ.
    CheckOutcome rollAndResolveCheck(const entities::Character &character, const scenario::CheckDefinition &check,
                                     utils::Random &rng, std::optional<bool> skillProficientOverride = std::nullopt,
                                     int extraBonus = 0);

    /// "Thorin - DEX Saving Throw: roll 14 +3 = 17 vs DC 15 -> SUCCESS"
    std::string formatCheckOutcome(const CheckOutcome &outcome);

} // namespace dnd::combat
