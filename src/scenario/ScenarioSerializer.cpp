#include "scenario/ScenarioSerializer.hpp"

#include <fstream>
#include <system_error>

namespace dnd::scenario
{

    namespace
    {

        nlohmann::json serializePosition(const EditorPosition &pos)
        {
            return {
                {"x", pos.x},
                {"y", pos.y}};
        }

        EditorPosition deserializePosition(const nlohmann::json &j)
        {
            EditorPosition pos;
            if (j.is_object())
            {
                if (j.contains("x") && j["x"].is_number())
                    pos.x = j["x"].get<float>();
                if (j.contains("y") && j["y"].is_number())
                    pos.y = j["y"].get<float>();
            }
            return pos;
        }

        nlohmann::json serializeMedia(const NodeMedia &media)
        {
            return {
                {"map", media.mapPath},
                {"art", media.artPath}};
        }

        NodeMedia deserializeMedia(const nlohmann::json &j)
        {
            NodeMedia media;
            if (j.is_object())
            {
                if (j.contains("map") && j["map"].is_string())
                    media.mapPath = j["map"].get<std::string>();
                if (j.contains("art") && j["art"].is_string())
                    media.artPath = j["art"].get<std::string>();
            }
            return media;
        }

        nlohmann::json serializeCheck(const CheckDefinition &check)
        {
            return {
                {"type", toString(check.type)},
                {"ability", toString(check.ability)},
                {"dc", check.dc},
                {"skill", check.skill}};
        }

        CheckDefinition deserializeCheck(const nlohmann::json &j)
        {
            CheckDefinition check;
            if (j.is_object())
            {
                if (j.contains("type") && j["type"].is_string())
                {
                    if (auto t = checkTypeFromString(j["type"].get<std::string>()))
                        check.type = *t;
                }
                if (j.contains("ability") && j["ability"].is_string())
                {
                    if (auto ab = abilityFromString(j["ability"].get<std::string>()))
                        check.ability = *ab;
                }
                if (j.contains("dc") && j["dc"].is_number_integer())
                    check.dc = j["dc"].get<int>();
                if (j.contains("skill") && j["skill"].is_string())
                    check.skill = j["skill"].get<std::string>();
            }
            return check;
        }

        nlohmann::json serializeLoot(const Loot &loot)
        {
            nlohmann::json j;
            j["gold"] = loot.gold;
            nlohmann::json itemsArr = nlohmann::json::array();
            for (const auto &item : loot.items)
            {
                itemsArr.push_back({
                    {"name", item.name},
                    {"quantity", item.quantity}});
            }
            j["items"] = itemsArr;
            return j;
        }

        Loot deserializeLoot(const nlohmann::json &j)
        {
            Loot loot;
            if (j.is_object())
            {
                if (j.contains("gold") && j["gold"].is_number_integer())
                    loot.gold = j["gold"].get<int>();
                if (j.contains("items") && j["items"].is_array())
                {
                    for (const auto &itemJ : j["items"])
                    {
                        if (itemJ.is_object())
                        {
                            LootItem item;
                            item.name = itemJ.value("name", "");
                            item.quantity = itemJ.value("quantity", 1);
                            loot.items.push_back(std::move(item));
                        }
                    }
                }
            }
            return loot;
        }

        nlohmann::json serializeEnemyTemplate(const EnemyTemplate &e)
        {
            return {
                {"id", e.id},
                {"name", e.name},
                {"ac", e.armorClass},
                {"maxHp", e.maxHp},
                {"currentHp", e.currentHp},
                {"initiativeBonus", e.initiativeBonus}};
        }

        EnemyTemplate deserializeEnemyTemplate(const nlohmann::json &j)
        {
            EnemyTemplate e;
            if (j.is_object())
            {
                e.id = j.value("id", "");
                e.name = j.value("name", "");
                e.armorClass = j.value("ac", 10);
                e.maxHp = j.value("maxHp", 10);
                e.currentHp = j.value("currentHp", e.maxHp);
                e.initiativeBonus = j.value("initiativeBonus", 0);
            }
            return e;
        }

