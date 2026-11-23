#include "gui/CombatPanel.hpp"

#include "entities/AbilityScores.hpp"
#include "gui/Widgets.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

namespace dnd::gui
{

    using namespace entities;

    namespace
    {
        const ImVec4 kGood(0.35f, 0.80f, 0.40f, 1.f);
        const ImVec4 kBad(0.90f, 0.30f, 0.30f, 1.f);
        const ImVec4 kMuted(0.65f, 0.65f, 0.65f, 1.f);
        const ImVec4 kAccent(0.95f, 0.80f, 0.35f, 1.f);

        int abilityIndex(Ability a)
        {
            for (std::size_t i = 0; i < kAbilityCount; ++i)
                if (kAllAbilities[i] == a)
                    return static_cast<int>(i);
            return 0;
        }
    } // namespace

    void CombatPanel::setSceneContext(const scenario::ScenarioGraph *graph, const scenario::Node *node)
    {
        const bool nodeChanged = node != m_node;
        m_graph = graph;
        m_node = node;
        if (nodeChanged && m_node && m_node->check)
            useSceneCheck();
    }

    void CombatPanel::useSceneCheck()
    {
        if (!m_node || !m_node->check)
            return;
        const scenario::CheckDefinition &c = *m_node->check;
        m_checkType = c.type == scenario::CheckType::SavingThrow ? 1 : 0;
        m_ability = abilityIndex(c.ability);
        m_skill = c.skill;
        m_dc = c.dc;
    }

    scenario::CheckDefinition CombatPanel::currentCheck() const
    {
        scenario::CheckDefinition c;
        c.type = m_checkType == 1 ? scenario::CheckType::SavingThrow : scenario::CheckType::SkillCheck;
        c.ability = kAllAbilities[static_cast<std::size_t>(std::clamp(m_ability, 0, static_cast<int>(kAbilityCount) - 1))];
        c.dc = m_dc;
        c.skill = m_skill;
        return c;
    }

