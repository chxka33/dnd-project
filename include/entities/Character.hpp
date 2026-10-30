#pragma once

#include "entities/AbilityScores.hpp"
#include "entities/CharacterClass.hpp"
#include "entities/Inventory.hpp"
#include "entities/SpellSlots.hpp"

#include <optional>
#include <string>

namespace dnd::entities
{

    /// Персонаж игрока (или противника): класс, уровень, характеристики, инвентарь и
    /// всё, что из них автоматически рассчитывается — модификаторы, бонус владения,
    /// КД, максимум HP, ячейки заклинаний.
    class Character
    {
    public:
        static constexpr int kMinLevel = 1;
        static constexpr int kMaxLevel = 20;

        /// Создаёт персонажа с полным здоровьем и полными ячейками заклинаний.
        /// level обрезается до [1, 20].
        Character(std::string name, CharacterClass cls, int level = 1,
                  AbilityScores abilities = AbilityScores());

        // --- Основные данные -------------------------------------------------
        std::string name;

        CharacterClass characterClass() const { return m_class; }
        const ClassInfo &classData() const { return classInfo(m_class); }
        int level() const { return m_level; }

        /// Меняет уровень (обрезается до 1..20): пересчитывает максимум HP и ячейки заклинаний.
        /// При повышении текущие HP растут на прирост максимума; при понижении — обрезаются.
        void setLevel(int level);
        /// +1 уровень. false, если уже достигнут максимальный.
        bool levelUp();

        AbilityScores &abilities() { return m_abilities; }
        const AbilityScores &abilities() const { return m_abilities; }
        Inventory &inventory() { return m_inventory; }
        const Inventory &inventory() const { return m_inventory; }

        // --- Производные характеристики -------------------------------------
        int modifier(Ability ability) const { return m_abilities.modifier(ability); }

        /// Бонус владения: 2 + (уровень - 1) / 4  (+2 на 1-4, +6 на 17-20).
        int proficiencyBonus() const { return proficiencyBonusForLevel(m_level); }
        static int proficiencyBonusForLevel(int level);

        bool isProficientInSave(Ability ability) const;
        /// Модификатор спасброска: модификатор характеристики (+ бонус владения, если владеет).
        int savingThrowBonus(Ability ability) const;

        /// Модификатор инициативы = модификатор Ловкости.
        int initiativeModifier() const { return modifier(Ability::Dexterity); }

        /// Класс Доспеха с учётом надетого доспеха, щита, «Защиты без доспехов»
        /// (варвар: 10 + ЛОВ + ТЕЛ; монах без щита: 10 + ЛОВ + МДР) и прочих бонусов.
        int armorClass() const;
        /// Прочий бонус к КД (магия, умения, прикрытие) — прибавляется к рассчитанному КД.
        int miscArmorClassBonus() const { return m_miscAcBonus; }
        void setMiscArmorClassBonus(int bonus) { m_miscAcBonus = bonus; }

        /// Грузоподъёмность и перегруз по текущей Силе.
        double carryCapacity() const { return Inventory::carryCapacity(m_abilities.score(Ability::Strength)); }
        bool isEncumbered() const { return m_inventory.totalWeight() > carryCapacity(); }

        // --- Здоровье --------------------------------------------------------
        /// Максимум HP: на 1-м уровне кость хитов + мод. ТЕЛ, далее среднее значение кости
        /// (кость/2 + 1) + мод. ТЕЛ за каждый уровень, но не менее 1 за уровень.
        /// Пересчитывается автоматически при изменении Телосложения.
        int maxHp() const;
        /// Текущие HP (никогда не выше maxHp()).
        int currentHp() const;
        int temporaryHp() const { return m_tempHp; }
        void setCurrentHp(int hp);

        /// Наносит урон: сначала снимаются временные HP, затем обычные (не ниже 0).
        /// Возвращает, сколько обычных HP потеряно. Отрицательный урон игнорируется.
        int applyDamage(int amount);
        /// Лечит (не выше максимума). Возвращает, на сколько HP реально выросло здоровье.
        /// Отрицательное лечение игнорируется.
        int heal(int amount);
        /// Временные HP не суммируются: берётся большее из старого и нового значения.
        void grantTemporaryHp(int amount);

        /// 0 HP — персонаж без сознания / повержен.
        bool isDown() const { return currentHp() <= 0; }
        int hitDieSize() const { return classData().hitDie; }

        // --- Заклинания -------------------------------------------------------
        bool isSpellcaster() const { return classData().caster != CasterType::None; }
        SpellSlots &spellSlots() { return m_slots; }
        const SpellSlots &spellSlots() const { return m_slots; }
        /// Сл спасброска от заклинаний = 8 + бонус владения + мод. ключевой характеристики.
        /// nullopt, если класс не колдует.
        std::optional<int> spellSaveDc() const;
        /// Бонус атаки заклинанием = бонус владения + мод. ключевой характеристики.
        std::optional<int> spellAttackBonus() const;

        // --- Пассивные показатели ----------------------------------------------
        /// Пассивная внимательность: 10 + модификатор Мудрости.
        int passivePerception() const;

        // --- Состояния (Conditions) --------------------------------------------
        const std::vector<std::string> &conditions() const { return m_conditions; }
        std::vector<std::string> &conditions() { return m_conditions; }
        void addCondition(std::string condition);
        void removeCondition(const std::string &condition);
        bool hasCondition(const std::string &condition) const;
        void clearConditions();

        // --- Отдых -------------------------------------------------------------
        /// Длинный отдых: полные HP, все ячейки заклинаний, временные HP сбрасываются.
        void longRest();
        /// Короткий отдых: колдун возвращает ячейки магии договора. Остальное восстанавливает Мастер/игрок вручную.
        void shortRest();

    private:
        void refreshDerived(int previousMaxHp);

        CharacterClass m_class;
        int m_level;
        AbilityScores m_abilities;
        Inventory m_inventory;
        SpellSlots m_slots;
        int m_currentHp = 0;
        int m_tempHp = 0;
        int m_miscAcBonus = 0;
        std::vector<std::string> m_conditions;
    };

} // namespace dnd::entities
