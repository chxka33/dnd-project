#include "simulator/SessionRunner.hpp"

#include "scenario/ChoiceNode.hpp"
#include "scenario/RandomNode.hpp"
#include "scenario/ScenarioSerializer.hpp"
#include "scenario/StoryNode.hpp"
#include "utils/ExecutablePath.hpp"

#include <system_error>
#include <utility>

namespace dnd::simulator
{
    using namespace dnd::scenario;

    // ------------------------------------------------------------------ сценарий

    std::string SessionRunner::sceneName(const NodeId &id) const
    {
        const Node *node = m_graph.find(id);
        if (!node)
            return id;
        return node->title.empty() ? id : node->title;
    }

    void SessionRunner::enterStart()
    {
        m_lastRoll.reset();
        m_currentNodeId = m_graph.contains(m_graph.startId()) ? m_graph.startId() : NodeId{};
        if (!m_currentNodeId.empty())
            logEvent("Entered scene: " + sceneName(m_currentNodeId));
        else
            logEvent("The scenario has no start scene.");
    }

    bool SessionRunner::loadScenario(const std::filesystem::path &filePath, std::string *outError)
    {
        std::unique_ptr<ScenarioGraph> loaded = ScenarioSerializer::loadFromFile(filePath, outError);
        if (!loaded)
            return false;
        m_graph = std::move(*loaded);
        m_hasScenario = true;
        m_scenarioPath = filePath.string();
        logEvent("Scenario loaded: " + (m_graph.title.empty() ? std::string("(untitled)") : m_graph.title));
        enterStart();
        return true;
    }

    void SessionRunner::setScenario(ScenarioGraph graph, std::string filePath)
    {
        m_graph = std::move(graph);
        m_hasScenario = true;
        m_scenarioPath = std::move(filePath);
        logEvent("Scenario loaded: " + (m_graph.title.empty() ? std::string("(untitled)") : m_graph.title));
        enterStart();
    }

    bool SessionRunner::restartScenario()
    {
        if (!m_hasScenario)
            return false;
        logEvent("Scenario restarted.");
        enterStart();
        return !m_currentNodeId.empty();
    }

    // ------------------------------------------------------------------ навигация

    const Node *SessionRunner::currentNode() const
    {
        return m_currentNodeId.empty() ? nullptr : m_graph.find(m_currentNodeId);
    }

    Node *SessionRunner::currentNode()
    {
        return m_currentNodeId.empty() ? nullptr : m_graph.find(m_currentNodeId);
    }

    bool SessionRunner::isAtEnd() const
    {
        if (!m_hasScenario)
            return false;
        const Node *node = currentNode();
        if (!node)
            return true;
        if (node->type() == NodeType::Story)
            return static_cast<const StoryNode *>(node)->isEnding();
        return false;
    }

    bool SessionRunner::moveToNode(const NodeId &targetId, std::string reason)
    {
        if (!m_graph.contains(targetId))
            return false;
        m_currentNodeId = targetId;
        m_lastRoll.reset();
        std::string text = "Entered scene: " + sceneName(targetId);
        if (!reason.empty())
            text += " (" + reason + ")";
        logEvent(std::move(text));
        return true;
    }

    bool SessionRunner::advanceStory()
    {
        const Node *node = currentNode();
        if (!node || node->type() != NodeType::Story)
            return false;
        const auto *story = static_cast<const StoryNode *>(node);
        if (story->isEnding())
        {
            logEvent("The story branch ends at: " + sceneName(node->id()));
            return false;
        }
        if (!m_graph.contains(story->next()))
        {
            logEvent("Broken link from: " + sceneName(node->id()));
            return false;
        }
        return moveToNode(story->next());
    }

    bool SessionRunner::chooseOption(std::size_t choiceIndex)
    {
        const Node *node = currentNode();
        if (!node || node->type() != NodeType::Choice)
            return false;
        const auto *choice = static_cast<const ChoiceNode *>(node);
        if (choiceIndex >= choice->choices().size())
            return false;
        const Choice &picked = choice->choices()[choiceIndex];
        if (picked.target.empty() || !m_graph.contains(picked.target))
            return false;
        logEvent("Choice: " + picked.label + " -> " + sceneName(picked.target));
        return moveToNode(picked.target);
    }

    // ------------------------------------------------------------------ случайные встречи

