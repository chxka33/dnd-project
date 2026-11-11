#pragma once

#include "scenario/ScenarioGraph.hpp"

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
    };

} // namespace dnd::gui::editor
