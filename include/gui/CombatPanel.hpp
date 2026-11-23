#pragma once

#include "combat/CheckResolver.hpp"
#include "combat/InitiativeTracker.hpp"
#include "entities/Roster.hpp"
#include "scenario/ScenarioGraph.hpp"
#include "utils/Random.hpp"

#include <optional>
#include <string>
#include <vector>

namespace dnd::gui
{

    /// Панель модуля 3.3: калькулятор проверок/спасбросков и трекер инициативы.
    /// Рисуется внутри уже открытого окна ImGui. Результаты копятся в takeEvents() для журнала сессии.
    class CombatPanel
    {
    public:
        combat::InitiativeTracker &tracker() { return m_tracker; }
        const combat::InitiativeTracker &tracker() const { return m_tracker; }

        /// Текущий сценарий и узел (оба могут быть nullptr). При смене узла с испытанием
        /// параметры испытания подставляются в калькулятор.
        void setSceneContext(const scenario::ScenarioGraph *graph, const scenario::Node *node);

        /// Возвращает индекс героя в roster, чей лист хотят открыть (кнопка "Sheet"), или -1.
        int draw(entities::Roster &roster, utils::Random &rng);

        /// Сообщения для журнала сессии (читает и очищает).
        std::vector<std::string> takeEvents();

        const std::optional<combat::CheckOutcome> &lastCheckOutcome() const { return m_lastOutcome; }

    private:
        void drawCheckResolver(entities::Roster &roster, utils::Random &rng);
        void drawInitiativeTracker(entities::Roster &roster, utils::Random &rng, int &openSheet);
        void useSceneCheck();
        scenario::CheckDefinition currentCheck() const;
        std::optional<bool> proficiencyOverride() const;
        void pushEvent(std::string text) { m_events.push_back(std::move(text)); }

        combat::InitiativeTracker m_tracker;

        const scenario::ScenarioGraph *m_graph = nullptr;
        const scenario::Node *m_node = nullptr;

        // калькулятор
        int m_checkType = 0; ///< 0 — проверка, 1 — спасбросок
        int m_ability = 0;   ///< индекс в entities::kAllAbilities
        std::string m_skill;
        bool m_skillProficient = false;
        int m_dc = 10;
        int m_extraBonus = 0;
        int m_roll = 10;
        int m_target = 0; ///< индекс героя в Roster
        std::optional<combat::CheckOutcome> m_lastOutcome;

        // трекер
        int m_amount = 5;
        int m_enemyPick = 0;

        std::vector<std::string> m_events;
    };

} // namespace dnd::gui
