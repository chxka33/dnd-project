#pragma once

#include "core/State.hpp"
#include "gui/editor/BestiaryPanel.hpp"
#include "gui/editor/NodeInspector.hpp"
#include "gui/editor/ScenarioCanvas.hpp"
#include "scenario/ScenarioGraph.hpp"

#include <string>
#include <vector>

namespace dnd::core
{

    /// Экран «Scenario Editor» (модуль 2.2): визуальный редактор нелинейных сценариев.
    ///
    /// Слева — интерактивная карта приключения (gui::editor::ScenarioCanvas), справа вкладки:
    ///   Scene    — инспектор выбранной сцены (тексты, заметки Мастера, испытания, награды, таблицы);
    ///   Bestiary — каталог монстров сценария;
    ///   Scenario — название сценария, стартовая сцена и результаты проверки целостности.
    /// Сохранение и загрузка идут через scenario::ScenarioSerializer (формат .dndsc, модуль 1.3).
    class EditorState final : public State
    {
    public:
        void onEnter() override;
        void onExit() override;

        void handleEvent(const sf::Event &event) override;
        void update(sf::Time) override {}
        void drawUi() override;
        void render(sf::RenderWindow &) override {}

    private:
        /// Действие, которое нужно выполнить после решения по несохранённым изменениям.
        enum class Pending
        {
            None,
            Exit,
            New,
            Load
        };

        void drawToolbar();
        void drawSidePanel(const ImVec2 &size);
        void drawSceneTab();
        void drawScenarioTab();
        void drawModals();

        void applyScale() const;
        void requestAction(Pending action);
        void performAction(Pending action);
        bool saveTo(const std::string &path);
        void requestSave();
        bool canQuickSave() const;
        void loadScenario();
        void resetScenario();
        void setStatus(std::string text, bool error);

        std::string snapshot() const;
        bool isDirty() const;

        scenario::ScenarioGraph m_graph;
        gui::editor::ScenarioCanvas m_canvas;
        gui::editor::NodeInspector m_inspector;
        gui::editor::BestiaryPanel m_bestiary;
        std::vector<scenario::ValidationIssue> m_issues;

        std::string m_path = "scenarios/new_scenario.dndsc"; ///< то, что введено в поле «File»
        std::string m_documentPath;                          ///< файл открытого сценария ("" — ещё не сохранён)
        std::string m_savedSnapshot;
        std::string m_status;
        bool m_statusIsError = false;

        Pending m_pending = Pending::None;
        bool m_openUnsavedPopup = false;
        bool m_openOverwritePopup = false;
        bool m_openDeletePopup = false;
        scenario::NodeId m_deleteTarget;
        bool m_focusSceneTab = false;

        float m_scale = 1.5f; ///< масштаб шрифта этого экрана
        float m_previousScale = 1.0f;
    };

} // namespace dnd::core