    std::optional<bool> CombatPanel::proficiencyOverride() const
    {
        // Wisdom + Perception определяется автоматически по листу героя; иначе — ручной флаг.
        const scenario::CheckDefinition c = currentCheck();
        std::string lower = c.skill;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return std::tolower(ch); });
        if (c.ability == Ability::Wisdom && lower == "perception")
            return std::nullopt;
        return m_skillProficient;
    }

    std::vector<std::string> CombatPanel::takeEvents()
    {
        std::vector<std::string> out;
        out.swap(m_events);
        return out;
    }

    int CombatPanel::draw(Roster &roster, utils::Random &rng)
    {
        int openSheet = -1;
        m_tracker.syncWithRoster(roster);

        if (ImGui::CollapsingHeader("Check / Saving throw", ImGuiTreeNodeFlags_DefaultOpen))
            drawCheckResolver(roster, rng);
        if (ImGui::CollapsingHeader("Initiative tracker", ImGuiTreeNodeFlags_DefaultOpen))
            drawInitiativeTracker(roster, rng, openSheet);
        return openSheet;
    }

    void CombatPanel::drawCheckResolver(Roster &roster, utils::Random &rng)
    {
        ImGui::PushID("check_resolver");

        if (m_node && m_node->check)
        {
            const scenario::CheckDefinition &c = *m_node->check;
            ImGui::TextColored(kAccent, "Scene challenge: %s %s%s%s, DC %d",
                               c.type == scenario::CheckType::SavingThrow ? "Saving throw" : "Skill check",
                               shortName(c.ability), c.skill.empty() ? "" : " / ", c.skill.c_str(), c.dc);
            ImGui::SameLine();
            if (ImGui::SmallButton("Use scene check"))
                useSceneCheck();
        }

        ImGui::RadioButton("Skill check", &m_checkType, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Saving throw", &m_checkType, 1);

        m_ability = std::clamp(m_ability, 0, static_cast<int>(kAbilityCount) - 1);
        if (ImGui::BeginCombo("Ability", fullName(kAllAbilities[static_cast<std::size_t>(m_ability)])))
        {
            for (int i = 0; i < static_cast<int>(kAbilityCount); ++i)
                if (ImGui::Selectable(fullName(kAllAbilities[static_cast<std::size_t>(i)]), i == m_ability))
                    m_ability = i;
            ImGui::EndCombo();
        }

        if (m_checkType == 0)
        {
            inputText("Skill (optional)", m_skill);
            const std::optional<bool> autoProf = proficiencyOverride();
            if (autoProf.has_value())
                ImGui::Checkbox("Proficient in skill (+proficiency bonus)", &m_skillProficient);
            else
                ImGui::TextColored(kMuted, "Perception: proficiency is taken from the hero's sheet.");
        }

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.f);
        ImGui::InputInt("DC", &m_dc);
        m_dc = std::clamp(m_dc, 1, 30);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.f);
        ImGui::InputInt("Extra bonus", &m_extraBonus);
        m_extraBonus = std::clamp(m_extraBonus, -20, 20);

        if (roster.empty())
        {
            ImGui::TextColored(kMuted, "The roster is empty.");
            ImGui::PopID();
            return;
        }

        m_target = std::clamp(m_target, 0, static_cast<int>(roster.size()) - 1);
        const scenario::CheckDefinition check = currentCheck();
        const std::optional<bool> prof = proficiencyOverride();

        const Character &preview = roster.at(static_cast<std::size_t>(m_target)).character;
        char previewMod[16];
        std::snprintf(previewMod, sizeof(previewMod), "%+d",
                      combat::modifierForCheck(preview, check, prof, m_extraBonus));
        const std::string previewLabel = characterLabel(preview) + "  (" + previewMod + ")";
        if (ImGui::BeginCombo("Hero", previewLabel.c_str()))
        {
            for (int i = 0; i < static_cast<int>(roster.size()); ++i)
            {
                const RosterEntry &e = roster.at(static_cast<std::size_t>(i));
                char mod[16];
                std::snprintf(mod, sizeof(mod), "%+d", combat::modifierForCheck(e.character, check, prof, m_extraBonus));
                const std::string label = std::string("[") + toString(e.side) + "] " + characterLabel(e.character) +
                                          "  (" + mod + ")";
                ImGui::PushID(i);
                if (ImGui::Selectable(label.c_str(), i == m_target))
                    m_target = i;
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.f);
        ImGui::InputInt("Player d20 roll", &m_roll);
        m_roll = std::clamp(m_roll, 1, 20);

        const Character &target = roster.at(static_cast<std::size_t>(m_target)).character;
        if (ImGui::Button("Resolve"))
        {
            m_lastOutcome = combat::resolveCheck(target, check, m_roll, prof, m_extraBonus);
            pushEvent(combat::formatCheckOutcome(*m_lastOutcome));
        }
        ImGui::SameLine();
        if (ImGui::Button("Roll d20"))
        {
            m_lastOutcome = combat::rollAndResolveCheck(target, check, rng, prof, m_extraBonus);
            m_roll = m_lastOutcome->diceRoll;
            pushEvent(combat::formatCheckOutcome(*m_lastOutcome));
        }

        if (m_lastOutcome)
        {
            const combat::CheckOutcome &o = *m_lastOutcome;
            ImGui::PushStyleColor(ImGuiCol_Text, o.success ? kGood : kBad);
            ImGui::Text("%s  (roll %d %+d = %d vs DC %d)", o.verdict(), o.diceRoll, o.modifier, o.total, o.dc);
            ImGui::PopStyleColor();
            if (o.natural20)
                ImGui::TextColored(kAccent, "NAT 20!");
            else if (o.natural1)
                ImGui::TextColored(kBad, "NAT 1!");
            ImGui::TextColored(kMuted, "%s", o.characterName.c_str());
        }
        ImGui::PopID();
    }

    void CombatPanel::drawInitiativeTracker(Roster &roster, utils::Random &rng, int &openSheet)
    {
        ImGui::PushID("initiative");

        // --- управление боем ---
        ImGui::Text("Round %d", m_tracker.round());
        if (const combat::Combatant *active = m_tracker.activeCombatant())
        {
            if (active->rosterIndex < roster.size())
            {
                ImGui::SameLine();
                ImGui::TextColored(kAccent, "  Turn: %s", roster.at(active->rosterIndex).character.name.c_str());
            }
        }

        if (ImGui::Button("Start combat"))
        {
            m_tracker.addPlayersFromRoster(roster, nullptr);
            m_tracker.startCombat(&roster);
            pushEvent("Combat started (round 1).");
        }
        ImGui::SameLine();
        if (ImGui::Button("Prev turn"))
            m_tracker.previousTurn(&roster);
        ImGui::SameLine();
        if (ImGui::Button("Next turn"))
        {
            const int roundBefore = m_tracker.round();
            const combat::Combatant *c = m_tracker.nextTurn(&roster);
            if (m_tracker.round() != roundBefore)
                pushEvent("Round " + std::to_string(m_tracker.round()) + " begins.");
            if (c && c->rosterIndex < roster.size())
                pushEvent("Turn: " + roster.at(c->rosterIndex).character.name);
        }

        if (ImGui::Button("Roll enemies init"))
            m_tracker.rollInitiativeForEnemies(roster, rng);
        ImGui::SameLine();
        if (ImGui::Button("Roll all init"))
            m_tracker.rollInitiativeForAll(roster, rng);
        ImGui::SameLine();
        if (ImGui::Button("Sync heroes"))
            m_tracker.addPlayersFromRoster(roster, nullptr);
        ImGui::SameLine();
        if (ImGui::Button("Clear enemies"))
            m_tracker.removeEnemies(roster);
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            m_tracker.removeEnemies(roster);
            m_tracker.clear();
            pushEvent("Combat ended.");
        }

        // --- загрузка врагов ---
        if (m_graph && m_node && !m_node->enemyIds.empty())
        {
            const std::string label = "Load scene foes (" + std::to_string(m_node->enemyIds.size()) + ")";
            if (ImGui::Button(label.c_str()))
            {
                const std::size_t n = m_tracker.loadNodeEnemies(roster, *m_graph, *m_node, &rng, false);
                pushEvent("Loaded " + std::to_string(n) + " enemy(ies) from the scene.");
            }
        }
        if (m_graph && !m_graph->enemies.empty())
        {
            m_enemyPick = std::clamp(m_enemyPick, 0, static_cast<int>(m_graph->enemies.size()) - 1);
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.f);
            if (ImGui::BeginCombo("##bestiary", m_graph->enemies[static_cast<std::size_t>(m_enemyPick)].name.c_str()))
            {
                for (int i = 0; i < static_cast<int>(m_graph->enemies.size()); ++i)
                {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(m_graph->enemies[static_cast<std::size_t>(i)].name.c_str(), i == m_enemyPick))
                        m_enemyPick = i;
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Add enemy"))
            {
                const scenario::EnemyTemplate &tpl = m_graph->enemies[static_cast<std::size_t>(m_enemyPick)];
                const std::size_t idx = m_tracker.spawnEnemyFromTemplate(roster, tpl, &rng);
                pushEvent("Enemy joined the fight: " + roster.at(idx).character.name);
            }
        }

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.f);
        ImGui::InputInt("Damage / heal amount", &m_amount);
        m_amount = std::max(0, m_amount);

        if (m_tracker.empty())
        {
            ImGui::TextColored(kMuted, "No combatants. Press \"Start combat\" or load enemies.");
            ImGui::PopID();
            return;
        }

        // --- таблица очереди ---
        int pendingInitIndex = -1;
        int pendingInitValue = 0;
        int removeIndex = -1;

        if (ImGui::BeginTable("queue", 8,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Turn", ImGuiTableColumnFlags_WidthStretch, 0.5f);
            ImGui::TableSetupColumn("Init", ImGuiTableColumnFlags_WidthStretch, 1.2f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.8f);
            ImGui::TableSetupColumn("Side", ImGuiTableColumnFlags_WidthStretch, 0.8f);
            ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthStretch, 2.4f);
            ImGui::TableSetupColumn("AC", ImGuiTableColumnFlags_WidthStretch, 0.5f);
            ImGui::TableSetupColumn("Conditions", ImGuiTableColumnFlags_WidthStretch, 2.2f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 3.2f);
            ImGui::TableHeadersRow();

            const auto &list = m_tracker.combatants();
            for (std::size_t i = 0; i < list.size(); ++i)
            {
                const combat::Combatant &cb = list[i];
                if (cb.rosterIndex >= roster.size())
                    continue;
                RosterEntry &entry = roster.at(cb.rosterIndex);
                Character &c = entry.character;
                const bool isActive = static_cast<int>(i) == m_tracker.activeTurnIndex();

                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                if (isActive)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(60, 90, 140, 130));
                else if (c.isDown())
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(120, 30, 30, 110));

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(isActive ? ">>" : "");

                ImGui::TableSetColumnIndex(1);
                int init = cb.initiative;
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputInt("##init", &init, 1, 5))
                {
                    pendingInitIndex = static_cast<int>(i);
                    pendingInitValue = init;
                }

                ImGui::TableSetColumnIndex(2);
                if (c.isDown())
                    ImGui::TextColored(kBad, "%s (down)", c.name.c_str());
                else
                    ImGui::TextUnformatted(c.name.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(toString(entry.side));

                ImGui::TableSetColumnIndex(4);
                hpBar(c);

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%d", c.armorClass());

                ImGui::TableSetColumnIndex(6);
                if (c.conditions().empty())
                    ImGui::TextColored(kMuted, "-");
                else
                    conditionBadges(c);

                ImGui::TableSetColumnIndex(7);
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
                ImGui::SameLine();
                if (ImGui::SmallButton("Sheet"))
                    openSheet = static_cast<int>(cb.rosterIndex);
                ImGui::SameLine();
                if (ImGui::SmallButton("X"))
                    removeIndex = static_cast<int>(i);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (pendingInitIndex >= 0)
            m_tracker.setInitiative(static_cast<std::size_t>(pendingInitIndex), pendingInitValue, &roster);
        if (removeIndex >= 0)
            m_tracker.removeCombatant(static_cast<std::size_t>(removeIndex));

        ImGui::PopID();
    }

} // namespace dnd::gui