        nlohmann::json serializeNode(const Node &node)
        {
            nlohmann::json j;
            j["id"] = node.id();
            j["type"] = toString(node.type());
            j["title"] = node.title;
            j["summary"] = node.summary;
            j["text"] = node.text;
            j["dmNotes"] = node.dmNotes;
            j["position"] = serializePosition(node.position);
            j["media"] = serializeMedia(node.media);

            if (node.check)
            {
                j["check"] = serializeCheck(*node.check);
                j["skillCheck"] = serializeCheck(*node.check); // обратная совместимость
            }
            else
            {
                j["check"] = nullptr;
                j["skillCheck"] = nullptr;
            }

            if (node.loot)
                j["loot"] = serializeLoot(*node.loot);

            if (!node.enemyIds.empty())
                j["enemyIds"] = node.enemyIds;

            if (const auto *story = dynamic_cast<const StoryNode *>(&node))
            {
                j["next"] = story->next();
            }
            else if (const auto *choice = dynamic_cast<const ChoiceNode *>(&node))
            {
                nlohmann::json choicesArr = nlohmann::json::array();
                for (const auto &c : choice->choices())
                {
                    choicesArr.push_back({{"label", c.label},
                                          {"target", c.target}});
                }
                j["choices"] = choicesArr;
            }
            else if (const auto *random = dynamic_cast<const RandomNode *>(&node))
            {
                j["diceSides"] = random->diceSides();
                nlohmann::json entriesArr = nlohmann::json::array();
                for (const auto &entry : random->entries())
                {
                    entriesArr.push_back({
                        {"minRoll", entry.minRoll},
                        {"maxRoll", entry.maxRoll},
                        {"label", entry.label},
                        {"target", entry.target}});
                }
                j["entries"] = entriesArr;

                // Для обратной совместимости с версией 1
                j["chance"] = random->chancePercent();
                j["encounter"] = random->encounterTarget();
                j["fallback"] = random->fallbackTarget();
            }

            return j;
        }

    } // namespace

    nlohmann::json ScenarioSerializer::toJson(const ScenarioGraph &graph)
    {
        nlohmann::json root;
        root["formatVersion"] = kCurrentFormatVersion;
        root["title"] = graph.title;
        root["description"] = graph.description;
        root["startNode"] = graph.startId();

        // Бестиарий кампании
        nlohmann::json enemiesArr = nlohmann::json::array();
        for (const auto &enemy : graph.enemies)
        {
            enemiesArr.push_back(serializeEnemyTemplate(enemy));
        }
        root["enemies"] = enemiesArr;

        // Узлы сюжета
        nlohmann::json nodesArr = nlohmann::json::array();
        for (const NodeId &id : graph.order())
        {
            if (const Node *node = graph.find(id))
                nodesArr.push_back(serializeNode(*node));
        }
        root["nodes"] = nodesArr;

        return root;
    }

