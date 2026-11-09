#include "core/CharactersState.hpp"

#include "core/Application.hpp"
#include "gui/Widgets.hpp"

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

        ImGui::End();
    }

} // namespace dnd::core
