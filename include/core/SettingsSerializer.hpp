#pragma once

#include "core/AppConfig.hpp"

#include <filesystem>
#include <string>

namespace dnd::core
{

    /// Сохранение и загрузка пользовательских настроек приложения (settings.json).
    class SettingsSerializer
    {
    public:
        static constexpr int kCurrentFormatVersion = 1;

        /// Файл настроек рядом с исполняемым файлом.
        static std::filesystem::path defaultPath();

        /// Загружает настройки в config. Отсутствие файла не считается ошибкой:
        /// в этом случае config остаётся со значениями по умолчанию.
        static bool load(AppConfig &config,
                         const std::filesystem::path &filePath,
                         std::string *outError = nullptr);

        /// Сохраняет настройки в файл, создавая родительские каталоги при необходимости.
        static bool save(const AppConfig &config,
                         const std::filesystem::path &filePath,
                         std::string *outError = nullptr);
    };

} // namespace dnd::core
