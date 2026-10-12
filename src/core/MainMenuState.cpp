#include "core/MainMenuState.hpp"
#include "core/Application.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <imgui.h>

namespace dnd::core
{

    void MainMenuState::drawUi()
    {
        auto &window = context().window;
        const ImVec2 size(window.getSize().x / 1.5, window.getSize().y / 1.5);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(
            ImVec2(window.getSize().x * 0.5f, window.getSize().y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::Begin("D&D Project", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove);
        ImGui::SetWindowFontScale(6.f);
        const ImVec2 button(-1.f, window.getSize().y / (1.5 * 3.4));
        if (ImGui::Button("Scenario Editor", button))
            context().states.requestPush(StateId::Editor);
        if (ImGui::Button("Game Simulator", button))
            context().states.requestPush(StateId::Simulator);
        if (ImGui::Button("Options", button))
            context().states.requestPush(StateId::Options);
        if (ImGui::Button("Quit", button))
            context().window.close();
        ImGui::End();
    }

    void PlaceholderState::drawUi()
    {
        auto &window = context().window;
        ImGui::SetNextWindowSize(ImVec2(window.getSize().x / 1.5, window.getSize().y / 1.5), ImGuiCond_FirstUseEver);
        ImGui::Begin(m_title);
        ImGui::TextUnformatted("Not implemented yet.");
        if (ImGui::Button("Back"))
            context().states.requestPop();
        ImGui::End();
    }

} // namespace dnd::core
