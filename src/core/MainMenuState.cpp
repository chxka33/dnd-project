#include "core/MainMenuState.hpp"
#include "core/Application.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <imgui.h>

namespace dnd::core {

void MainMenuState::drawUi() {
    auto& window = context().window;
    const ImVec2 size(320.f, 220.f);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(window.getSize().x * 0.5f, window.getSize().y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("D&D Project", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoMove);
    const ImVec2 button(-1.f, 48.f);
    if (ImGui::Button("Scenario Editor", button)) context().states.requestPush(StateId::Editor);
    if (ImGui::Button("Game Simulator", button))  context().states.requestPush(StateId::Simulator);
    if (ImGui::Button("Quit", button))            context().window.close();
    ImGui::End();
}

void PlaceholderState::drawUi() {
    ImGui::SetNextWindowSize(ImVec2(360.f, 140.f), ImGuiCond_FirstUseEver);
    ImGui::Begin(m_title);
    ImGui::TextUnformatted("Not implemented yet.");
    if (ImGui::Button("Back")) context().states.requestPop();
    ImGui::End();
}

} // namespace dnd::core
