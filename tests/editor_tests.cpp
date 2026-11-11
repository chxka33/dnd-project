#include <catch2/catch_test_macros.hpp>

#include "gui/editor/EditorOps.hpp"
#include "scenario/ChoiceNode.hpp"
#include "scenario/RandomNode.hpp"
#include "scenario/ScenarioGraph.hpp"
#include "scenario/StoryNode.hpp"

using namespace dnd::scenario;
using namespace dnd::gui::editor;

namespace
{
    /// start(Story) -> fork(Choice) -> {a, b}(Story), roll(Random) отдельно.
    struct Fixture
    {
        ScenarioGraph graph;
        Fixture()
        {
            createNode(graph, NodeType::Story, {0.f, 0.f});   // story_1 (стартовый)
            createNode(graph, NodeType::Choice, {300.f, 0.f}); // choice_2
            createNode(graph, NodeType::Story, {600.f, 0.f});  // story_3
            createNode(graph, NodeType::Random, {600.f, 200.f}); // random_4
        }
        NodeId id(std::size_t i) const { return graph.order().at(i); }
    };
} // namespace

TEST_CASE("Editor: createNode задаёт тип, позицию и делает первый узел стартовым", "[editor]")
{
    Fixture f;
    CHECK(f.graph.size() == 4);
    CHECK(f.graph.startId() == f.id(0));
    CHECK(f.graph.at(f.id(1)).type() == NodeType::Choice);
    CHECK(f.graph.at(f.id(1)).position.x == 300.f);
    CHECK(f.graph.at(f.id(3)).type() == NodeType::Random);
}

TEST_CASE("Editor: connect создаёт связь по типу исходного узла", "[editor]")
{
    Fixture f;

    // Story: заменяет next.
    REQUIRE(connect(f.graph, f.id(0), f.id(1)));
    CHECK(static_cast<StoryNode &>(f.graph.at(f.id(0))).next() == f.id(1));
    REQUIRE(connect(f.graph, f.id(0), f.id(2)));
    CHECK(static_cast<StoryNode &>(f.graph.at(f.id(0))).next() == f.id(2));

    // Choice: каждый вызов добавляет вариант.
    REQUIRE(connect(f.graph, f.id(1), f.id(2)));
    REQUIRE(connect(f.graph, f.id(1), f.id(3)));
    CHECK(static_cast<ChoiceNode &>(f.graph.at(f.id(1))).choices().size() == 2);

    // Random: новые строки занимают следующие свободные значения кубика.
    auto &random = static_cast<RandomNode &>(f.graph.at(f.id(3)));
    random.setDiceSides(6);
    REQUIRE(connect(f.graph, f.id(3), f.id(0)));
    REQUIRE(connect(f.graph, f.id(3), f.id(2)));
    REQUIRE(random.entries().size() == 2);
    CHECK(random.entries()[0].minRoll == 1);
    CHECK(random.entries()[1].minRoll == 2);
}

TEST_CASE("Editor: connect отклоняет петли и несуществующие узлы", "[editor]")
{
    Fixture f;
    CHECK_FALSE(connect(f.graph, f.id(0), f.id(0)));
    CHECK_FALSE(connect(f.graph, f.id(0), "missing"));
    CHECK_FALSE(connect(f.graph, "missing", f.id(0)));
}

