#pragma once

#include "entities/Character.hpp"
#include "entities/Roster.hpp"

#include <filesystem>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace dnd::scenario
{

    /// Класс-утилита для сохранения и загрузки персонажей и групп (.dndch) в формате JSON.
    class CharacterSerializer
    {
    public:
        static constexpr int kCurrentFormatVersion = 1;

        /// Сериализует одного персонажа в объект JSON.
        static nlohmann::json toJson(const entities::Character &character);

        /// Десериализует одного персонажа из объекта JSON.
        static std::unique_ptr<entities::Character> fromJson(const nlohmann::json &j,
                                                             std::string *outError = nullptr);

        /// Сохраняет одного персонажа в файл .dndch.
        static bool saveToFile(const entities::Character &character,
                               const std::filesystem::path &filePath,
                               std::string *outError = nullptr,
                               int indent = 4);

        /// Загружает одного персонажа из файла .dndch.
        static std::unique_ptr<entities::Character> loadFromFile(const std::filesystem::path &filePath,
                                                                 std::string *outError = nullptr);

        /// Сериализует группу персонажей (Roster) в объект JSON.
        static nlohmann::json rosterToJson(const entities::Roster &roster);

        /// Десериализует группу персонажей из JSON и заполняет outRoster.
        static bool rosterFromJson(const nlohmann::json &j,
                                  entities::Roster &outRoster,
                                  std::string *outError = nullptr);

        /// Сохраняет группу персонажей в файл .dndch.
        static bool saveRosterToFile(const entities::Roster &roster,
                                    const std::filesystem::path &filePath,
                                    std::string *outError = nullptr,
                                    int indent = 4);

        /// Загружает группу персонажей из файла .dndch.
        static bool loadRosterFromFile(const std::filesystem::path &filePath,
                                      entities::Roster &outRoster,
                                      std::string *outError = nullptr);
    };

} // namespace dnd::scenario
