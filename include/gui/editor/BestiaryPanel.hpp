#pragma once

#include "scenario/ScenarioGraph.hpp"

#include <optional>
#include <string>

namespace dnd::gui::editor
{

    /// Менеджер бестиария сценария: каталог монстров кампании (имя, AC, HP, бонус инициативы)
    /// и их привязка к сценам.
    class BestiaryPanel
    {
    public:
        /// currentScene — выбранная на карте сцена (пустая строка, если ничего не выбрано).
        /// Возвращает id сцены, на которую Мастер нажал в списке «Used in» (чтобы показать её на карте).
        std::optional<scenario::NodeId> draw(scenario::ScenarioGraph &graph, const scenario::NodeId &currentScene);

        void reset() { m_selected.clear(); }

    private:
        std::string m_selected; ///< id выбранного монстра
    };

} // namespace dnd::gui::editor
