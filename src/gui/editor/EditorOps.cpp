#include "gui/editor/EditorOps.hpp"

#include <algorithm>
#include <cctype>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace dnd::gui::editor
{
    using namespace dnd::scenario;

    std::vector<Link> collectLinks(const ScenarioGraph &graph)
    {
        std::vector<Link> links;
        for (const NodeId &id : graph.order())
        {
            const Node &node = graph.at(id);
            switch (node.type())
            {
            case NodeType::Story:
            {
                const auto &story = static_cast<const StoryNode &>(node);
                if (!story.next().empty())
                    links.push_back({id, story.next(), Link::Kind::Story, 0, {}});
                break;
            }
            case NodeType::Choice:
            {
                const auto &choice = static_cast<const ChoiceNode &>(node);
                for (std::size_t i = 0; i < choice.choices().size(); ++i)
                {
                    const Choice &c = choice.choices()[i];
                    if (!c.target.empty())
                        links.push_back({id, c.target, Link::Kind::Choice, i, c.label});
                }
                break;
            }
            case NodeType::Random:
            {
                const auto &random = static_cast<const RandomNode &>(node);
                for (std::size_t i = 0; i < random.entries().size(); ++i)
                {
                    const RollTableEntry &e = random.entries()[i];
                    if (e.target.empty())
                        continue;
                    std::string label = e.minRoll == e.maxRoll
                                            ? std::to_string(e.minRoll)
                                            : std::to_string(e.minRoll) + "-" + std::to_string(e.maxRoll);
                    links.push_back({id, e.target, Link::Kind::Random, i, std::move(label)});
                }
                if (!random.fallbackTarget().empty())
                    links.push_back({id, random.fallbackTarget(), Link::Kind::RandomFallback, 0, "else"});
                break;
            }
            }
        }
        return links;
    }

    bool removeNode(ScenarioGraph &graph, const NodeId &id)
    {
        const bool wasStart = graph.startId() == id;
        if (!graph.remove(id))
            return false;
        if (wasStart && !graph.empty())
            graph.setStart(graph.order().front());
        return true;
    }

    int nextFreeRoll(const RandomNode &node)
    {
        const int sides = node.diceSides();
        std::vector<bool> used(static_cast<std::size_t>(sides) + 1, false);
        for (const RollTableEntry &e : node.entries())
            for (int r = std::max(1, e.minRoll); r <= std::min(sides, e.maxRoll); ++r)
                used[static_cast<std::size_t>(r)] = true;
        for (int r = 1; r <= sides; ++r)
            if (!used[static_cast<std::size_t>(r)])
                return r;
        return sides; // таблица заполнена целиком — пересечение лучше, чем выход за диапазон
    }

    RollCoverage rollCoverage(const RandomNode &node)
    {
        const int sides = node.diceSides();
        std::vector<int> hits(static_cast<std::size_t>(sides) + 1, 0);
        for (const RollTableEntry &e : node.entries())
            for (int r = std::max(1, e.minRoll); r <= std::min(sides, e.maxRoll); ++r)
                ++hits[static_cast<std::size_t>(r)];
        RollCoverage result;
        for (int r = 1; r <= sides; ++r)
        {
            if (hits[static_cast<std::size_t>(r)] >= 1)
                ++result.covered;
            if (hits[static_cast<std::size_t>(r)] > 1)
                ++result.overlapping;
        }
        return result;
    }

    bool connect(ScenarioGraph &graph, const NodeId &from, const NodeId &to)
    {
        if (from == to || !graph.contains(from) || !graph.contains(to))
            return false;

        Node &node = graph.at(from);
        switch (node.type())
        {
        case NodeType::Story:
            static_cast<StoryNode &>(node).setNext(to);
            return true;
        case NodeType::Choice:
            static_cast<ChoiceNode &>(node).addChoice("New choice", to);
            return true;
        case NodeType::Random:
        {
            auto &random = static_cast<RandomNode &>(node);
            const int roll = nextFreeRoll(random);
            random.addEntry(roll, roll, "Outcome", to);
            return true;
        }
        }
        return false;
    }

    bool disconnect(ScenarioGraph &graph, const Link &link)
    {
        Node *node = graph.find(link.from);
        if (!node)
            return false;

        switch (link.kind)
        {
        case Link::Kind::Story:
            if (node->type() != NodeType::Story)
                return false;
            static_cast<StoryNode *>(node)->setNext({});
            return true;
        case Link::Kind::Choice:
        {
            if (node->type() != NodeType::Choice)
                return false;
            auto &choices = static_cast<ChoiceNode *>(node)->choices();
            if (link.index >= choices.size())
                return false;
            choices[link.index].target.clear();
            return true;
        }
        case Link::Kind::Random:
        {
            if (node->type() != NodeType::Random)
                return false;
            auto &entries = static_cast<RandomNode *>(node)->entries();
            if (link.index >= entries.size())
                return false;
            entries[link.index].target.clear();
            return true;
        }
        case Link::Kind::RandomFallback:
            if (node->type() != NodeType::Random)
                return false;
            static_cast<RandomNode *>(node)->setFallbackTarget({});
            return true;
        }
        return false;
    }

    Node &createNode(ScenarioGraph &graph, NodeType type, EditorPosition pos)
    {
        Node *created = nullptr;
        switch (type)
        {
        case NodeType::Story:
            created = &graph.emplace<StoryNode>(graph.generateId("story"), "New scene");
            break;
        case NodeType::Choice:
            created = &graph.emplace<ChoiceNode>(graph.generateId("choice"), "New fork");
            break;
        case NodeType::Random:
            created = &graph.emplace<RandomNode>(graph.generateId("random"), "Random encounter");
            break;
        }
        created->position = pos;
        return *created;
    }

    std::string makeEnemyId(const ScenarioGraph &graph, const std::string &name)
    {
        std::string base;
        for (unsigned char ch : name)
        {
            if (std::isalnum(ch) && ch < 128)
                base += static_cast<char>(std::tolower(ch));
            else if (!base.empty() && base.back() != '_')
                base += '_';
        }
        while (!base.empty() && base.back() == '_')
            base.pop_back();
        if (base.empty())
            base = "enemy";

        std::string id = base;
        for (int n = 2; graph.findEnemy(id) != nullptr; ++n)
            id = base + "_" + std::to_string(n);
        return id;
    }

    bool removeEnemyEverywhere(ScenarioGraph &graph, const std::string &enemyId)
    {
        if (!graph.removeEnemy(enemyId))
            return false;
        for (const NodeId &id : graph.order())
        {
            auto &ids = graph.at(id).enemyIds;
            ids.erase(std::remove(ids.begin(), ids.end(), enemyId), ids.end());
        }
        return true;
    }

    bool attachEnemy(ScenarioGraph &graph, const NodeId &nodeId, const std::string &enemyId)
    {
        Node *node = graph.find(nodeId);
        if (!node || !graph.findEnemy(enemyId))
            return false;
        auto &ids = node->enemyIds;
        if (std::find(ids.begin(), ids.end(), enemyId) != ids.end())
            return false;
        ids.push_back(enemyId);
        return true;
    }

    bool detachEnemy(ScenarioGraph &graph, const NodeId &nodeId, const std::string &enemyId)
    {
        Node *node = graph.find(nodeId);
        if (!node)
            return false;
        auto &ids = node->enemyIds;
        const auto it = std::find(ids.begin(), ids.end(), enemyId);
        if (it == ids.end())
            return false;
        ids.erase(it);
        return true;
    }

    std::size_t enemyUsageCount(const ScenarioGraph &graph, const std::string &enemyId)
    {
        std::size_t count = 0;
        for (const NodeId &id : graph.order())
        {
            const auto &ids = graph.at(id).enemyIds;
            if (std::find(ids.begin(), ids.end(), enemyId) != ids.end())
                ++count;
        }
        return count;
    }

    void autoLayout(ScenarioGraph &graph)
    {
        constexpr float kGapX = 90.f;
        constexpr float kGapY = 50.f;

        std::unordered_map<NodeId, int> depth;
        std::unordered_map<int, int> rowInColumn;
        int maxDepth = -1;

        auto place = [&](const NodeId &id, int column)
        {
            const int row = rowInColumn[column]++;
            graph.at(id).position = {column * (kNodeWidth + kGapX), row * (kNodeHeight + kGapY)};
            maxDepth = std::max(maxDepth, column);
        };

        if (graph.contains(graph.startId()))
        {
            std::queue<NodeId> queue;
            depth[graph.startId()] = 0;
            queue.push(graph.startId());
            while (!queue.empty())
            {
                const NodeId id = queue.front();
                queue.pop();
                place(id, depth[id]);
                for (const NodeId &next : graph.at(id).targets())
                {
                    if (graph.contains(next) && depth.count(next) == 0)
                    {
                        depth[next] = depth[id] + 1;
                        queue.push(next);
                    }
                }
            }
        }

        const int orphanColumn = maxDepth + 1;
        for (const NodeId &id : graph.order())
            if (depth.count(id) == 0)
                place(id, orphanColumn);
    }

} // namespace dnd::gui::editor
