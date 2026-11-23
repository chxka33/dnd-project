#pragma once

#include "core/State.hpp"

#include <string>

namespace dnd::core
{

    /// Экран «Options» — настройки приложения (модуль 1.4).
    ///
    /// Реализовано: режим окна, разрешение в оконном режиме,
    /// ручной/автоматический масштаб интерфейса и сохранение в settings.json.
    /// Заглушка: каталоги данных.
    class OptionsState final : public State
    {
    public:
        void onEnter() override;
        void onExit() override;

        void handleEvent(const sf::Event &event) override;
        void update(sf::Time) override {}
        void drawUi() override;
        void render(sf::RenderWindow &) override {}

    private:
        void applyScale();
        void drawDisplaySection();
        void drawInterfaceSection();
        void drawDirectoriesSection();

        void saveSettings();
        void restoreDefaults();
        void closeOptions();
        void setStatus(std::string text, bool error);

        float m_previousScale = 1.f;

        int m_screenMode = 2; ///< индекс AppConfig::ScreenMode
        int m_width = 1920;
        int m_height = 1080;

        bool m_autoUiScale = true;
        float m_uiScale = 1.5f;

        std::string m_campaignDir;
        std::string m_saveDir;

        std::string m_status;
        bool m_statusIsError = false;
    };

} // namespace dnd::core
