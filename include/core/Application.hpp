#pragma once

#include "core/AppConfig.hpp"
#include "core/ResourceManager.hpp"
#include "core/StateManager.hpp"
#include "entities/Roster.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <string>

namespace dnd::core
{
    struct AppContext
    {
        sf::RenderWindow &window;
        ResourceManager &resources;
        StateManager &states;
        entities::Roster &roster; ///< персонажи текущей сессии (общие для всех экранов)
        AppConfig &config;        ///< настройки приложения (Options меняет их на лету)
        bool windowRecreateRequested = false; ///< пересоздать окно между кадрами
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
        void recreateWindow();

        AppConfig m_config;
        sf::RenderWindow m_window;
        ResourceManager m_resources;
        StateManager m_states;
        entities::Roster m_roster;
        AppContext m_context;
    };

} // namespace dnd::core