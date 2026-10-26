#include "entities/AbilityScores.hpp"

#include <algorithm>

namespace dnd::entities
{

    namespace
    {
        std::size_t indexOf(Ability ability)
        {
            return static_cast<std::size_t>(ability);
        }

        int clampScore(int value)
        {
            return std::clamp(value, AbilityScores::kMinScore, AbilityScores::kMaxScore);
        }
    } // namespace

    const char *shortName(Ability ability)
    {
        switch (ability)
        {
        case Ability::Strength:     return "СИЛ";
        case Ability::Dexterity:    return "ЛОВ";
        case Ability::Constitution: return "ТЕЛ";
        case Ability::Intelligence: return "ИНТ";
        case Ability::Wisdom:       return "МДР";
        case Ability::Charisma:     return "ХАР";
        }
        return "СИЛ";
    }

    AbilityScores::AbilityScores()
    {
        m_scores.fill(kDefaultScore);
    }

    AbilityScores::AbilityScores(int strength, int dexterity, int constitution,
                                 int intelligence, int wisdom, int charisma)
        : m_scores{clampScore(strength), clampScore(dexterity), clampScore(constitution),
                   clampScore(intelligence), clampScore(wisdom), clampScore(charisma)}
    {
    }

    int AbilityScores::score(Ability ability) const
    {
        return m_scores[indexOf(ability)];
    }

    void AbilityScores::setScore(Ability ability, int value)
    {
        m_scores[indexOf(ability)] = clampScore(value);
    }

    void AbilityScores::addScore(Ability ability, int delta)
    {
        setScore(ability, score(ability) + delta);
    }

    int AbilityScores::modifierForScore(int score)
    {
        // Целочисленное деление в C++ округляет к нулю, а в D&D нужно вниз: 9 -> -1, 7 -> -2.
        const int diff = score - kDefaultScore;
        return diff >= 0 ? diff / 2 : -((-diff + 1) / 2);
    }

} // namespace dnd::entities
