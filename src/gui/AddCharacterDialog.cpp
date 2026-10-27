#include "gui/AddCharacterDialog.hpp"

#include "gui/Widgets.hpp"

#include <imgui.h>

#include <algorithm>

namespace dnd::gui
{

    using namespace entities;

    void AddCharacterDialog::reset()
    {
        m_name = "New Hero";
        m_classIndex = 4;
        m_level = 1;
        m_sideIndex = 0;
        m_scores.fill(AbilityScores::kDefaultScore);
    }

    int AddCharacterDialog::draw(Roster &roster)
    {
        if (m_openRequested)
        {
            reset();
            ImGui::OpenPopup(kPopupId);
            m_openRequested = false;
        }

        int addedIndex = -1;
        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            inputText("Name", m_name);

            const CharacterClass current = kAllClasses[static_cast<std::size_t>(m_classIndex)];
            if (ImGui::BeginCombo("Class", classInfo(current).displayName))
            {
                for (std::size_t i = 0; i < kAllClasses.size(); ++i)
                {
                    const bool selected = static_cast<int>(i) == m_classIndex;
                    if (ImGui::Selectable(classInfo(kAllClasses[i]).displayName, selected))
                        m_classIndex = static_cast<int>(i);
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::InputInt("Level", &m_level);
            m_level = std::clamp(m_level, Character::kMinLevel, Character::kMaxLevel);

            const char *sides[] = {"Player", "Enemy"};
            ImGui::Combo("Side", &m_sideIndex, sides, 2);

            ImGui::SeparatorText("Ability scores");
            for (std::size_t i = 0; i < kAbilityCount; ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                ImGui::InputInt(fullName(kAllAbilities[i]), &m_scores[i]);
                m_scores[i] = std::clamp(m_scores[i], AbilityScores::kMinScore, AbilityScores::kMaxScore);
                ImGui::PopID();
            }
            if (ImGui::Button("Standard array (15 14 13 12 10 8)"))
                m_scores = {15, 14, 13, 12, 10, 8};

            ImGui::Separator();
            if (ImGui::Button("Add"))
            {
                const std::string name = m_name.empty() ? "Unnamed" : m_name;
                const AbilityScores scores(m_scores[0], m_scores[1], m_scores[2],
                                           m_scores[3], m_scores[4], m_scores[5]);
                addedIndex = static_cast<int>(roster.add(
                    Character(name, current, m_level, scores),
                    m_sideIndex == 0 ? Side::Player : Side::Enemy));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        return addedIndex;
    }

} // namespace dnd::gui
