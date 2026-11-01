#include "entities/Character.hpp"

#include <algorithm>
#include <utility>

namespace dnd::entities
{

    Character::Character(std::string name_, CharacterClass cls, int level, AbilityScores abilities)
        : name(std::move(name_)),
          m_class(cls),
          m_level(std::clamp(level, kMinLevel, kMaxLevel)),
          m_abilities(abilities),
          m_slots(SpellSlots::forClass(cls, m_level))
    {
        m_currentHp = maxHp();
    }

    void Character::setLevel(int level)
    {
        level = std::clamp(level, kMinLevel, kMaxLevel);
        if (level == m_level)
            return;

        const int previousMax = maxHp();
        m_level = level;
        refreshDerived(previousMax);
    }

    bool Character::levelUp()
    {
        if (m_level >= kMaxLevel)
            return false;
        setLevel(m_level + 1);
        return true;
    }

    void Character::refreshDerived(int previousMaxHp)
    {
        const int newMax = maxHp();
        // Повышение: текущие HP растут на прирост максимума. Понижение: обрезаются.
        m_currentHp = std::clamp(currentHp() + std::max(0, newMax - previousMaxHp), 0, newMax);
        m_slots.setMaxima(SpellSlots::maxSlotsFor(m_class, m_level));
    }

    int Character::proficiencyBonusForLevel(int level)
    {
        level = std::clamp(level, kMinLevel, kMaxLevel);
        return 2 + (level - 1) / 4;
    }

    bool Character::isProficientInSave(Ability ability) const
    {
        const auto &saves = classData().savingThrows;
        return saves[0] == ability || saves[1] == ability;
    }

    int Character::savingThrowBonus(Ability ability) const
    {
        return modifier(ability) + (isProficientInSave(ability) ? proficiencyBonus() : 0);
    }

    int Character::armorClass() const
    {
        const int dex = modifier(Ability::Dexterity);
        const Item *armor = m_inventory.equippedArmor();
        const Item *shield = m_inventory.equippedShield();

        int ac = 10 + dex; // без доспехов
        if (armor)
        {
            const ArmorStats &stats = *armor->armor;
            switch (stats.category)
            {
            case ArmorCategory::Light:  ac = stats.baseAc + dex; break;
            case ArmorCategory::Medium: ac = stats.baseAc + std::min(dex, 2); break;
            case ArmorCategory::Heavy:  ac = stats.baseAc; break;
            }
        }
        else if (m_class == CharacterClass::Barbarian)
        {
            ac = 10 + dex + modifier(Ability::Constitution);
        }
        else if (m_class == CharacterClass::Monk && !shield)
        {
            ac = 10 + dex + modifier(Ability::Wisdom);
        }

        if (shield)
            ac += shield->shieldBonus;
        return ac + m_miscAcBonus;
    }

    int Character::maxHp() const
    {
        const int die = hitDieSize();
        const int con = modifier(Ability::Constitution);
        const int firstLevel = std::max(1, die + con);
        const int perLevel = std::max(1, die / 2 + 1 + con);
        return firstLevel + (m_level - 1) * perLevel;
    }

    int Character::currentHp() const
    {
        return std::min(m_currentHp, maxHp());
    }

    void Character::setCurrentHp(int hp)
    {
        m_currentHp = std::clamp(hp, 0, maxHp());
    }

    int Character::applyDamage(int amount)
    {
        if (amount <= 0)
            return 0;

        const int absorbed = std::min(m_tempHp, amount);
        m_tempHp -= absorbed;
        amount -= absorbed;

        const int hp = currentHp();
        const int lost = std::min(hp, amount);
        m_currentHp = hp - lost;
        return lost;
    }

    int Character::heal(int amount)
    {
        if (amount <= 0)
            return 0;
        const int before = currentHp();
        m_currentHp = std::min(before + amount, maxHp());
        return m_currentHp - before;
    }

    void Character::grantTemporaryHp(int amount)
    {
        m_tempHp = std::max(m_tempHp, std::max(0, amount));
    }

    std::optional<int> Character::spellSaveDc() const
    {
        const auto ability = classData().spellcastingAbility;
        if (!ability)
            return std::nullopt;
        return 8 + proficiencyBonus() + modifier(*ability);
    }

    std::optional<int> Character::spellAttackBonus() const
    {
        const auto ability = classData().spellcastingAbility;
        if (!ability)
            return std::nullopt;
        return proficiencyBonus() + modifier(*ability);
    }

    void Character::longRest()
    {
        m_currentHp = maxHp();
        m_tempHp = 0;
        m_slots.restoreAll();
    }

    void Character::shortRest()
    {
        if (classData().caster == CasterType::Pact)
            m_slots.restoreAll();
    }

    int Character::passivePerception() const
    {
        return 10 + modifier(Ability::Wisdom);
    }

    void Character::addCondition(std::string condition)
    {
        if (condition.empty())
            return;
        if (!hasCondition(condition))
            m_conditions.push_back(std::move(condition));
    }

    void Character::removeCondition(const std::string &condition)
    {
        m_conditions.erase(
            std::remove(m_conditions.begin(), m_conditions.end(), condition),
            m_conditions.end());
    }

    bool Character::hasCondition(const std::string &condition) const
    {
        return std::find(m_conditions.begin(), m_conditions.end(), condition) != m_conditions.end();
    }

    void Character::clearConditions()
    {
        m_conditions.clear();
    }

} // namespace dnd::entities
