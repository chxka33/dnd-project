#pragma once

#include "combat/InitiativeTracker.hpp"
#include "entities/Roster.hpp"
#include "scenario/ScenarioGraph.hpp"
#include "scenario/SessionSerializer.hpp"
#include "utils/Random.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/// Модуль 3.4: логика интерактивного симулятора сессии. Без ImGui и SFML —
/// так её можно покрыть юнит-тестами, а экран SimulatorState остаётся тонким.
namespace dnd::simulator
{

    /// Ведение кампании: текущий узел нелинейного сюжета, переходы, броски по таблицам
    /// случайных встреч, выдача наград, журнал сессии и сохранение прогресса (.dndsav).
    class SessionRunner
    {
    public:
        // --- сценарий -------------------------------------------------------
        /// Загружает .dndsc и ставит партию в стартовый узел.
        bool loadScenario(const std::filesystem::path &filePath, std::string *outError = nullptr);
        /// Задаёт сценарий из памяти (тесты, новое прохождение).
        void setScenario(scenario::ScenarioGraph graph, std::string filePath = {});
        /// Возвращает партию в стартовый узел и сбрасывает последний бросок.
        bool restartScenario();

        bool hasScenario() const { return m_hasScenario; }
        const scenario::ScenarioGraph &graph() const { return m_graph; }
        scenario::ScenarioGraph &graph() { return m_graph; }
        const std::string &scenarioPath() const { return m_scenarioPath; }

        // --- текущий узел и навигация ---------------------------------------
        const scenario::NodeId &currentNodeId() const { return m_currentNodeId; }
        const scenario::Node *currentNode() const;
        scenario::Node *currentNode();

        /// true, если сценарий загружен, а идти дальше некуда (нет узла или финал Story-узла).
        bool isAtEnd() const;

        /// Переход в любой существующий узел (в том числе принудительный выбор Мастера).
        bool moveToNode(const scenario::NodeId &targetId, std::string reason = {});
        /// Story: переход к следующему узлу. false — это финал ветки или связь битая.
        bool advanceStory();
        /// Choice: переход по выбранному игроками варианту.
        bool chooseOption(std::size_t choiceIndex);

        // --- таблица случайных встреч ---------------------------------------
        /// Random: бросок кубика за Мастера. autoFollow — сразу перейти в выпавший узел.
        std::optional<scenario::RollResult> rollRandomEncounter(utils::Random &rng, bool autoFollow = true);
        /// Переход в узел, выпавший при последнем броске (если бросали без autoFollow).
        bool followLastRoll();
        const std::optional<scenario::RollResult> &lastRollResult() const { return m_lastRoll; }

        // --- награды и журнал ------------------------------------------------
        /// Выдаёт награду текущего узла герою. Нет награды или неверный индекс — пустой результат.
        entities::LootResult giveCurrentNodeLoot(entities::Roster &roster, std::size_t rosterIndex);

        void logEvent(std::string message);
        void appendEvents(const std::vector<std::string> &events);
        const std::vector<std::string> &eventLog() const { return m_eventLog; }
        void clearLog() { m_eventLog.clear(); }

        // --- сохранение прогресса (.dndsav) ---------------------------------
        scenario::SessionSaveData exportSaveData(const entities::Roster &roster,
                                                 const combat::InitiativeTracker &tracker) const;
        bool applySaveData(const scenario::SessionSaveData &data, entities::Roster &roster,
                           combat::InitiativeTracker &tracker, std::string *outError = nullptr);
        bool saveSessionToFile(const std::filesystem::path &filePath, const entities::Roster &roster,
                               const combat::InitiativeTracker &tracker, std::string *outError = nullptr) const;
        bool loadSessionFromFile(const std::filesystem::path &filePath, entities::Roster &roster,
                                 combat::InitiativeTracker &tracker, std::string *outError = nullptr);

    private:
        std::string sceneName(const scenario::NodeId &id) const;
        void enterStart();

        scenario::ScenarioGraph m_graph;
        bool m_hasScenario = false;
        std::string m_scenarioPath;
        scenario::NodeId m_currentNodeId;
        std::optional<scenario::RollResult> m_lastRoll;
        std::vector<std::string> m_eventLog;
    };

} // namespace dnd::simulator
