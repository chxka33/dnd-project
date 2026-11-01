#pragma once

#include "entities/Roster.hpp"

#include <string>

namespace dnd::gui
{

    /// Интерактивный лист персонажа: имя и уровень, характеристики, КД, здоровье,
    /// ячейки заклинаний и инвентарь. Рисуется внутри уже открытого окна ImGui.
    ///
    /// Два режима:
    ///  - Setup — подготовка к игре (экран Characters): правка имени, уровня, характеристик,
    ///    снаряжения; здоровье и ячейки показываются только как максимум, без урона/лечения/отдыха.
    ///    Персонаж всегда остаётся «полным» (при росте максимума HP растут и текущие).
    ///  - Play — во время игры (симулятор): добавляются урон, лечение, временные HP, отдыхи,
    ///    трата ячеек и использование расходников.
    class CharacterSheetPanel
    {
    public:
        enum class Mode
        {
            Setup,
            Play
        };

        explicit CharacterSheetPanel(Mode mode = Mode::Play) : m_mode(mode) {}

        /// Выбирает персонажа по индексу в Roster (значение корректируется при отрисовке).
        void select(int index) { m_selected = index; }
        int selected() const { return m_selected; }

        void draw(entities::Roster &roster);

    private:
        void drawHeader(entities::Roster &roster, entities::Character &c);
        void drawAbilities(entities::Character &c);
        void drawCombatStats(entities::Character &c);
        void drawHealth(entities::Character &c);
        void drawSpells(entities::Character &c);
        void drawInventory(entities::Character &c);
        void drawAddItem(entities::Character &c);

        Mode m_mode;
        int m_selected = 0;
        int m_lastSelected = -1;
        bool m_removeRequested = false;

        // Здоровье
        int m_amount = 1;
        std::string m_status;

        // Добавление предмета: каталог
        int m_presetIndex = 0;
        int m_presetQuantity = 1;

        // Добавление предмета: свой предмет
        std::string m_customName;
        std::string m_customDescription;
        int m_customType = 0;
        int m_customQuantity = 1;
        float m_customWeight = 0.f;
        std::string m_customDamage = "1d6";
        std::string m_customDamageType = "slashing";
        bool m_customFinesse = false;
        bool m_customRanged = false;
        int m_customArmorCategory = 0;
        int m_customBaseAc = 11;
        int m_customStrReq = 0;
        bool m_customStealth = false;
        int m_customShieldBonus = 2;
    };

} // namespace dnd::gui