    std::unique_ptr<ScenarioGraph> ScenarioSerializer::fromJson(const nlohmann::json &j, std::string *outError)
    {
        try
        {
            if (!j.is_object())
            {
                if (outError)
                    *outError = "Root JSON element must be an object.";
                return nullptr;
            }

            if (!j.contains("formatVersion") || !j["formatVersion"].is_number_integer())
            {
                if (outError)
                    *outError = "Missing or invalid 'formatVersion' field.";
                return nullptr;
            }

            const int version = j["formatVersion"].get<int>();
            if (version > kCurrentFormatVersion || version < 1)
            {
                if (outError)
                    *outError = "Unsupported formatVersion: " + std::to_string(version) +
                                " (highest supported is " + std::to_string(kCurrentFormatVersion) + ").";
                return nullptr;
            }

            auto graph = std::make_unique<ScenarioGraph>();

            if (j.contains("title") && j["title"].is_string())
                graph->title = j["title"].get<std::string>();
            if (j.contains("description") && j["description"].is_string())
                graph->description = j["description"].get<std::string>();

            // Загрузка бестиария сценария
            if (j.contains("enemies") && j["enemies"].is_array())
            {
                for (const auto &enemyJ : j["enemies"])
                {
                    if (enemyJ.is_object())
                    {
                        auto enemy = deserializeEnemyTemplate(enemyJ);
                        if (!enemy.id.empty())
                            graph->addEnemy(std::move(enemy));
                    }
                }
            }

            if (!j.contains("nodes") || !j["nodes"].is_array())
            {
                if (outError)
                    *outError = "Missing or invalid 'nodes' array.";
                return nullptr;
            }

            for (const auto &nodeJ : j["nodes"])
            {
                if (!nodeJ.is_object())
                {
                    if (outError)
                        *outError = "Each item in 'nodes' must be a JSON object.";
                    return nullptr;
                }

                if (!nodeJ.contains("id") || !nodeJ["id"].is_string() || nodeJ["id"].get<std::string>().empty())
                {
                    if (outError)
                        *outError = "Node must have a non-empty string 'id'.";
                    return nullptr;
                }

                const NodeId id = nodeJ["id"].get<std::string>();
                const std::string title = nodeJ.value("title", "");

                if (!nodeJ.contains("type") || !nodeJ["type"].is_string())
                {
                    if (outError)
                        *outError = "Node '" + id + "' is missing string 'type'.";
                    return nullptr;
                }

                auto optType = nodeTypeFromString(nodeJ["type"].get<std::string>());
                if (!optType)
                {
                    if (outError)
                        *outError = "Node '" + id + "' has unknown type: " + nodeJ["type"].get<std::string>();
                    return nullptr;
                }

                std::unique_ptr<Node> node;
                switch (*optType)
                {
                case NodeType::Story:
                {
                    auto story = std::make_unique<StoryNode>(id, title);
                    if (nodeJ.contains("next") && nodeJ["next"].is_string())
                        story->setNext(nodeJ["next"].get<std::string>());
                    node = std::move(story);
                    break;
                }
                case NodeType::Choice:
                {
                    auto choice = std::make_unique<ChoiceNode>(id, title);
                    if (nodeJ.contains("choices") && nodeJ["choices"].is_array())
                    {
                        for (const auto &cJ : nodeJ["choices"])
                        {
                            if (cJ.is_object())
                            {
                                std::string label = cJ.value("label", "");
                                std::string target = cJ.value("target", "");
                                choice->addChoice(std::move(label), std::move(target));
                            }
                        }
                    }
                    node = std::move(choice);
                    break;
                }
                case NodeType::Random:
                {
                    auto random = std::make_unique<RandomNode>(id, title);
                    if (nodeJ.contains("diceSides") && nodeJ["diceSides"].is_number_integer())
                        random->setDiceSides(nodeJ["diceSides"].get<int>());

                    if (nodeJ.contains("entries") && nodeJ["entries"].is_array())
                    {
                        for (const auto &ej : nodeJ["entries"])
                        {
                            if (ej.is_object())
                            {
                                int minR = ej.value("minRoll", 1);
                                int maxR = ej.value("maxRoll", 1);
                                std::string lbl = ej.value("label", "");
                                std::string tgt = ej.value("target", "");
                                random->addEntry(minR, maxR, std::move(lbl), std::move(tgt));
                            }
                        }
                    }

                    // Обратная совместимость с форматом версий 1
                    if (nodeJ.contains("chance") && nodeJ["chance"].is_number_integer())
                        random->setChancePercent(nodeJ["chance"].get<int>());
                    if (nodeJ.contains("encounter") && nodeJ["encounter"].is_string())
                        random->setEncounterTarget(nodeJ["encounter"].get<std::string>());
                    if (nodeJ.contains("fallback") && nodeJ["fallback"].is_string())
                        random->setFallbackTarget(nodeJ["fallback"].get<std::string>());

                    node = std::move(random);
                    break;
                }
                }

                // Общие поля узла
                node->summary = nodeJ.value("summary", "");
                node->dmNotes = nodeJ.value("dmNotes", "");

                if (nodeJ.contains("text") && nodeJ["text"].is_string())
                    node->text = nodeJ["text"].get<std::string>();
                if (nodeJ.contains("position"))
                    node->position = deserializePosition(nodeJ["position"]);
                if (nodeJ.contains("media"))
                    node->media = deserializeMedia(nodeJ["media"]);

                if (nodeJ.contains("check") && !nodeJ["check"].is_null())
                    node->check = deserializeCheck(nodeJ["check"]);
                else if (nodeJ.contains("skillCheck") && !nodeJ["skillCheck"].is_null())
                    node->check = deserializeCheck(nodeJ["skillCheck"]);

                if (nodeJ.contains("loot") && nodeJ["loot"].is_object())
                    node->loot = deserializeLoot(nodeJ["loot"]);

                if (nodeJ.contains("enemyIds") && nodeJ["enemyIds"].is_array())
                {
                    for (const auto &eid : nodeJ["enemyIds"])
                    {
                        if (eid.is_string())
                            node->enemyIds.push_back(eid.get<std::string>());
                    }
                }

                graph->add(std::move(node));
            }

            if (j.contains("startNode") && j["startNode"].is_string())
            {
                const std::string startId = j["startNode"].get<std::string>();
                if (!startId.empty())
                {
                    if (graph->contains(startId))
                    {
                        graph->setStart(startId);
                    }
                    else
                    {
                        if (outError)
                            *outError = "Specified startNode '" + startId + "' not found in scenario.";
                        return nullptr;
                    }
                }
            }

            return graph;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Deserialization exception: ") + ex.what();
            return nullptr;
        }
    }

