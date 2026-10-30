#pragma once

#include "entities/Roster.hpp"
#include "scenario/Node.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace dnd::scenario
{

    /// Данные активного сохранения игровой сессии (.dndsav).
    struct SessionSaveData
    {
        std::string scenarioFilePath;       ///< путь к файлу сценария .dndsc
        NodeId currentNodeId;               ///< идентификатор текущего активного узла
        entities::Roster roster;            ///< актуальное состояние группы (HP, инвентарь, слоты, условия)
        std::vector<std::string> eventLog;  ///< история событий сессии
        int combatRound = 1;                ///< текущий раунд боя
        int activeTurnIndex = 0;            ///< чей сейчас ход в трекере инициативы
    };

    /// Класс-утилита для сохранения и загрузки состояния игровой сессии (.dndsav).
    class SessionSerializer
    {
    public:
        static constexpr int kCurrentFormatVersion = 1;

        /// Сериализует сессию в объект JSON.
        static nlohmann::json toJson(const SessionSaveData &session);

        /// Десериализует сессию из объекта JSON.
        static bool fromJson(const nlohmann::json &j,
                             SessionSaveData &outSession,
                             std::string *outError = nullptr);

        /// Сохраняет состояние сессии в файл .dndsav.
        static bool saveToFile(const SessionSaveData &session,
                               const std::filesystem::path &filePath,
                               std::string *outError = nullptr,
                               int indent = 4);

        /// Загружает состояние сессии из файла .dndsav.
        static bool loadFromFile(const std::filesystem::path &filePath,
                                 SessionSaveData &outSession,
                                 std::string *outError = nullptr);
    };

} // namespace dnd::scenario
