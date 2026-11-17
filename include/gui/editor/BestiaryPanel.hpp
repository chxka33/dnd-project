#pragma once

#include "entities/Roster.hpp"
#include "scenario/ScenarioGraph.hpp"

#include <optional>
#include <string>

namespace dnd::gui::editor
{

    /// Менеджер бестиария сценария: каталог монстров кампании (имя, класс, уровень, характеристики,
    /// экипировка, AC, HP, бонус инициативы) и их привязка к сценам.
    /// Синхронизирован с экраном Characters (общие классы, каталог предметов, импорт персонажей).
    class BestiaryPanel
    {
    public:
        /// currentScene — выбранная на карте сцена (пустая строка, если ничего не выбрано).
        /// roster — группа персонажей текущей сессии (для быстрого импорта в бестиарий).
        /// Возвращает id сцены, на которую Мастер нажал в списке «Used in» (чтобы показать её на карте).
        std::optional<scenario::NodeId> draw(scenario::ScenarioGraph &graph,
                                             const scenario::NodeId &currentScene,
                                             const entities::Roster *roster = nullptr);

        void reset() { m_selected.clear(); }

    private:
        void drawImportPopup(scenario::ScenarioGraph &graph, const entities::Roster *roster);
        void drawAbilities(scenario::EnemyTemplate &enemy);
        void drawInventory(scenario::EnemyTemplate &enemy);
        void drawAddItem(scenario::EnemyTemplate &enemy);

        std::string m_selected; ///< id выбранного монстра

        // Добавление предмета в инвентарь монстра (синхронизировано с Characters)
        int m_presetIndex = 0;
        int m_presetQuantity = 1;

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

} // namespace dnd::gui::editor
