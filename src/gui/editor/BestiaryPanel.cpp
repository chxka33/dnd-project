#include "gui/editor/BestiaryPanel.hpp"

#include "gui/editor/EditorOps.hpp"
#include "gui/editor/EditorWidgets.hpp"

#include <imgui.h>

#include <algorithm>

namespace dnd::gui::editor
{
    using namespace dnd::scenario;

    std::optional<NodeId> BestiaryPanel::draw(ScenarioGraph &graph, const NodeId &currentScene)
    {
        std::optional<NodeId> focusRequest;
        ImGui::PushID("bestiary");

        if (ImGui::Button("Add monster"))
        {
            EnemyTemplate enemy;
            enemy.name = "New monster";
            enemy.id = makeEnemyId(graph, "monster");
            m_selected = enemy.id;
            graph.addEnemy(std::move(enemy));
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%d in bestiary", static_cast<int>(graph.enemies.size()));

        // --- список ---
        const float listHeight = ImGui::GetTextLineHeightWithSpacing() * 8.f;
        ImGui::BeginChild("list", ImVec2(0.f, listHeight), true);
        if (graph.enemies.empty())
            ImGui::TextDisabled("No monsters yet.");
        for (const EnemyTemplate &enemy : graph.enemies)
        {
            ImGui::PushID(enemy.id.c_str());
            const std::string text = enemy.name + "   AC " + std::to_string(enemy.armorClass) +
                                     "  HP " + std::to_string(enemy.maxHp);
            if (ImGui::Selectable(text.c_str(), enemy.id == m_selected))
                m_selected = enemy.id;
            ImGui::PopID();
        }
        ImGui::EndChild();

        // --- редактор выбранного монстра ---
        EnemyTemplate *enemy = graph.findEnemy(m_selected);
        if (!enemy)
        {
            ImGui::TextWrapped("Select a monster to edit it.");
            ImGui::PopID();
            return focusRequest;
        }

        ImGui::SeparatorText("Edit monster");
        ImGui::TextDisabled("id: %s", enemy->id.c_str());

        ImGui::TextUnformatted("Name");
        ImGui::SetNextItemWidth(-FLT_MIN);
        inputTextStd("##name", enemy->name);

        ImGui::TextUnformatted("Armor Class (AC)");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.f);
        intField("##ac", enemy->armorClass, 1, 40);

        ImGui::TextUnformatted("Max HP");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.f);
        if (intField("##maxhp", enemy->maxHp, 1, 9999))
            enemy->currentHp = std::min(enemy->currentHp, enemy->maxHp);

        ImGui::TextUnformatted("Current HP");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.f);
        intField("##hp", enemy->currentHp, 0, enemy->maxHp);

        ImGui::TextUnformatted("Initiative bonus");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.f);
        intField("##init", enemy->initiativeBonus, -10, 20);

        // --- привязка к сценам ---
        ImGui::SeparatorText("Used in scenes");
        const std::string enemyId = enemy->id;
        bool any = false;
        for (const NodeId &id : graph.order())
        {
            const Node &node = graph.at(id);
            if (std::find(node.enemyIds.begin(), node.enemyIds.end(), enemyId) == node.enemyIds.end())
                continue;
            any = true;
            ImGui::PushID(id.c_str());
            if (ImGui::SmallButton("Show"))
                focusRequest = id;
            ImGui::SameLine();
            ImGui::TextUnformatted(node.title.empty() ? id.c_str() : node.title.c_str());
            ImGui::PopID();
        }
        if (!any)
            ImGui::TextDisabled("Not attached to any scene.");

        if (!currentScene.empty() && graph.contains(currentScene))
        {
            const Node &scene = graph.at(currentScene);
            const bool attached = std::find(scene.enemyIds.begin(), scene.enemyIds.end(), enemyId) !=
                                  scene.enemyIds.end();
            const std::string sceneName = scene.title.empty() ? currentScene : scene.title;
            if (attached)
            {
                if (ImGui::Button(("Detach from \"" + sceneName + "\"").c_str()))
                    detachEnemy(graph, currentScene, enemyId);
            }
            else if (ImGui::Button(("Attach to \"" + sceneName + "\"").c_str()))
            {
                attachEnemy(graph, currentScene, enemyId);
            }
        }
        else
        {
            ImGui::TextDisabled("Select a scene on the map to attach this monster to it.");
        }

        // --- удаление ---
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.2f, 0.2f, 1.f));
        if (ImGui::Button("Remove monster..."))
            ImGui::OpenPopup("confirm_remove");
        ImGui::PopStyleColor(2);

        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("confirm_remove", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const std::size_t uses = enemyUsageCount(graph, enemyId);
            ImGui::Text("Remove \"%s\" from the bestiary?", enemy->name.c_str());
            if (uses > 0)
                ImGui::Text("It will also be detached from %d scene(s).", static_cast<int>(uses));
            ImGui::Separator();
            if (ImGui::Button("Remove"))
            {
                removeEnemyEverywhere(graph, enemyId);
                m_selected.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::PopID();
        return focusRequest;
    }

} // namespace dnd::gui::editor
