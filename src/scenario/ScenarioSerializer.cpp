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

        nlohmann::json serializeSkillCheck(const SkillCheck &sc)
        {
            return {
                {"ability", toString(sc.ability)},
                {"dc", sc.dc},
                {"skill", sc.skill}};
        }

        SkillCheck deserializeSkillCheck(const nlohmann::json &j)
        {
            SkillCheck sc;
            if (j.is_object())
            {
                if (j.contains("ability") && j["ability"].is_string())
                {
                    if (auto ab = abilityFromString(j["ability"].get<std::string>()))
                        sc.ability = *ab;
                }
                if (j.contains("dc") && j["dc"].is_number_integer())
                    sc.dc = j["dc"].get<int>();
                if (j.contains("skill") && j["skill"].is_string())
                    sc.skill = j["skill"].get<std::string>();
            }
            return sc;
        }

        nlohmann::json serializeNode(const Node &node)
        {
            nlohmann::json j;
            j["id"] = node.id();
            j["type"] = toString(node.type());
            j["title"] = node.title;
            j["text"] = node.text;
            j["position"] = serializePosition(node.position);
            j["media"] = serializeMedia(node.media);

            if (node.skillCheck)
                j["skillCheck"] = serializeSkillCheck(*node.skillCheck);
            else
                j["skillCheck"] = nullptr;

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
                        *outError = "Node '" + id + "' is missing 'type' string.";
                    return nullptr;
                }

                const std::string typeStr = nodeJ["type"].get<std::string>();
                auto nodeType = nodeTypeFromString(typeStr);
                if (!nodeType)
                {
                    if (outError)
                        *outError = "Node '" + id + "' has unknown type '" + typeStr + "'.";
                    return nullptr;
                }

                std::unique_ptr<Node> node;
                switch (*nodeType)
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

                // Общие поля
                if (nodeJ.contains("text") && nodeJ["text"].is_string())
                    node->text = nodeJ["text"].get<std::string>();
                if (nodeJ.contains("position"))
                    node->position = deserializePosition(nodeJ["position"]);
                if (nodeJ.contains("media"))
                    node->media = deserializeMedia(nodeJ["media"]);
                if (nodeJ.contains("skillCheck") && !nodeJ["skillCheck"].is_null())
                    node->skillCheck = deserializeSkillCheck(nodeJ["skillCheck"]);

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
                        *outError = "Failed to create directory '" + filePath.parent_path().string() +
                                    "': " + ec.message();
                    return false;
                }
            }

            std::ofstream ofs(filePath);
            if (!ofs.is_open())
            {
                if (outError)
                    *outError = "Failed to open file for writing: " + filePath.string();
                return false;
            }

            nlohmann::json j = toJson(graph);
            ofs << j.dump(indent) << '\n';
            if (!ofs.good())
            {
                if (outError)
                    *outError = "Failed to write data to file: " + filePath.string();
                return false;
            }

            return true;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Exception while saving file: ") + ex.what();
            return false;
        }
    }

    std::unique_ptr<ScenarioGraph> ScenarioSerializer::loadFromFile(const std::filesystem::path &filePath,
                                                                    std::string *outError)
    {
        try
        {
            std::ifstream ifs(filePath);
            if (!ifs.is_open())
            {
                if (outError)
                    *outError = "Failed to open file for reading: " + filePath.string();
                return nullptr;
            }

            nlohmann::json j = nlohmann::json::parse(ifs, nullptr, true, true);
            return fromJson(j, outError);
        }
        catch (const nlohmann::json::parse_error &ex)
        {
            if (outError)
                *outError = std::string("JSON parse error: ") + ex.what();
            return nullptr;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Exception while loading file: ") + ex.what();
            return nullptr;
        }
    }

} // namespace dnd::scenario
