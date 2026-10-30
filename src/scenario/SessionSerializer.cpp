#include "scenario/SessionSerializer.hpp"
#include "scenario/CharacterSerializer.hpp"

#include <fstream>
#include <system_error>

namespace dnd::scenario
{

    nlohmann::json SessionSerializer::toJson(const SessionSaveData &session)
    {
        nlohmann::json root;
        root["formatVersion"] = kCurrentFormatVersion;
        root["scenarioFilePath"] = session.scenarioFilePath;
        root["currentNodeId"] = session.currentNodeId;
        root["combatRound"] = session.combatRound;
        root["activeTurnIndex"] = session.activeTurnIndex;
        root["eventLog"] = session.eventLog;
        root["roster"] = CharacterSerializer::rosterToJson(session.roster);
        return root;
    }

    bool SessionSerializer::fromJson(const nlohmann::json &j,
                                     SessionSaveData &outSession,
                                     std::string *outError)
    {
        try
        {
            if (!j.is_object())
            {
                if (outError)
                    *outError = "Session JSON must be an object.";
                return false;
            }

            if (!j.contains("formatVersion") || !j["formatVersion"].is_number_integer())
            {
                if (outError)
                    *outError = "Missing or invalid formatVersion in session file.";
                return false;
            }

            const int version = j["formatVersion"].get<int>();
            if (version > kCurrentFormatVersion || version < 1)
            {
                if (outError)
                    *outError = "Unsupported session formatVersion: " + std::to_string(version);
                return false;
            }

            outSession.scenarioFilePath = j.value("scenarioFilePath", "");
            outSession.currentNodeId = j.value("currentNodeId", "");
            outSession.combatRound = j.value("combatRound", 1);
            outSession.activeTurnIndex = j.value("activeTurnIndex", 0);

            outSession.eventLog.clear();
            if (j.contains("eventLog") && j["eventLog"].is_array())
            {
                for (const auto &logItem : j["eventLog"])
                {
                    if (logItem.is_string())
                        outSession.eventLog.push_back(logItem.get<std::string>());
                }
            }

            if (j.contains("roster") && j["roster"].is_object())
            {
                if (!CharacterSerializer::rosterFromJson(j["roster"], outSession.roster, outError))
                    return false;
            }

            return true;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Session parse error: ") + ex.what();
            return false;
        }
    }

    bool SessionSerializer::saveToFile(const SessionSaveData &session,
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
                        *outError = "Failed to create directory for " + filePath.string() + ": " + ec.message();
                    return false;
                }
            }

            std::ofstream out(filePath, std::ios::out | std::ios::trunc);
            if (!out.is_open())
            {
                if (outError)
                    *outError = "Cannot open file for writing: " + filePath.string();
                return false;
            }

            const nlohmann::json root = toJson(session);
            out << root.dump(indent) << '\n';
            return true;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Failed to save session: ") + ex.what();
            return false;
        }
    }

    bool SessionSerializer::loadFromFile(const std::filesystem::path &filePath,
                                         SessionSaveData &outSession,
                                         std::string *outError)
    {
        try
        {
            std::ifstream in(filePath, std::ios::in);
            if (!in.is_open())
            {
                if (outError)
                    *outError = "Session file not found: " + filePath.string();
                return false;
            }

            nlohmann::json root;
            in >> root;
            return fromJson(root, outSession, outError);
        }
        catch (const nlohmann::json::parse_error &pe)
        {
            if (outError)
                *outError = std::string("JSON parse error in '") + filePath.string() + "': " + pe.what();
            return false;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Failed to load session: ") + ex.what();
            return false;
        }
    }

} // namespace dnd::scenario