    std::optional<RollResult> SessionRunner::rollRandomEncounter(utils::Random &rng, bool autoFollow)
    {
        const Node *node = currentNode();
        if (!node || node->type() != NodeType::Random)
            return std::nullopt;
        const RollResult result = static_cast<const RandomNode *>(node)->roll(rng);
        m_lastRoll = result;
        logEvent("Roll table (d" + std::to_string(result.diceSides) + "): rolled " + std::to_string(result.roll) +
                 " -> " + result.label);
        if (autoFollow)
            followLastRoll();
        return result;
    }

    bool SessionRunner::followLastRoll()
    {
        if (!m_lastRoll || m_lastRoll->next.empty() || !m_graph.contains(m_lastRoll->next))
            return false;
        return moveToNode(m_lastRoll->next, "random table");
    }

    // ------------------------------------------------------------------ награды и журнал

    entities::LootResult SessionRunner::giveCurrentNodeLoot(entities::Roster &roster, std::size_t rosterIndex)
    {
        const Node *node = currentNode();
        if (!node || !node->loot || rosterIndex >= roster.size())
            return {};
        entities::Character &hero = roster.at(rosterIndex).character;
        const entities::LootResult got = hero.receiveLoot(*node->loot);
        logEvent("Loot given to " + hero.name + ": " + std::to_string(got.gold) + " gp, " +
                 std::to_string(got.itemUnits) + " item(s).");
        return got;
    }

    void SessionRunner::logEvent(std::string message)
    {
        if (!message.empty())
            m_eventLog.push_back(std::move(message));
    }

    void SessionRunner::appendEvents(const std::vector<std::string> &events)
    {
        for (const std::string &e : events)
            logEvent(e);
    }

    // ------------------------------------------------------------------ сохранение

    SessionSaveData SessionRunner::exportSaveData(const entities::Roster &roster,
                                                  const combat::InitiativeTracker &tracker) const
    {
        SessionSaveData data;
        data.scenarioFilePath = m_scenarioPath;
        data.currentNodeId = m_currentNodeId;
        data.roster = roster;
        data.eventLog = m_eventLog;
        data.combatRound = tracker.round();
        data.activeTurnIndex = tracker.activeTurnIndex();
        return data;
    }

    bool SessionRunner::applySaveData(const SessionSaveData &data, entities::Roster &roster,
                                      combat::InitiativeTracker &tracker, std::string *outError)
    {
        // Сценарий: подгружаем файл из сохранения, если он указан и отличается от текущего.
        if (!data.scenarioFilePath.empty() && (!m_hasScenario || data.scenarioFilePath != m_scenarioPath))
        {
            std::error_code ec;
            std::filesystem::path path(data.scenarioFilePath);
            if (!std::filesystem::exists(path, ec))
                path = utils::executableRelativePath(path.filename());
            std::string error;
            if (loadScenario(path, &error))
            {
                m_scenarioPath = data.scenarioFilePath;
            }
            else if (!m_hasScenario)
            {
                if (outError)
                    *outError = "Cannot load scenario of the session: " + error;
                return false;
            }
        }

        roster = data.roster;
        m_eventLog = data.eventLog;
        m_lastRoll.reset();
        m_currentNodeId = m_graph.contains(data.currentNodeId) ? data.currentNodeId
                          : (m_graph.contains(m_graph.startId()) ? m_graph.startId() : NodeId{});

        tracker.clear();
        for (std::size_t i = 0; i < roster.size(); ++i)
            tracker.addFromRoster(roster, i, 10 + roster.at(i).character.initiativeModifier());
        tracker.setRound(data.combatRound);
        tracker.setActiveTurnIndex(data.activeTurnIndex);
        return true;
    }

    bool SessionRunner::saveSessionToFile(const std::filesystem::path &filePath, const entities::Roster &roster,
                                          const combat::InitiativeTracker &tracker, std::string *outError) const
    {
        return SessionSerializer::saveToFile(exportSaveData(roster, tracker), filePath, outError);
    }

    bool SessionRunner::loadSessionFromFile(const std::filesystem::path &filePath, entities::Roster &roster,
                                            combat::InitiativeTracker &tracker, std::string *outError)
    {
        SessionSaveData data;
        if (!SessionSerializer::loadFromFile(filePath, data, outError))
            return false;
        return applySaveData(data, roster, tracker, outError);
    }

} // namespace dnd::simulator
