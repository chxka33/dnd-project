#include "entities/SpellSlots.hpp"

#include <algorithm>

namespace dnd::entities
{

    namespace
    {
        // Ячейки полного заклинателя по уровню персонажа (строка = уровень 1..20,
        // столбец = уровень заклинаний 1..9).
        constexpr int kFullCasterTable[20][SpellSlots::kMaxSpellLevel] = {
            {2, 0, 0, 0, 0, 0, 0, 0, 0}, // 1
            {3, 0, 0, 0, 0, 0, 0, 0, 0}, // 2
            {4, 2, 0, 0, 0, 0, 0, 0, 0}, // 3
            {4, 3, 0, 0, 0, 0, 0, 0, 0}, // 4
            {4, 3, 2, 0, 0, 0, 0, 0, 0}, // 5
            {4, 3, 3, 0, 0, 0, 0, 0, 0}, // 6
            {4, 3, 3, 1, 0, 0, 0, 0, 0}, // 7
            {4, 3, 3, 2, 0, 0, 0, 0, 0}, // 8
            {4, 3, 3, 3, 1, 0, 0, 0, 0}, // 9
            {4, 3, 3, 3, 2, 0, 0, 0, 0}, // 10
            {4, 3, 3, 3, 2, 1, 0, 0, 0}, // 11
            {4, 3, 3, 3, 2, 1, 0, 0, 0}, // 12
            {4, 3, 3, 3, 2, 1, 1, 0, 0}, // 13
            {4, 3, 3, 3, 2, 1, 1, 0, 0}, // 14
            {4, 3, 3, 3, 2, 1, 1, 1, 0}, // 15
            {4, 3, 3, 3, 2, 1, 1, 1, 0}, // 16
            {4, 3, 3, 3, 2, 1, 1, 1, 1}, // 17
            {4, 3, 3, 3, 3, 1, 1, 1, 1}, // 18
            {4, 3, 3, 3, 3, 2, 1, 1, 1}, // 19
            {4, 3, 3, 3, 3, 2, 2, 1, 1}, // 20
        };

        struct PactEntry
        {
            int slots;
            int slotLevel;
        };

        // Магия договора колдуна по уровню персонажа 1..20.
        constexpr PactEntry kPactTable[20] = {
            {1, 1}, {2, 1}, {2, 2}, {2, 2}, {2, 3}, {2, 3}, {2, 4}, {2, 4}, {2, 5}, {2, 5},
            {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}};

        int clampLevel(int level)
        {
            return std::clamp(level, 1, 20);
        }

        bool validLevel(int spellLevel)
        {
            return spellLevel >= 1 && spellLevel <= SpellSlots::kMaxSpellLevel;
        }
    } // namespace

    SpellSlots::Table SpellSlots::maxSlotsFor(CharacterClass cls, int level)
    {
        level = clampLevel(level);
        Table result{};

        switch (classInfo(cls).caster)
        {
        case CasterType::None:
            break;
        case CasterType::Full:
            for (int i = 0; i < kMaxSpellLevel; ++i)
                result[static_cast<std::size_t>(i)] = kFullCasterTable[level - 1][i];
            break;
        case CasterType::Half:
        {
            // Заклинания с 2-го уровня; эффективный уровень заклинателя = ceil(level / 2).
            if (level < 2)
                break;
            const int effective = (level + 1) / 2;
            for (int i = 0; i < kMaxSpellLevel; ++i)
                result[static_cast<std::size_t>(i)] = kFullCasterTable[effective - 1][i];
            break;
        }
        case CasterType::Pact:
        {
            const PactEntry &e = kPactTable[level - 1];
            result[static_cast<std::size_t>(e.slotLevel - 1)] = e.slots;
            break;
        }
        }
        return result;
    }

    SpellSlots SpellSlots::forClass(CharacterClass cls, int level)
    {
        SpellSlots slots;
        slots.m_max = maxSlotsFor(cls, level);
        slots.m_current = slots.m_max;
        return slots;
    }

    int SpellSlots::max(int spellLevel) const
    {
        return validLevel(spellLevel) ? m_max[static_cast<std::size_t>(spellLevel - 1)] : 0;
    }

    int SpellSlots::current(int spellLevel) const
    {
        return validLevel(spellLevel) ? m_current[static_cast<std::size_t>(spellLevel - 1)] : 0;
    }

    bool SpellSlots::hasAny() const
    {
        return std::any_of(m_max.begin(), m_max.end(), [](int n) { return n > 0; });
    }

    bool SpellSlots::use(int spellLevel)
    {
        if (!hasSlot(spellLevel))
            return false;
        --m_current[static_cast<std::size_t>(spellLevel - 1)];
        return true;
    }

    void SpellSlots::restore(int spellLevel, int count)
    {
        if (!validLevel(spellLevel) || count <= 0)
            return;
        const auto i = static_cast<std::size_t>(spellLevel - 1);
        m_current[i] = std::min(m_current[i] + count, m_max[i]);
    }

    void SpellSlots::restoreAll()
    {
        m_current = m_max;
    }

    void SpellSlots::setMaxima(const Table &newMax)
    {
        for (std::size_t i = 0; i < m_max.size(); ++i)
        {
            const int gained = std::max(0, newMax[i] - m_max[i]);
            m_max[i] = newMax[i];
            m_current[i] = std::min(m_current[i] + gained, m_max[i]);
        }
    }

} // namespace dnd::entities
