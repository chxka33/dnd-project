#include "gui/editor/NodeInspector.hpp"

#include "gui/editor/EditorOps.hpp"
#include "gui/editor/EditorWidgets.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <string>

namespace dnd::gui::editor
{
    using namespace dnd::scenario;

    namespace
    {
        constexpr std::array<Ability, 6> kAbilities = {Ability::Strength, Ability::Dexterity, Ability::Constitution,
                                                       Ability::Intelligence, Ability::Wisdom, Ability::Charisma};
        constexpr std::array<const char *, 6> kAbilityNames = {"Strength", "Dexterity", "Constitution",
                                                               "Intelligence", "Wisdom", "Charisma"};
        constexpr std::array<int, 7> kDice = {4, 6, 8, 10, 12, 20, 100};

        void label(const char *text)
        {
            ImGui::TextUnformatted(text);
        }
    } // namespace

    bool NodeInspector::draw(ScenarioGraph &graph, Node &node)
    {
        bool deleteRequested = false;
        ImGui::PushID(node.id().c_str());

        // --- шапка ---
        ImGui::TextColored(nodeTypeColorVec(node.type()), "%s", nodeTypeLabel(node.type()));
        ImGui::SameLine();
        ImGui::TextDisabled("id: %s", node.id().c_str());
        if (graph.startId() == node.id())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.f), "[START]");
        }
        else
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Set as start"))
                graph.setStart(node.id());
        }
        ImGui::Spacing();

        // --- основные тексты ---
        label("Title");
        ImGui::SetNextItemWidth(-FLT_MIN);
        inputTextStd("##title", node.title);

        label("Summary (shown on the map)");
        inputMultilineStd("##summary", node.summary, 3);

        label("Text for the players (read aloud)");
        inputMultilineStd("##text", node.text, 6);

        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.f), "Master's notes (secret)");
        inputMultilineStd("##notes", node.dmNotes, 6);

        // --- переходы (зависят от типа узла) ---
        if (ImGui::CollapsingHeader("Transitions", ImGuiTreeNodeFlags_DefaultOpen))
        {
            switch (node.type())
            {
            case NodeType::Story:
                drawStoryTransitions(graph, static_cast<StoryNode &>(node));
                break;
            case NodeType::Choice:
                drawChoiceTransitions(graph, static_cast<ChoiceNode &>(node));
                break;
            case NodeType::Random:
                drawRandomTable(graph, static_cast<RandomNode &>(node));
                break;
            }
        }

        if (ImGui::CollapsingHeader("Challenge", ImGuiTreeNodeFlags_DefaultOpen))
            drawChallenge(node);
        if (ImGui::CollapsingHeader("Loot", ImGuiTreeNodeFlags_DefaultOpen))
            drawLoot(node);
        if (ImGui::CollapsingHeader("Monsters in this scene", ImGuiTreeNodeFlags_DefaultOpen))
            drawEnemies(graph, node);

        // --- удаление ---
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.2f, 0.2f, 1.f));
        if (ImGui::Button("Delete scene..."))
            deleteRequested = true;
        ImGui::PopStyleColor(2);

        ImGui::PopID();
        return deleteRequested;
    }

    // ------------------------------------------------------------------ испытание

    void NodeInspector::drawChallenge(Node &node)
    {
        bool has = node.check.has_value();
        if (ImGui::Checkbox("This scene has a skill check / saving throw", &has))
        {
            if (has)
                node.check = CheckDefinition{};
            else
                node.check.reset();
        }
        if (!node.check)
            return;

        CheckDefinition &check = *node.check;

        int type = check.type == CheckType::SavingThrow ? 1 : 0;
        if (ImGui::RadioButton("Skill check", &type, 0))
            check.type = CheckType::SkillCheck;
        ImGui::SameLine();
        if (ImGui::RadioButton("Saving throw", &type, 1))
            check.type = CheckType::SavingThrow;

        label("Ability");
        int abilityIndex = static_cast<int>(
            std::find(kAbilities.begin(), kAbilities.end(), check.ability) - kAbilities.begin());
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##ability", &abilityIndex, kAbilityNames.data(), static_cast<int>(kAbilityNames.size())))
            check.ability = kAbilities[static_cast<std::size_t>(abilityIndex)];

        if (check.type == CheckType::SkillCheck)
        {
            label("Skill (optional, e.g. Stealth)");
            ImGui::SetNextItemWidth(-FLT_MIN);
            inputTextStd("##skill", check.skill);
        }

        label("Difficulty Class (DC)");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SliderInt("##dc", &check.dc, 1, 30);
    }

    // ------------------------------------------------------------------ награды

    void NodeInspector::drawLoot(Node &node)
    {
        bool has = node.loot.has_value();
        if (ImGui::Checkbox("This scene gives a reward", &has))
        {
            if (has)
                node.loot = Loot{};
            else
                node.loot.reset();
        }
        if (!node.loot)
            return;

        Loot &loot = *node.loot;

        label("Gold");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.f);
        intField("##gold", loot.gold, 0, 1000000, 1);

        label("Items");
        int removeIndex = -1;
        for (std::size_t i = 0; i < loot.items.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            LootItem &item = loot.items[i];
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 13.f);
            inputTextStd("##name", item.name);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.f);
            intField("##qty", item.quantity, 1, 9999, 1);
            ImGui::SameLine();
            if (ImGui::SmallButton("x"))
                removeIndex = static_cast<int>(i);
            ImGui::PopID();
        }
        if (removeIndex >= 0)
            loot.items.erase(loot.items.begin() + removeIndex);
        if (ImGui::Button("Add item"))
            loot.items.push_back({"New item", 1});
    }

    // ------------------------------------------------------------------ переходы

    void NodeInspector::drawStoryTransitions(ScenarioGraph &graph, StoryNode &node)
    {
        label("Next scene");
        NodeId next = node.next();
        if (nodeCombo("##next", graph, next, node.id()))
            node.setNext(next);
        if (node.isEnding())
            ImGui::TextDisabled("No next scene: the story ends here.");
    }

    void NodeInspector::drawChoiceTransitions(ScenarioGraph &graph, ChoiceNode &node)
    {
        auto &choices = node.choices();
        int removeIndex = -1;
        for (std::size_t i = 0; i < choices.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Separator();
            ImGui::Text("Option %d", static_cast<int>(i) + 1);
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
                removeIndex = static_cast<int>(i);

            label("Text");
            ImGui::SetNextItemWidth(-FLT_MIN);
            inputTextStd("##label", choices[i].label);
            label("Leads to");
            nodeCombo("##target", graph, choices[i].target, node.id());
            ImGui::PopID();
        }
        if (removeIndex >= 0)
            node.removeChoice(static_cast<std::size_t>(removeIndex));

        if (choices.empty())
            ImGui::TextDisabled("A fork needs at least one option.");
        if (ImGui::Button("Add option"))
            node.addChoice("New choice");
    }

    void NodeInspector::drawRandomTable(ScenarioGraph &graph, RandomNode &node)
    {
        // --- кость ---
        label("Die");
        const std::string preview = "d" + std::to_string(node.diceSides());
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.f);
        if (ImGui::BeginCombo("##die", preview.c_str()))
        {
            for (int sides : kDice)
            {
                const std::string name = "d" + std::to_string(sides);
                if (ImGui::Selectable(name.c_str(), sides == node.diceSides()))
                    node.setDiceSides(sides);
            }
            ImGui::EndCombo();
        }

        // --- строки таблицы ---
        const int sides = node.diceSides();
        auto &entries = node.entries();
        int removeIndex = -1;
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            RollTableEntry &entry = entries[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::Separator();

            ImGui::TextUnformatted("Roll");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.f);
            intField("##min", entry.minRoll, 1, sides, 1);
            ImGui::SameLine();
            ImGui::TextUnformatted("to");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8.f);
            intField("##max", entry.maxRoll, 1, sides, 1);
            if (entry.maxRoll < entry.minRoll && !ImGui::IsItemActive())
                entry.maxRoll = entry.minRoll;
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
                removeIndex = static_cast<int>(i);

            label("Outcome");
            ImGui::SetNextItemWidth(-FLT_MIN);
            inputTextStd("##label", entry.label);
            label("Leads to");
            nodeCombo("##target", graph, entry.target, node.id());
            ImGui::PopID();
        }
        if (removeIndex >= 0)
            node.removeEntry(static_cast<std::size_t>(removeIndex));

        if (ImGui::Button("Add row"))
        {
            const int roll = nextFreeRoll(node);
            node.addEntry(roll, roll, "New outcome", {});
        }

        // --- покрытие кубика ---
        const RollCoverage coverage = rollCoverage(node);
        ImGui::Text("Table covers %d of %d values", coverage.covered, sides);
        if (coverage.overlapping > 0)
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.3f, 1.f),
                               "%d values are in several rows (the first row wins).", coverage.overlapping);

        // --- запасной переход ---
        ImGui::Separator();
        label("If no row matches, go to");
        NodeId fallback = node.fallbackTarget();
        if (nodeCombo("##fallback", graph, fallback, node.id()))
            node.setFallbackTarget(fallback);
    }

    // ------------------------------------------------------------------ монстры

    void NodeInspector::drawEnemies(ScenarioGraph &graph, Node &node)
    {
        if (graph.enemies.empty())
        {
            ImGui::TextWrapped("The bestiary is empty. Add monsters on the Bestiary tab first.");
            return;
        }

        std::string detach;
        for (const std::string &enemyId : node.enemyIds)
        {
            ImGui::PushID(enemyId.c_str());
            if (const EnemyTemplate *enemy = graph.findEnemy(enemyId))
                ImGui::Text("%s  (AC %d, HP %d, init %+d)", enemy->name.c_str(), enemy->armorClass,
                            enemy->maxHp, enemy->initiativeBonus);
            else
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.f), "Unknown monster: %s", enemyId.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Detach"))
                detach = enemyId;
            ImGui::PopID();
        }
        if (!detach.empty())
            detachEnemy(graph, node.id(), detach);

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##attach", "Attach a monster..."))
        {
            for (const EnemyTemplate &enemy : graph.enemies)
            {
                const bool already = std::find(node.enemyIds.begin(), node.enemyIds.end(), enemy.id) !=
                                     node.enemyIds.end();
                if (already)
                    continue;
                ImGui::PushID(enemy.id.c_str());
                if (ImGui::Selectable(enemy.name.c_str()))
                    attachEnemy(graph, node.id(), enemy.id);
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    }

} // namespace dnd::gui::editor
