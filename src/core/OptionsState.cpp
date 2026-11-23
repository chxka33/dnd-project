#include "core/OptionsState.hpp"

#include "core/Application.hpp"
#include "core/SettingsSerializer.hpp"
#include "core/UiScale.hpp"
#include "utils/ExecutablePath.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace dnd::core
{

    namespace
    {

        // Экран настроек делаем заметно крупнее остальных экранов:
        // так поля и подписи легче читать и на них проще попадать мышью.
        constexpr float kOptionsScaleBoost = 1.35f;

        const char *kScreenModeNames[] = {"Windowed", "Fullscreen", "Borderless"};

        struct ResolutionPreset
        {
            const char *label;
            int width;
            int height;
        };

        const ResolutionPreset kResolutionPresets[] = {
            {"1280 x 720 (HD)", 1280, 720},
            {"1366 x 768", 1366, 768},
            {"1600 x 900", 1600, 900},
            {"1920 x 1080 (Full HD)", 1920, 1080},
            {"2560 x 1440 (QHD)", 2560, 1440},
            {"3840 x 2160 (4K UHD)", 3840, 2160},
        };

        constexpr int kResolutionPresetCount =
            static_cast<int>(sizeof(kResolutionPresets) / sizeof(kResolutionPresets[0]));

        int matchingResolutionPreset(int width, int height)
        {
            for (int i = 0; i < kResolutionPresetCount; ++i)
            {
                if (kResolutionPresets[i].width == width && kResolutionPresets[i].height == height)
                    return i;
            }
            return -1;
        }

        AppConfig::ScreenMode screenModeFromIndex(int index)
        {
            switch (index)
            {
            case 0:
                return AppConfig::ScreenMode::Windowed;
            case 1:
                return AppConfig::ScreenMode::Fullscreen;
            default:
                return AppConfig::ScreenMode::Borderless;
            }
        }

        std::string fallbackCampaignDir()
        {
            return (utils::executableDir() / "campaigns").string();
        }

        std::string fallbackSaveDir()
        {
            return utils::executableDir().string();
        }

    } // namespace

    void OptionsState::onEnter()
    {
        m_previousScale = ImGui::GetIO().FontGlobalScale;

        AppConfig &config = context().config;
        m_screenMode = static_cast<int>(config.screenMode);
        m_width = static_cast<int>(config.width);
        m_height = static_cast<int>(config.height);

        m_autoUiScale = config.uiScale <= 0.f;
        m_uiScale = m_autoUiScale
                        ? automaticUiScale(context().window.getSize())
                        : std::clamp(config.uiScale, kUiMinScale, kUiMaxScale);

        m_campaignDir = config.campaignDir.empty() ? fallbackCampaignDir() : config.campaignDir;
        m_saveDir = config.saveDir.empty() ? fallbackSaveDir() : config.saveDir;

        m_status.clear();
        m_statusIsError = false;
        applyScale();
    }

    void OptionsState::onExit()
    {
        // Настройки пишутся на диск только при выходе из этого экрана,
        // а не после каждого изменения ползунка или переключателя.
        saveSettings();
        ImGui::GetIO().FontGlobalScale = m_previousScale;
    }

    void OptionsState::handleEvent(const sf::Event &event)
    {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            closeOptions();
    }

    void OptionsState::applyScale()
    {
        const sf::Vector2u windowSize = context().window.getSize();
        const float baseScale = context().config.uiScale > 0.f
                                    ? context().config.uiScale
                                    : automaticUiScale(windowSize);
        ImGui::GetIO().FontGlobalScale =
            std::clamp(baseScale * kOptionsScaleBoost, kUiMinScale, kUiMaxScale);
    }

    void OptionsState::setStatus(std::string text, bool error)
    {
        m_status = std::move(text);
        m_statusIsError = error;
    }

    void OptionsState::drawUi()
    {
        applyScale();

        const sf::Vector2u windowSize = context().window.getSize();
        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)),
                                 ImGuiCond_Always);
        ImGui::Begin("Options", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);

        if (ImGui::Button("< Back"))
            closeOptions();

        ImGui::Separator();

        ImGui::BeginChild("options_content", ImVec2(0.f, 0.f), false);
        drawDisplaySection();
        drawInterfaceSection();
        drawDirectoriesSection();

        ImGui::Separator();
        if (ImGui::Button("Restore defaults"))
            restoreDefaults();

        if (!m_status.empty())
        {
            ImGui::Spacing();
            ImGui::TextColored(m_statusIsError ? ImVec4(0.95f, 0.4f, 0.35f, 1.f)
                                               : ImVec4(0.6f, 0.85f, 0.6f, 1.f),
                               "%s", m_status.c_str());
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void OptionsState::drawDisplaySection()
    {
        ImGui::SeparatorText("Display");

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.f);
        ImGui::Combo("Window mode", &m_screenMode, kScreenModeNames,
                     static_cast<int>(sizeof(kScreenModeNames) / sizeof(kScreenModeNames[0])));

        const bool windowed = m_screenMode == static_cast<int>(AppConfig::ScreenMode::Windowed);
        if (windowed)
        {
            const int preset = matchingResolutionPreset(m_width, m_height);
            const char *preview = preset >= 0 ? kResolutionPresets[preset].label : "Custom";

            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.f);
            if (ImGui::BeginCombo("Resolution preset", preview))
            {
                for (int i = 0; i < kResolutionPresetCount; ++i)
                {
                    const bool selected = i == preset;
                    if (ImGui::Selectable(kResolutionPresets[i].label, selected))
                    {
                        m_width = kResolutionPresets[i].width;
                        m_height = kResolutionPresets[i].height;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.f);
            if (ImGui::InputInt("Width", &m_width))
                m_width = std::clamp(m_width, 640, 7680);
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.f);
            if (ImGui::InputInt("Height", &m_height))
                m_height = std::clamp(m_height, 480, 4320);
        }
        else
        {
            const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
            ImGui::TextDisabled("Resolution: desktop (%u x %u) in this mode.", desktop.width, desktop.height);
        }

        ImGui::TextDisabled("Window mode and resolution are applied when you press the button below.");
        if (ImGui::Button("Apply display settings"))
        {
            if (m_width < 640 || m_height < 480)
            {
                setStatus("Resolution is too small: minimum is 640 x 480.", true);
            }
            else
            {
                AppConfig &config = context().config;
                const AppConfig::ScreenMode newMode = screenModeFromIndex(m_screenMode);
                const bool changed = config.screenMode != newMode ||
                                     config.width != static_cast<unsigned>(m_width) ||
                                     config.height != static_cast<unsigned>(m_height);
                config.screenMode = newMode;
                config.width = static_cast<unsigned>(m_width);
                config.height = static_cast<unsigned>(m_height);

                if (changed)
                    context().windowRecreateRequested = true;
                setStatus(changed ? "Display settings applied."
                                  : "Display settings are up to date.",
                          false);
            }
        }
    }

    void OptionsState::drawInterfaceSection()
    {
        ImGui::SeparatorText("Interface");

        const sf::Vector2u windowSize = context().window.getSize();
        if (m_autoUiScale)
            m_uiScale = automaticUiScale(windowSize);

        bool autoScale = m_autoUiScale;
        if (ImGui::Checkbox("Automatic UI scale (adapt to window size)", &autoScale))
        {
            m_autoUiScale = autoScale;
            if (m_autoUiScale)
                context().config.uiScale = 0.f;
            else
            {
                m_uiScale = std::clamp(m_uiScale, kUiMinScale, kUiMaxScale);
                context().config.uiScale = m_uiScale;
            }
            applyScale();
        }

        if (m_autoUiScale)
        {
            ImGui::TextDisabled("Current automatic scale: %.2fx (window height %u px).",
                                automaticUiScale(windowSize), windowSize.y);
        }
        else
        {
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.f);
            float scale = m_uiScale;
            if (ImGui::SliderFloat("UI scale", &scale, kUiMinScale, kUiMaxScale, "%.2fx"))
            {
                m_uiScale = std::clamp(scale, kUiMinScale, kUiMaxScale);
                context().config.uiScale = m_uiScale;
                applyScale();
            }
            ImGui::TextDisabled("Manual scale overrides the automatic fit for all screens.");
        }
    }

    void OptionsState::drawDirectoriesSection()
    {
        ImGui::SeparatorText("Folders");
        ImGui::TextWrapped("Placeholder: file dialogs currently use the folder next to the executable. "
                           "Custom data directories will be connected later.");

        char campaigns[512];
        char saves[512];
        std::snprintf(campaigns, sizeof(campaigns), "%s", m_campaignDir.c_str());
        std::snprintf(saves, sizeof(saves), "%s", m_saveDir.c_str());

        ImGui::BeginDisabled();
        ImGui::TextUnformatted("Campaigns folder");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##campaigns_dir", campaigns, sizeof(campaigns), ImGuiInputTextFlags_ReadOnly);
        ImGui::TextUnformatted("Saves folder");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##saves_dir", saves, sizeof(saves), ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();
    }

    void OptionsState::saveSettings()
    {
        std::string error;
        const std::filesystem::path path = SettingsSerializer::defaultPath();
        if (SettingsSerializer::save(context().config, path, &error))
            setStatus("Saved: " + path.string(), false);
        else
            setStatus("Save failed: " + error, true);
    }

    void OptionsState::restoreDefaults()
    {
        const AppConfig defaults;
        m_screenMode = static_cast<int>(defaults.screenMode);
        m_width = static_cast<int>(defaults.width);
        m_height = static_cast<int>(defaults.height);

        m_autoUiScale = true;
        m_uiScale = automaticUiScale(context().window.getSize());

        m_campaignDir = fallbackCampaignDir();
        m_saveDir = fallbackSaveDir();

        AppConfig &config = context().config;
        config.uiScale = 0.f;
        config.campaignDir.clear();
        config.saveDir.clear();
        applyScale();

        setStatus("Defaults restored", false);
    }

    void OptionsState::closeOptions()
    {
        context().states.requestPop();
    }

} // namespace dnd::core