TEST_CASE("Editor: collectLinks и disconnect", "[editor]")
{
    Fixture f;
    connect(f.graph, f.id(0), f.id(1));
    connect(f.graph, f.id(1), f.id(2));
    connect(f.graph, f.id(1), f.id(3));
    auto &random = static_cast<RandomNode &>(f.graph.at(f.id(3)));
    random.setDiceSides(6);
    connect(f.graph, f.id(3), f.id(0));
    random.setFallbackTarget(f.id(1));

    auto links = collectLinks(f.graph);
    REQUIRE(links.size() == 5);
    CHECK(links[0].kind == Link::Kind::Story);
    CHECK(links[1].kind == Link::Kind::Choice);
    CHECK(links[3].kind == Link::Kind::Random);
    CHECK(links[3].label == "1");
    CHECK(links[4].kind == Link::Kind::RandomFallback);

    // Отключение обнуляет цель, но сохраняет вариант / строку таблицы.
    REQUIRE(disconnect(f.graph, links[1]));
    CHECK(static_cast<ChoiceNode &>(f.graph.at(f.id(1))).choices().size() == 2);
    CHECK(static_cast<ChoiceNode &>(f.graph.at(f.id(1))).choices()[0].target.empty());
    REQUIRE(disconnect(f.graph, links[4]));
    CHECK(random.fallbackTarget().empty());
    CHECK(collectLinks(f.graph).size() == 3);
}

TEST_CASE("Editor: removeNode переназначает старт", "[editor]")
{
    Fixture f;
    const NodeId start = f.id(0);
    REQUIRE(removeNode(f.graph, start));
    CHECK(f.graph.size() == 3);
    CHECK(f.graph.startId() == f.id(0)); // первый из оставшихся
    CHECK_FALSE(removeNode(f.graph, "missing"));
}

TEST_CASE("Editor: покрытие таблицы случайных встреч", "[editor]")
{
    RandomNode node("r");
    node.setDiceSides(6);
    CHECK(nextFreeRoll(node) == 1);

    node.addEntry(1, 2, "ambush", "a");
    node.addEntry(2, 3, "traveller", "b"); // 2 перекрывается
    CHECK(nextFreeRoll(node) == 4);

    const RollCoverage c = rollCoverage(node);
    CHECK(c.covered == 3);
    CHECK(c.overlapping == 1);

    node.addEntry(4, 6, "camp", "c");
    CHECK(nextFreeRoll(node) == 6); // таблица заполнена: берём последнее значение
    CHECK(rollCoverage(node).covered == 6);
}

TEST_CASE("Editor: бестиарий — id, привязка и удаление", "[editor]")
{
    Fixture f;
    f.graph.addEnemy({makeEnemyId(f.graph, "Goblin Archer"), "Goblin Archer", 13, 7, 7, 2});
    CHECK(f.graph.enemies[0].id == "goblin_archer");
    CHECK(makeEnemyId(f.graph, "Goblin Archer") == "goblin_archer_2");
    CHECK(makeEnemyId(f.graph, "Гоблин") == "enemy"); // не-ASCII имя

    REQUIRE(attachEnemy(f.graph, f.id(0), "goblin_archer"));
    CHECK_FALSE(attachEnemy(f.graph, f.id(0), "goblin_archer")); // без дублей
    CHECK_FALSE(attachEnemy(f.graph, f.id(0), "dragon"));        // нет в бестиарии
    REQUIRE(attachEnemy(f.graph, f.id(2), "goblin_archer"));
    CHECK(enemyUsageCount(f.graph, "goblin_archer") == 2);

    REQUIRE(removeEnemyEverywhere(f.graph, "goblin_archer"));
    CHECK(f.graph.enemies.empty());
    CHECK(f.graph.at(f.id(0)).enemyIds.empty());
    CHECK(f.graph.at(f.id(2)).enemyIds.empty());
}

TEST_CASE("Editor: autoLayout раскладывает узлы по слоям от стартового", "[editor]")
{
    Fixture f;
    connect(f.graph, f.id(0), f.id(1));
    connect(f.graph, f.id(1), f.id(2));
    // random_4 недостижим от старта.
    autoLayout(f.graph);

    const float x0 = f.graph.at(f.id(0)).position.x;
    const float x1 = f.graph.at(f.id(1)).position.x;
    const float x2 = f.graph.at(f.id(2)).position.x;
    const float x3 = f.graph.at(f.id(3)).position.x;
    CHECK(x0 == 0.f);
    CHECK(x1 > x0);
    CHECK(x2 > x1);
    CHECK(x3 > x2); // недостижимые — в отдельной колонке справа
}
