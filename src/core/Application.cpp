#include "core/Application.hpp"
#include "core/CharactersState.hpp"
#include "core/MainMenuState.hpp"

#include <SFML/System/Clock.hpp>

#include <imgui-SFML.h>
#include <imgui.h>

#include <memory>
#include <stdexcept> // для std::runtime_error

namespace dnd::core
{

    Application::Application(const AppConfig &config)
        : m_config(config), m_window([&]
                                     {
        if (m_config.screenMode == AppConfig::ScreenMode::Fullscreen)
        {
            // Эксклюзивный fullscreen, меняет разрешение монитора
            return sf::RenderWindow(sf::VideoMode::getDesktopMode(),
                                    config.title,
                                    sf::Style::Fullscreen);
        }

        if (m_config.screenMode == AppConfig::ScreenMode::Borderless)
        {
            // Окно без рамки во весь экран — лучший вариант на практике
            return sf::RenderWindow(sf::VideoMode::getDesktopMode(),
                                    config.title,
                                    sf::Style::None);
        }

        return sf::RenderWindow(sf::VideoMode(config.width, config.height),
                                config.title); }()),
          m_context{m_window, m_resources, m_states, m_roster}
    {
        m_window.setFramerateLimit(m_config.fps);
        m_resources.setAssetRoot(m_config.assetRoot);

        // Отключаем imgui.ini — позже его можно вернуть для запоминания раскладки окон.
        if (!ImGui::SFML::Init(m_window))
        {
            throw std::runtime_error("Failed to initialise ImGui-SFML");
        }
        ImGui::GetIO().IniFilename = nullptr;

        m_states.setContext(&m_context);
        registerStates();
        m_states.requestSwitch(StateId::MainMenu);
        m_states.applyPending();
    }

    Application::~Application()
    {
        ImGui::SFML::Shutdown();
    }

    void Application::registerStates()
    {
        m_states.registerState(StateId::MainMenu,
                               []
                               { return std::make_unique<MainMenuState>(); });
        // TODO(Назаров): заменить на EditorState
        m_states.registerState(StateId::Editor,
                               []
                               { return std::make_unique<PlaceholderState>("Scenario Editor"); });
        // TODO(Ярошук): заменить на SimulatorState
        m_states.registerState(StateId::Simulator,
                               []
                               { return std::make_unique<PlaceholderState>("Game Simulator"); });
        m_states.registerState(StateId::Characters,
                               []
                               { return std::make_unique<CharactersState>(); });
        m_states.registerState(StateId::Options,
                               []
                               { return std::make_unique<PlaceholderState>("Options"); });
    }

    void Application::processEvents()
    {
        sf::Event event;
        while (m_window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(m_window, event);

            if (event.type == sf::Event::Closed)
            {
                m_window.close();
                return;
            }

            // Не пускаем в игру ввод, который «съел» ImGui.
            const ImGuiIO &io = ImGui::GetIO();
            const bool mouse = event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased || event.type == sf::Event::MouseMoved || event.type == sf::Event::MouseWheelScrolled;
            const bool keyboard = event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased || event.type == sf::Event::TextEntered;
            if ((mouse && io.WantCaptureMouse) || (keyboard && io.WantCaptureKeyboard))
            {
                continue;
            }
            m_states.handleEvent(event);
        }
    }

    int Application::run()
    {
        // Логика обновляется с фиксированным шагом, отрисовка — на каждый кадр.
        const sf::Time step = sf::seconds(1.f / 60.f);
        const sf::Time maxFrame = sf::seconds(0.25f); // защита от «спирали смерти»

        sf::Clock clock;
        sf::Time accumulator = sf::Time::Zero;

        while (m_window.isOpen() && !m_states.empty())
        {
            sf::Time frame = clock.restart();
            if (frame > maxFrame)
                frame = maxFrame;
            accumulator += frame;

            processEvents();

            while (accumulator >= step)
            {
                m_states.update(step);
                accumulator -= step;
            }

            ImGui::SFML::Update(m_window, frame);
            m_states.drawUi();

            m_window.clear(sf::Color(24, 20, 18));
            m_states.render(m_window);
            ImGui::SFML::Render(m_window);
            m_window.display();

            m_states.applyPending(); // смена экранов — только между кадрами
        }
        return 0;
    }

} // namespace dnd::core