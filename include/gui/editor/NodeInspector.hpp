#pragma once

#include "scenario/ScenarioGraph.hpp"

#include <string>

namespace dnd::gui::editor
{

    /// Инспектор параметров выбранной сцены (правая панель редактора):
    /// тексты для игроков и заметки Мастера, испытание (проверка / спасбросок + DC),
    /// награды, переходы (по типу узла), таблица случайных встреч, прикреплённые монстры.
    class NodeInspector
    {
    public:
        /// Рисует форму узла. Возвращает true, если Мастер нажал «Delete scene»
        /// (само удаление и подтверждение — на стороне вызывающего).
        bool draw(scenario::ScenarioGraph &graph, scenario::Node &node);

    private:
        void drawChallenge(scenario::Node &node);
        void drawLoot(scenario::Node &node);
        void drawStoryTransitions(scenario::ScenarioGraph &graph, scenario::StoryNode &node);
        void drawChoiceTransitions(scenario::ScenarioGraph &graph, scenario::ChoiceNode &node);
        void drawRandomTable(scenario::ScenarioGraph &graph, scenario::RandomNode &node);
        void drawEnemies(scenario::ScenarioGraph &graph, scenario::Node &node);

        // Добавление предмета в награду сцены (синхронизировано с каталогом Characters)
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
