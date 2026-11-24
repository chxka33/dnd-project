#pragma once

#include "core/State.hpp"
#include "gui/CharacterSheetPanel.hpp"
#include "gui/CombatPanel.hpp"
#include "gui/DmSheetPanel.hpp"
#include "gui/editor/ScenarioCanvas.hpp"
#include "simulator/SessionRunner.hpp"
#include "utils/Random.hpp"

#include <cstddef>
#include <string>

#include <imgui.h>

namespace dnd::core
{

    /// Экран «Game Simulator» (модуль 3.4): ведение живой сессии за Мастера.
    ///
    /// Слева — карта прохождения (подсвечен узел партии) и журнал сессии, справа вкладки:
    ///   Scene          — текст сцены, заметки Мастера, награды, враги и переходы по сюжету;
    ///   DM Party Sheet — сводная ширма партии (модуль 3.2);
    ///   Combat         — проверки, спасброски и трекер инициативы (модуль 3.3);
    ///   Character      — подробный лист выбранного персонажа.
    /// Сохранение и загрузка прогресса идут через SessionRunner (формат .dndsav).
    class SimulatorState final : public State
    {
    public:
        void onEnter() override;
        void onExit() override;

        void handleEvent(const sf::Event &event) override;
        void update(sf::Time) override {}
        void drawUi() override;
        void render(sf::RenderWindow &) override {}

    private:
        void drawToolbar();
        void drawMapAndLog(const ImVec2 &size);
        void drawRightPanel(const ImVec2 &size);
        void drawSceneTab();
        void drawNavigation(const scenario::Node &node);
        void drawLootBlock(const scenario::Node &node);
        void drawLog();
        void drawModals();

        void applyScale() const;
        void setStatus(std::string text, bool error);
        void flushCombatEvents();

        simulator::SessionRunner m_runner;
        utils::Random m_rng;

        gui::editor::ScenarioCanvas m_canvas;
        gui::DmSheetPanel m_dmSheet;
        gui::CombatPanel m_combat;
        gui::CharacterSheetPanel m_sheet{gui::CharacterSheetPanel::Mode::Play};

        char m_sessionName[128] = "session";
        std::string m_note;
        std::string m_status;
        bool m_statusIsError = false;

        bool m_openLoadScenario = false;
        bool m_openLoadParty = false;
        bool m_openSaveSession = false;
        bool m_openLoadSession = false;

        bool m_showDmNotes = true;
        int m_lootTarget = 0;   ///< индекс героя в Roster для кнопки «Give loot»
        int m_focusTab = -1;    ///< 0 Scene, 1 Party, 2 Combat, 3 Character
        std::size_t m_logShown = 0;

        float m_scale = 1.5f;
        float m_previousScale = 1.0f;
    };

} // namespace dnd::core
