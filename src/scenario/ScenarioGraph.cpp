#include "scenario/ScenarioGraph.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace dnd::scenario
{

    void ScenarioGraph::addEnemy(EnemyTemplate enemy)
    {
        if (enemy.id.empty())
            throw std::invalid_argument("Enemy id must not be empty");
        for (auto &e : enemies)
        {
            if (e.id == enemy.id)
            {
                e = std::move(enemy);
                return;
            }
        }
        enemies.push_back(std::move(enemy));
    }

    bool ScenarioGraph::removeEnemy(const std::string &id)
    {
        const auto it = std::find_if(enemies.begin(), enemies.end(),
                                     [&](const EnemyTemplate &e) { return e.id == id; });
        if (it == enemies.end())
            return false;
        enemies.erase(it);
        return true;
    }

    EnemyTemplate *ScenarioGraph::findEnemy(const std::string &id)
    {
        for (auto &e : enemies)
        {
            if (e.id == id)
                return &e;
        }
        return nullptr;
    }

    const EnemyTemplate *ScenarioGraph::findEnemy(const std::string &id) const
    {
        for (const auto &e : enemies)
        {
            if (e.id == id)
                return &e;
        }
        return nullptr;
    }

    Node &ScenarioGraph::add(std::unique_ptr<Node> node)
    {
        if (!node)
            throw std::invalid_argument("ScenarioGraph: null node");

        const NodeId id = node->id();
        if (id.empty())
            throw std::invalid_argument("ScenarioGraph: node id must not be empty");
        if (m_nodes.count(id) != 0)
            throw std::invalid_argument("ScenarioGraph: duplicate node id '" + id + "'");

        Node &ref = *node;
        m_nodes.emplace(id, std::move(node));
        m_order.push_back(id);
        if (m_start.empty())
            m_start = id;
        return ref;
    }

    bool ScenarioGraph::remove(const NodeId &id)
    {
        const auto it = m_nodes.find(id);
        if (it == m_nodes.end())
            return false;

        m_nodes.erase(it);
        m_order.erase(std::remove(m_order.begin(), m_order.end(), id), m_order.end());
        if (m_start == id)
            m_start.clear();

        // Входящие связи не должны указывать в пустоту.
        for (auto &entry : m_nodes)
            entry.second->replaceTarget(id, NodeId{});
        return true;
    }

    Node *ScenarioGraph::find(const NodeId &id)
    {
        const auto it = m_nodes.find(id);
        return it == m_nodes.end() ? nullptr : it->second.get();
    }

    const Node *ScenarioGraph::find(const NodeId &id) const
    {
        const auto it = m_nodes.find(id);
        return it == m_nodes.end() ? nullptr : it->second.get();
    }

    Node &ScenarioGraph::at(const NodeId &id)
    {
        if (Node *n = find(id))
            return *n;
        throw std::out_of_range("ScenarioGraph: no node '" + id + "'");
    }

    const Node &ScenarioGraph::at(const NodeId &id) const
    {
        if (const Node *n = find(id))
            return *n;
        throw std::out_of_range("ScenarioGraph: no node '" + id + "'");
    }

    NodeId ScenarioGraph::generateId(const std::string &prefix)
    {
        NodeId id;
        do
        {
            id = prefix + "_" + std::to_string(++m_idCounter);
        } while (contains(id));
        return id;
    }

    void ScenarioGraph::setStart(const NodeId &id)
    {
        if (!contains(id))
            throw std::out_of_range("ScenarioGraph: no node '" + id + "'");
        m_start = id;
    }

    std::vector<NodeId> ScenarioGraph::reachable() const
    {
        std::vector<NodeId> result;
        if (!contains(m_start))
            return result;

        std::unordered_set<NodeId> seen{m_start};
        result.push_back(m_start);

        // result одновременно служит очередью обхода в ширину.
        for (std::size_t i = 0; i < result.size(); ++i)
        {
            for (const NodeId &next : at(result[i]).targets())
            {
                if (contains(next) && seen.insert(next).second)
                    result.push_back(next);
            }
        }
        return result;
    }

    std::vector<ValidationIssue> ScenarioGraph::validate() const
    {
        using Severity = ValidationIssue::Severity;
        std::vector<ValidationIssue> issues;
        auto report = [&](Severity s, const NodeId &node, std::string msg)
        {
            issues.push_back({s, node, std::move(msg)});
        };

        if (m_nodes.empty())
            report(Severity::Error, {}, "Сценарий пуст: нет ни одного узла.");
        else if (!contains(m_start))
            report(Severity::Error, {}, "Не задан стартовый узел.");

        for (const NodeId &id : m_order)
        {
            const Node &node = at(id);

            // Битые ссылки.
            for (const NodeId &t : node.targets())
                if (!contains(t))
                    report(Severity::Error, id, "Связь ведёт в несуществующий узел '" + t + "'.");

            // Проверка навыка.
            if (node.skillCheck && (node.skillCheck->dc < 1 || node.skillCheck->dc > 30))
                report(Severity::Warning, id, "DC проверки навыка вне диапазона 1..30.");

            // Правила конкретных типов.
            if (const auto *choice = dynamic_cast<const ChoiceNode *>(&node))
            {
                if (choice->choices().empty())
                    report(Severity::Error, id, "В развилке нет ни одного варианта выбора.");
                for (std::size_t i = 0; i < choice->choices().size(); ++i)
                {
                    const Choice &c = choice->choices()[i];
                    if (c.target.empty())
                        report(Severity::Error, id,
                               "Вариант #" + std::to_string(i + 1) + " никуда не ведёт.");
                    if (c.label.empty())
                        report(Severity::Warning, id,
                               "У варианта #" + std::to_string(i + 1) + " пустой текст.");
                }
            }
            else if (const auto *random = dynamic_cast<const RandomNode *>(&node))
            {
                if (random->encounterTarget().empty())
                    report(Severity::Error, id, "Не задан узел для сработавшего события.");
                if (random->fallbackTarget().empty())
                    report(Severity::Error, id, "Не задан узел для случая «ничего не произошло».");
            }
        }

        // Недостижимые узлы (только если старт корректен, иначе шум).
        if (contains(m_start))
        {
            const std::vector<NodeId> reach = reachable();
            const std::unordered_set<NodeId> reached(reach.begin(), reach.end());
            for (const NodeId &id : m_order)
                if (reached.count(id) == 0)
                    report(Severity::Warning, id, "Узел недостижим от стартового.");
        }

        return issues;
    }

    bool ScenarioGraph::isPlayable(const std::vector<ValidationIssue> &issues)
    {
        return std::none_of(issues.begin(), issues.end(), [](const ValidationIssue &i)
                            { return i.severity == ValidationIssue::Severity::Error; });
    }

} // namespace dnd::scenario
