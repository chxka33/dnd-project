#include "combat/CheckResolver.hpp"

#include "entities/AbilityScores.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace dnd::combat
{

    namespace
    {
        bool equalsIgnoreCase(const std::string &a, const char *b)
        {
            const std::string other = b;
            if (a.size() != other.size())
                return false;
            return std::equal(a.begin(), a.end(), other.begin(), [](char x, char y)
                              { return std::tolower(static_cast<unsigned char>(x)) ==
                                       std::tolower(static_cast<unsigned char>(y)); });
        }
    } // namespace

    int skillCheckModifier(const entities::Character &character, scenario::Ability ability,
                           bool skillProficient, int extraBonus)
    {
        return character.modifier(ability) + (skillProficient ? character.proficiencyBonus() : 0) + extraBonus;
    }

    int savingThrowModifier(const entities::Character &character, scenario::Ability ability, int extraBonus)
    {
        return character.savingThrowBonus(ability) + extraBonus;
    }

    bool isSkillProficient(const entities::Character &character, const scenario::CheckDefinition &check,
                           std::optional<bool> skillProficientOverride)
    {
        if (check.type == scenario::CheckType::SavingThrow)
            return character.isProficientInSave(check.ability);
        if (skillProficientOverride.has_value())
            return *skillProficientOverride;
        if (check.ability == scenario::Ability::Wisdom && equalsIgnoreCase(check.skill, "Perception"))
            return character.isProficientInPerception();
        return false;
    }

    int modifierForCheck(const entities::Character &character, const scenario::CheckDefinition &check,
                         std::optional<bool> skillProficientOverride, int extraBonus)
    {
        if (check.type == scenario::CheckType::SavingThrow)
            return savingThrowModifier(character, check.ability, extraBonus);
        return skillCheckModifier(character, check.ability,
                                  isSkillProficient(character, check, skillProficientOverride), extraBonus);
    }

    CheckOutcome resolveCheck(const entities::Character &character, const scenario::CheckDefinition &check,
                              int playerRoll, std::optional<bool> skillProficientOverride, int extraBonus)
    {
        CheckOutcome out;
        out.type = check.type;
        out.ability = check.ability;
        out.skill = check.skill;
        out.characterName = character.name;
        out.diceRoll = playerRoll;
        out.modifier = modifierForCheck(character, check, skillProficientOverride, extraBonus);
        out.total = playerRoll + out.modifier;
        out.dc = check.dc;
        out.proficient = isSkillProficient(character, check, skillProficientOverride);
        out.success = out.total >= check.dc;
        out.natural20 = playerRoll == 20;
        out.natural1 = playerRoll == 1;
        return out;
    }

    CheckOutcome rollAndResolveCheck(const entities::Character &character, const scenario::CheckDefinition &check,
                                     utils::Random &rng, std::optional<bool> skillProficientOverride, int extraBonus)
    {
        return resolveCheck(character, check, rng.range(1, 20), skillProficientOverride, extraBonus);
    }

    std::string formatCheckOutcome(const CheckOutcome &o)
    {
        std::string what = entities::shortName(o.ability);
        if (o.type == scenario::CheckType::SkillCheck && !o.skill.empty())
            what += " (" + o.skill + ")";
        what += o.type == scenario::CheckType::SavingThrow ? " Saving Throw" : " Check";

        char buf[256];
        std::snprintf(buf, sizeof(buf), "%s - %s: roll %d %+d = %d vs DC %d -> %s%s%s", o.characterName.c_str(),
                      what.c_str(), o.diceRoll, o.modifier, o.total, o.dc, o.verdict(),
                      o.natural20 ? " (NAT 20)" : "", o.natural1 ? " (NAT 1)" : "");
        return buf;
    }

} // namespace dnd::combat
