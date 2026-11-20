#include "core/CharactersState.hpp"

#include "core/Application.hpp"
#include "entities/ItemPresets.hpp"
#include "gui/Widgets.hpp"
#include "scenario/CharacterSerializer.hpp"
#include "utils/ExecutablePath.hpp"

#include <filesystem>
#include <system_error>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <imgui.h>

#include <algorithm>
#include <string>

namespace dnd::core
{

    void CharactersState::applyScale() const
    {
        // FontGlobalScale действует и на всплывающие окна (выпадающие списки, диалоги).
        ImGui::GetIO().FontGlobalScale = m_scale;
    }

    void CharactersState::onEnter()
    {
        m_previousScale = ImGui::GetIO().FontGlobalScale;
        applyScale();
        entities::registerRosterItems(context().roster);
    }

    void CharactersState::onExit()
    {
        ImGui::GetIO().FontGlobalScale = m_previousScale;
    }

    void CharactersState::handleEvent(const sf::Event &event)
    {
        if (event.type != sf::Event::KeyPressed || event.key.code != sf::Keyboard::Escape)
            return;
        // Если открыт список или диалог — Esc закрывает его, а не весь экран.
        if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
            return;
        context().states.requestPop();
    }

    void CharactersState::drawRosterList(entities::Roster &roster)
    {
        // Экран создаёт и настраивает только героев. Противники приходят из сценария
        // (редактор) или добавляются в бою, поэтому здесь они не показываются.
        if (roster.count(entities::Side::Player) == 0)
        {
            ImGui::TextWrapped("No characters yet. Use \"Add character\" above.");
            return;
        }

        ImGui::SeparatorText("Party");
        for (std::size_t i = 0; i < roster.size(); ++i)
        {
            const entities::RosterEntry &e = roster.at(i);
            if (e.side != entities::Side::Player)
                continue;
            ImGui::PushID(static_cast<int>(i));
            const std::string label = gui::characterLabel(e.character);
            if (ImGui::Selectable(label.c_str(), static_cast<int>(i) == m_sheet.selected()))
                m_sheet.select(static_cast<int>(i));
            ImGui::PopID();
        }
    }

    void CharactersState::drawUi()
    {
        entities::Roster &roster = context().roster;
        const sf::Vector2u size = context().window.getSize();

        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(size.x), static_cast<float>(size.y)), ImGuiCond_Always);
        ImGui::Begin("Characters", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);

        // --- Панель инструментов ---
        if (ImGui::Button("< Back"))
            context().states.requestPop();
        ImGui::SameLine();
        if (ImGui::Button("Add character"))
            m_addDialog.open();
        ImGui::SameLine();
        if (ImGui::Button("Save"))
            m_openSave = true;
        ImGui::SameLine();
        if (ImGui::Button("Load"))
            m_openLoad = true;
        ImGui::SameLine();
        ImGui::TextUnformatted("UI scale");
        ImGui::SameLine();
        if (ImGui::Button("-"))
        {
            m_scale = std::max(1.0f, m_scale - 0.25f);
            applyScale();
        }
        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            m_scale = std::min(4.0f, m_scale + 0.25f);
            applyScale();
        }
        if (!m_status.empty())
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(m_status.c_str());
        }
        ImGui::Separator();

        // --- Слева: список, справа: настройка выбранного ---
        const float listWidth = ImGui::GetFontSize() * 16.f;
        ImGui::BeginChild("roster_list", ImVec2(listWidth, 0.f), true);
        drawRosterList(roster);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("character_setup");
        if (roster.count(entities::Side::Player) > 0)
        {
            // Выбранным всегда должен быть герой (противник мог оказаться выбранным после удаления).
            const int selected = m_sheet.selected();
            const bool valid = selected >= 0 && selected < static_cast<int>(roster.size()) &&
                               roster.at(static_cast<std::size_t>(selected)).side == entities::Side::Player;
            if (!valid)
            {
                for (std::size_t i = 0; i < roster.size(); ++i)
                {
                    if (roster.at(i).side == entities::Side::Player)
                    {
                        m_sheet.select(static_cast<int>(i));
                        break;
                    }
                }
            }
            m_sheet.draw(roster);
        }
        ImGui::EndChild();

        const int added = m_addDialog.draw(roster);
        if (added >= 0)
            m_sheet.select(added);

        drawFileDialogs(roster);

        ImGui::End();
    }

    void CharactersState::drawFileDialogs(entities::Roster &roster)
    {
        namespace fs = std::filesystem;
        const fs::path saveDir = utils::executableDir();

        if (m_openSave)
        {
            ImGui::OpenPopup("Save characters");
            m_openSave = false;
        }
        if (m_openLoad)
        {
            ImGui::OpenPopup("Load characters");
            m_openLoad = false;
        }

        if (ImGui::BeginPopupModal("Save characters", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("File (extension .dndch):");
            ImGui::InputText("##fname", m_fileName, sizeof(m_fileName));
            if (ImGui::Button("Save"))
            {
                std::string name = m_fileName;
                if (name.empty())
                    name = "party";
                if (fs::path(name).extension() != ".dndch")
                    name += ".dndch";
                // Только имя файла: без вложенных путей.
                const fs::path path = saveDir / fs::path(name).filename();
                std::string err;
                if (scenario::CharacterSerializer::saveRosterToFile(roster, path, &err))
                    m_status = "Saved: " + path.string();
                else
                    m_status = "Save failed: " + err;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Load characters", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::vector<fs::path> files;
            std::error_code ec;
            if (fs::is_directory(saveDir, ec))
            {
                for (const auto &entry : fs::directory_iterator(saveDir, ec))
                    if (entry.is_regular_file() && entry.path().extension() == ".dndch")
                        files.push_back(entry.path());
                std::sort(files.begin(), files.end());
            }
            if (files.empty())
                ImGui::TextUnformatted("No saved .dndch files found.");
            for (const auto &p : files)
            {
                if (ImGui::Selectable(p.filename().string().c_str()))
                {
                    entities::Roster loaded;
                    std::string err;
                    if (scenario::CharacterSerializer::loadRosterFromFile(p, loaded, &err))
                    {
                        roster = std::move(loaded);
                        m_sheet.select(0);
                        m_status = "Loaded: " + p.string();
                    }
                    else
                        m_status = "Load failed: " + err;
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

} // namespace dnd::core