    bool ScenarioSerializer::saveToFile(const ScenarioGraph &graph,
                                        const std::filesystem::path &filePath,
                                        std::string *outError,
                                        int indent)
    {
        try
        {
            if (filePath.has_parent_path())
            {
                std::error_code ec;
                std::filesystem::create_directories(filePath.parent_path(), ec);
                if (ec)
                {
                    if (outError)
                        *outError = "Failed to create directories for '" + filePath.string() + "': " + ec.message();
                    return false;
                }
            }

            std::ofstream out(filePath, std::ios::out | std::ios::trunc);
            if (!out.is_open())
            {
                if (outError)
                    *outError = "Failed to open file for writing: " + filePath.string();
                return false;
            }

            const nlohmann::json j = toJson(graph);
            out << j.dump(indent) << '\n';
            if (!out)
            {
                if (outError)
                    *outError = "Stream write error while writing to " + filePath.string();
                return false;
            }
            return true;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("File save exception: ") + ex.what();
            return false;
        }
    }

    std::unique_ptr<ScenarioGraph> ScenarioSerializer::loadFromFile(const std::filesystem::path &filePath,
                                                                   std::string *outError)
    {
        try
        {
            std::ifstream in(filePath, std::ios::in);
            if (!in.is_open())
            {
                if (outError)
                    *outError = "File not found or cannot be opened: " + filePath.string();
                return nullptr;
            }

            nlohmann::json j;
            in >> j;
            if (!in && !in.eof())
            {
                if (outError)
                    *outError = "Stream error while reading " + filePath.string();
                return nullptr;
            }

            return fromJson(j, outError);
        }
        catch (const nlohmann::json::parse_error &pe)
        {
            if (outError)
                *outError = std::string("JSON parse error in '") + filePath.string() + "': " + pe.what();
            return nullptr;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("File load exception: ") + ex.what();
            return nullptr;
        }
    }

} // namespace dnd::scenario
