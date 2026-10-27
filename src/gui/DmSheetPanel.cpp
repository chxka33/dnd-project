#include "gui/DmSheetPanel.hpp"

#include "gui/Widgets.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace dnd::gui
{

    using namespace entities;

    namespace
    {
        const ImVec4 kBad(0.90f, 0.30f, 0.30f, 1.f);
        const ImVec4 kMuted(0.65f, 0.65f, 0.65f, 1.f);

        // "L1 2/4  L2 3/3" — компактная сводка ячеек заклинаний.
        std::string slotsSummary(const Character &c)
        {
            const SpellSlots &slots = c.spellSlots();
            std::string text;
            for (int level = 1; level <= SpellSlots::kMaxSpellLevel; ++level)
            {
                if (slots.max(level) == 0)
                    continue;
                char part[32];
                std::snprintf(part, sizeof(part), "%sL%d %d/%d", text.empty() ? "" : "  ", level,
                              slots.current(level), slots.max(level));
                text += part;
            }
            return text.empty() ? "-" : text;
        }
    } // namespace

    int DmSheetPanel::draw(Roster &roster)
    {
        int openSheet = -1;

        if (roster.empty())
        {
            ImGui::TextWrapped("The roster is empty. Use \"Add character\" or \"Add demo party\" above.");
            return openSheet;
        }

        ImGui::Text("Players: %zu/%zu up    Enemies: %zu/%zu up", roster.countAlive(Side::Player),
                    roster.count(Side::Player), roster.countAlive(Side::Enemy), roster.count(Side::Enemy));

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.f);
        ImGui::InputInt("Amount", &m_amount);
        m_amount = std::max(0, m_amount);
        ImGui::SameLine();
        if (ImGui::Button("Long rest (all players)"))
            roster.longRestPlayers();
        ImGui::TextColored(kMuted, "Dmg / Heal apply \"Amount\" to that character.");

        ImGui::SeparatorText("Players");
        drawSide(roster, Side::Player, openSheet);
        ImGui::SeparatorText("Enemies");
        drawSide(roster, Side::Enemy, openSheet);

        // Подтверждение удаления — вне таблиц, в ID-контексте панели.
        if (m_openRemovePopup)
        {
            ImGui::OpenPopup("Remove from roster?");
            m_openRemovePopup = false;
        }
        if (confirmRemovePopup("Remove from roster?", m_removeName))
        {
            if (m_removeIndex >= 0)
                roster.remove(static_cast<std::size_t>(m_removeIndex));
            m_removeIndex = -1;
        }
        return openSheet;
    }

    void DmSheetPanel::drawSide(Roster &roster, Side side, int &openSheet)
    {
        if (roster.count(side) == 0)
        {
            ImGui::TextColored(kMuted, "None.");
            return;
        }

        ImGui::PushID(static_cast<int>(side));
        if (ImGui::BeginTable("table", 10,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.f);
            ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthStretch, 1.6f);
            ImGui::TableSetupColumn("Lv", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthStretch, 3.f);
            ImGui::TableSetupColumn("AC", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("Init", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("Prof", ImGuiTableColumnFlags_WidthStretch, 0.7f);
            ImGui::TableSetupColumn("Spell DC", ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableSetupColumn("Slots", ImGuiTableColumnFlags_WidthStretch, 3.f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 3.f);
            ImGui::TableHeadersRow();

            for (std::size_t i = 0; i < roster.size(); ++i)
            {
                RosterEntry &entry = roster.at(i);
                if (entry.side != side)
                    continue;
                Character &c = entry.character;

                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                if (c.isDown())
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(120, 30, 30, 110));

                ImGui::TableSetColumnIndex(0);
                if (c.isDown())
                    ImGui::TextColored(kBad, "%s (down)", c.name.c_str());
                else
                    ImGui::TextUnformatted(c.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(c.classData().displayName);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", c.level());
                ImGui::TableSetColumnIndex(3);
                hpBar(c);
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%d", c.armorClass());
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%+d", c.initiativeModifier());
                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%+d", c.proficiencyBonus());
                ImGui::TableSetColumnIndex(7);
                if (const auto dc = c.spellSaveDc(); dc && c.spellSlots().hasAny())
                    ImGui::Text("%d", *dc);
                else
                    ImGui::TextUnformatted("-");
                ImGui::TableSetColumnIndex(8);
                ImGui::TextUnformatted(slotsSummary(c).c_str());

                ImGui::TableSetColumnIndex(9);
                if (ImGui::SmallButton("Dmg"))
                    c.applyDamage(m_amount);
                ImGui::SameLine();
                if (ImGui::SmallButton("Heal"))
                    c.heal(m_amount);
                ImGui::SameLine();
                if (ImGui::SmallButton("Sheet"))
                    openSheet = static_cast<int>(i);
                ImGui::SameLine();
                if (ImGui::SmallButton("X"))
                {
                    m_removeIndex = static_cast<int>(i);
                    m_removeName = c.name;
                    m_openRemovePopup = true;
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

} // namespace dnd::gui
