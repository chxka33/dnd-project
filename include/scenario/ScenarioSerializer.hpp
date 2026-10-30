#pragma once

#include "scenario/ScenarioGraph.hpp"

#include <filesystem>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace dnd::scenario
{

    /// Класс-утилита для сохранения и загрузки нелинейного сюжета сценария в формате .dndsc (JSON).
    class ScenarioSerializer
    {
    public:
        /// Текущая версия формата файла .dndsc.
        static constexpr int kCurrentFormatVersion = 2;

        /// Сериализует нелинейный сюжет в объект nlohmann::json.
        static nlohmann::json toJson(const ScenarioGraph &graph);

        /// Десериализует нелинейный сюжет из объекта nlohmann::json.
        /// При ошибке возвращает nullptr и, если указан outError, записывает в него пояснение.
        static std::unique_ptr<ScenarioGraph> fromJson(const nlohmann::json &j, std::string *outError = nullptr);

        /// Сохраняет сценарий в файл .dndsc с форматированием.
        /// Создает родительские директории, если они отсутствуют.
        /// Возвращает true при успехе, false при ошибке.
        static bool saveToFile(const ScenarioGraph &graph,
                               const std::filesystem::path &filePath,
                               std::string *outError = nullptr,
                               int indent = 4);

        /// Загружает сценарий из файла .dndsc.
        /// Возвращает nullptr и записывает сообщение в outError при ошибке.
        static std::unique_ptr<ScenarioGraph> loadFromFile(const std::filesystem::path &filePath,
                                                           std::string *outError = nullptr);
    };

} // namespace dnd::scenario
