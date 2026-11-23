#pragma once

#include <string>

namespace dnd::core
{

    /// Пользовательские настройки приложения.
    struct AppConfig
    {
        unsigned width = 1920;
        unsigned height = 1080;
        unsigned fps = 120;
        std::string title = "D&D Project";
        std::string assetRoot = "assets";

        /// Ручной масштаб интерфейса. 0 — автоматически подстраивать под размер окна.
        /// Значение можно изменить в меню Options, переопределив автоматический масштаб.
        float uiScale = 0.f;

        /// Каталоги данных. Пока зарезервировано: файлы сохраняются рядом с исполняемым файлом.
        std::string campaignDir;
        std::string saveDir;

        enum class ScreenMode
        {
            Windowed,
            Fullscreen,
            Borderless
        };
        ScreenMode screenMode = ScreenMode::Borderless;
    };

} // namespace dnd::core
