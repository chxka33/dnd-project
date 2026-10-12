#pragma once

#include "core/ResourceManager.hpp"
#include "core/StateManager.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <string>

namespace dnd::core
{
    struct AppContext
    {
        sf::RenderWindow &window;
        ResourceManager &resources;
        StateManager &states;
    };

    struct AppConfig
    {
        unsigned width = 1920;
        unsigned height = 1080;
        unsigned fps = 120;
        std::string title = "D&D Project";
        std::string assetRoot = "assets";

        enum class ScreenMode
        {
            Windowed,
            Fullscreen,
            Borderless
        };
        ScreenMode screenMode = ScreenMode::Borderless;
    };

    /// Главный объект приложения: окно + Game Loop.
    class Application
    {
    public:
        explicit Application(const AppConfig &config = AppConfig{});
        ~Application();

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        /// Запускает игровой цикл. Возвращает код выхода.
        int run();

    private:
        void processEvents();
        void registerStates();

        AppConfig m_config;
        sf::RenderWindow m_window;
        ResourceManager m_resources;
        StateManager m_states;
        AppContext m_context;
    };

} // namespace dnd::core
