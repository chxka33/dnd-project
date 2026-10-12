#pragma once

#include "scenario/ChoiceNode.hpp"
#include "scenario/Node.hpp"
#include "scenario/RandomNode.hpp"
#include "scenario/StoryNode.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dnd::scenario
{

    /// Замечание, найденное при проверке сценария (показывается Мастеру в редакторе).
    struct ValidationIssue
    {
        enum class Severity
        {
            Warning, ///< сценарий работает, но, скорее всего, это ошибка автора
            Error    ///< сценарий нельзя проиграть корректно
        };

        Severity severity = Severity::Error;
        NodeId node; ///< проблемный узел (пусто — проблема всего сценария)
        std::string message;
    };

    /// Граф сценария: владеет узлами и знает стартовый узел.
    /// Узлы хранятся в порядке добавления (стабильный порядок для редактора и .dndsc).
    class ScenarioGraph
    {
    public:
        std::string title;
        std::string description;

        // ------------------------------------------------------------ узлы
        /// Добавляет узел. Пустой или повторяющийся id -> std::invalid_argument.
        /// Первый добавленный узел автоматически становится стартовым.
        Node &add(std::unique_ptr<Node> node);

        /// Создаёт узел типа T на месте: graph.emplace<StoryNode>("intro", "Таверна").
        template <class T, class... Args>
        T &emplace(NodeId id, Args &&...args)
        {
            auto node = std::make_unique<T>(std::move(id), std::forward<Args>(args)...);
            T &ref = *node;
            add(std::move(node));
            return ref;
        }

        /// Удаляет узел; входящие связи на него обнуляются, стартовый узел сбрасывается.
        bool remove(const NodeId &id);

        bool contains(const NodeId &id) const { return m_nodes.count(id) != 0; }
        Node *find(const NodeId &id);
        const Node *find(const NodeId &id) const;
        Node &at(const NodeId &id);             ///< нет узла -> std::out_of_range
        const Node &at(const NodeId &id) const; ///< нет узла -> std::out_of_range

        std::size_t size() const { return m_nodes.size(); }
        bool empty() const { return m_nodes.empty(); }
        const std::vector<NodeId> &order() const { return m_order; } ///< порядок добавления

        /// Свободный идентификатор вида "prefix_1", "prefix_2", ...
        NodeId generateId(const std::string &prefix = "node");

        // ------------------------------------------------------------ старт
        const NodeId &startId() const { return m_start; }
        void setStart(const NodeId &id); ///< нет такого узла -> std::out_of_range
        Node *start() { return find(m_start); }
        const Node *start() const { return find(m_start); }

        // ------------------------------------------------------------ анализ
        /// Узлы, достижимые от стартового (обход в ширину, в порядке обнаружения).
        std::vector<NodeId> reachable() const;

        /// Проверка целостности: старт, битые ссылки, пустые развилки, недостижимые узлы...
        std::vector<ValidationIssue> validate() const;

        /// true, если среди замечаний нет ни одной ошибки (предупреждения допустимы).
        static bool isPlayable(const std::vector<ValidationIssue> &issues);

    private:
        std::unordered_map<NodeId, std::unique_ptr<Node>> m_nodes;
        std::vector<NodeId> m_order;
        NodeId m_start;
        unsigned m_idCounter = 0;
    };

} // namespace dnd::scenario
