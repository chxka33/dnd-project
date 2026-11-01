#pragma once

#include "scenario/ScenarioGraph.hpp"

#include <cstddef>
#include <string>
#include <vector>

/// Чистая логика редактора сценариев (модуль 2.2): без ImGui и SFML.
/// Всё, что меняет ScenarioGraph по действиям Мастера, лежит здесь — так её можно
/// покрыть юнит-тестами, а GUI-код остаётся тонким.
namespace dnd::gui::editor
{

    /// Размер узла на холсте в мировых координатах (до масштабирования).
    constexpr float kNodeWidth = 190.f;
    constexpr float kNodeHeight = 130.f;

    /// Одна стрелка перехода на карте.
    struct Link
    {
        enum class Kind
        {
            Story,          ///< StoryNode::next
            Choice,         ///< ChoiceNode::choices()[index].target
            Random,         ///< RandomNode::entries()[index].target
            RandomFallback  ///< RandomNode::fallbackTarget() — «ничего не выпало»
        };

        scenario::NodeId from;
        scenario::NodeId to;
        Kind kind = Kind::Story;
        std::size_t index = 0; ///< номер варианта / строки таблицы
        std::string label;     ///< подпись на стрелке (текст варианта, диапазон кубика)

        bool sameAs(const Link &o) const
        {
            return from == o.from && to == o.to && kind == o.kind && index == o.index;
        }
    };

    /// Все непустые исходящие связи сценария (в порядке узлов графа).
    std::vector<Link> collectLinks(const scenario::ScenarioGraph &graph);

    /// Создаёт связь from -> to в зависимости от типа исходного узла:
    ///  Story — заменяет next; Choice — добавляет новый вариант; Random — новую строку таблицы.
    /// false, если узлы не существуют или from == to.
    bool connect(scenario::ScenarioGraph &graph, const scenario::NodeId &from, const scenario::NodeId &to);

    /// Убирает связь (обнуляет цель). Сам вариант / строка таблицы остаются — Мастер может
    /// перенацелить их в инспекторе. false, если связь не найдена.
    bool disconnect(scenario::ScenarioGraph &graph, const Link &link);

    /// Создаёт узел нужного типа в позиции pos (мировые координаты) и возвращает его.
    scenario::Node &createNode(scenario::ScenarioGraph &graph, scenario::NodeType type,
                               scenario::EditorPosition pos);

    /// Удаляет узел вместе с входящими связями. Если удалён стартовый узел, стартом становится
    /// первый из оставшихся (иначе сценарий молча становится «неиграбельным»).
    bool removeNode(scenario::ScenarioGraph &graph, const scenario::NodeId &id);

    /// Первое значение кубика, не занятое ни одной строкой таблицы (не больше diceSides).
    int nextFreeRoll(const scenario::RandomNode &node);

    /// Сколько значений кубика 1..diceSides покрыто таблицей и сколько покрыто более одного раза.
    struct RollCoverage
    {
        int covered = 0;
        int overlapping = 0;
    };
    RollCoverage rollCoverage(const scenario::RandomNode &node);

    /// Уникальный id для нового монстра бестиария: "goblin", "goblin_2", ... ("enemy" для не-ASCII имён).
    std::string makeEnemyId(const scenario::ScenarioGraph &graph, const std::string &name);

    /// Удаляет монстра из бестиария и открепляет его от всех сцен.
    bool removeEnemyEverywhere(scenario::ScenarioGraph &graph, const std::string &enemyId);

    /// Прикрепляет монстра к сцене (без дублей). false — нет такого узла/монстра или уже прикреплён.
    bool attachEnemy(scenario::ScenarioGraph &graph, const scenario::NodeId &nodeId, const std::string &enemyId);
    bool detachEnemy(scenario::ScenarioGraph &graph, const scenario::NodeId &nodeId, const std::string &enemyId);

    /// Сколько сцен ссылаются на монстра.
    std::size_t enemyUsageCount(const scenario::ScenarioGraph &graph, const std::string &enemyId);

    /// Раскладывает узлы по слоям (BFS от стартового узла): колонка = глубина, строка = порядок обхода.
    /// Недостижимые узлы уходят в отдельную колонку справа.
    void autoLayout(scenario::ScenarioGraph &graph);

} // namespace dnd::gui::editor
