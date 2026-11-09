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
        const ImVec4 kLoot(0.95f, 0.80f, 0.35f, 1.f);

        // "DC 13  L1 2/4  L2 3/3" — компактная сводка заклинаний: СЛ и ячейки.
        std::string slotsSummary(const Character &c)
        {
            const SpellSlots &slots = c.spellSlots();
            if (!slots.hasAny())
                return "-";

            std::string text;
            if (const auto dc = c.spellSaveDc())
                text = "DC " + std::to_string(*dc);
            for (int level = 1; level <= SpellSlots::kMaxSpellLevel; ++level)
            {
                if (slots.max(level) == 0)
                    continue;
                char part[32];
                std::snprintf(part, sizeof(part), "%sL%d %d/%d", text.empty() ? "" : "  ", level,
                              slots.current(level), slots.max(level));
                text += part;
            }
            return text;
        }

        // "50 gp, Potion of Healing x2, Longsword"
        std::string lootSummary(const scenario::Loot &loot)
        {
            std::string text;
            if (loot.gold > 0)
                text = std::to_string(loot.gold) + " gp";
            for (const scenario::LootItem &item : loot.items)
            {
                if (item.quantity <= 0 || item.name.empty())
                    continue;
                if (!text.empty())
                    text += ", ";
                text += item.name;
                if (item.quantity > 1)
                    text += " x" + std::to_string(item.quantity);
            }
            return text;
        }

        bool lootIsEmpty(const scenario::Loot &loot)
        {
            return lootSummary(loot).empty();
        }
    } // namespace

    int DmSheetPanel::draw(Roster &roster)
    {
        int openSheet = -1;

        if (roster.empty())
        {
            ImGui::TextWrapped("The roster is empty. Use \"Add character\" above.");
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
        ImGui::TextColored(kMuted, "-HP / +HP apply \"Amount\" to that character.");
        drawLootBanner();

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

    int DmSheetPanel::takeLootRecipient()
    {
        const int recipient = m_lootRecipient;
        m_lootRecipient = -1;
        return recipient;
    }

    void DmSheetPanel::drawLootBanner()
    {
        if (m_loot && !lootIsEmpty(*m_loot))
            ImGui::TextColored(kLoot, "Scene loot: %s  -  press \"Loot\" on a hero to give it.",
                               lootSummary(*m_loot).c_str());
        if (!m_status.empty())
            ImGui::TextColored(kMuted, "%s", m_status.c_str());
    }

    void DmSheetPanel::drawSide(Roster &roster, Side side, int &openSheet)
    {
        if (roster.count(side) == 0)
        {
            ImGui::TextColored(kMuted, "None.");
            return;
        }

        // Лут получают только герои; у противников кнопки нет.
        const bool canLoot = side == Side::Player && m_loot && !lootIsEmpty(*m_loot);

        ImGui::PushID(static_cast<int>(side));
        if (ImGui::BeginTable("table", 10,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.8f);
            ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthStretch, 2.6f);
            ImGui::TableSetupColumn("AC", ImGuiTableColumnFlags_WidthStretch, 0.55f);
            ImGui::TableSetupColumn("PP", ImGuiTableColumnFlags_WidthStretch, 0.55f);
            ImGui::TableSetupColumn("Init", ImGuiTableColumnFlags_WidthStretch, 0.65f);
            ImGui::TableSetupColumn("Spells", ImGuiTableColumnFlags_WidthStretch, 2.4f);
            ImGui::TableSetupColumn("Conditions", ImGuiTableColumnFlags_WidthStretch, 2.4f);
            ImGui::TableSetupColumn("Equipment", ImGuiTableColumnFlags_WidthStretch, 2.6f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 4.2f);
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
                ImGui::Text("%s %d", c.classData().displayName, c.level());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Proficiency bonus %+d", c.proficiencyBonus());

                ImGui::TableSetColumnIndex(2);
                hpBar(c);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", c.armorClass());

                // Пассивная внимательность — для скрытых проверок за ширмой.
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%d", c.passivePerception());
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Passive Perception: 10 + WIS %+d%s",
                                      c.modifier(Ability::Wisdom),
                                      c.isProficientInPerception() ? " + proficiency" : "");

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%+d", c.initiativeModifier());

                ImGui::TableSetColumnIndex(6);
                ImGui::TextUnformatted(slotsSummary(c).c_str());

                ImGui::TableSetColumnIndex(7);
                if (c.conditions().empty())
                    ImGui::TextColored(kMuted, "-");
                else
                    conditionBadges(c);

                ImGui::TableSetColumnIndex(8);
                ImGui::TextWrapped("%s", equipmentSummary(c).c_str());
                if (ImGui::IsItemHovered())
                    inventoryTooltip(c);

                ImGui::TableSetColumnIndex(9);
                if (ImGui::SmallButton("-HP"))
                    c.applyDamage(m_amount);
                ImGui::SameLine();
                if (ImGui::SmallButton("+HP"))
                    c.heal(m_amount);
                ImGui::SameLine();
                if (ImGui::SmallButton("Cond"))
                    ImGui::OpenPopup("conditions");
                if (ImGui::BeginPopup("conditions"))
                {
                    conditionMenu(c);
                    ImGui::EndPopup();
                }
                if (canLoot)
                {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Loot"))
                    {
                        const LootResult got = c.receiveLoot(*m_loot);
                        m_lootRecipient = static_cast<int>(i);
                        char msg[160];
                        std::snprintf(msg, sizeof(msg), "%s received %d gp and %d item(s).",
                                      c.name.c_str(), got.gold, got.itemUnits);
                        m_status = msg;
                    }
                }
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
