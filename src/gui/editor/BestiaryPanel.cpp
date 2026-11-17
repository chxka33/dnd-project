#include "gui/editor/BestiaryPanel.hpp"

#include "entities/AbilityScores.hpp"
#include "entities/CharacterClass.hpp"
#include "entities/ItemPresets.hpp"
#include "gui/Widgets.hpp"
#include "gui/editor/EditorOps.hpp"
#include "gui/editor/EditorWidgets.hpp"
#include "scenario/CharacterSerializer.hpp"

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>

namespace dnd::gui::editor
{
    using namespace dnd::scenario;

    namespace
    {
        const ImVec4 kGood(0.35f, 0.80f, 0.40f, 1.f);
        const ImVec4 kMuted(0.65f, 0.65f, 0.65f, 1.f);
        const char *kItemTypeNames[] = {"Misc", "Weapon", "Armor", "Shield", "Consumable"};
        const char *kArmorCategoryNames[] = {"Light", "Medium", "Heavy"};
        constexpr const char *kCharactersDir = "characters";
    } // namespace

    void BestiaryPanel::drawImportPopup(ScenarioGraph &graph, const entities::Roster *roster)
    {
        if (!ImGui::BeginPopup("import_from_characters"))
            return;

        ImGui::TextDisabled("Current session (Characters screen)");
        bool anySession = false;
        if (roster)
        {
            for (std::size_t i = 0; i < roster->size(); ++i)
            {
                const entities::Character &ch = roster->at(i).character;
                anySession = true;
                ImGui::PushID(static_cast<int>(i));
                const std::string label = gui::characterLabel(ch);
                if (ImGui::Selectable(label.c_str()))
                {
                    EnemyTemplate enemy;
                    enemy.applyCharacter(ch);
                    enemy.id = makeEnemyId(graph, enemy.name);
                    m_selected = enemy.id;
                    graph.addEnemy(std::move(enemy));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
            }
        }
        if (!anySession)
            ImGui::TextColored(kMuted, "No characters in current session.");

        namespace fs = std::filesystem;
        std::vector<fs::path> files;
        std::error_code ec;
        if (fs::is_directory(kCharactersDir, ec))
        {
            for (const auto &entry : fs::directory_iterator(kCharactersDir, ec))
                if (entry.is_regular_file() && entry.path().extension() == ".dndch")
                    files.push_back(entry.path());
            std::sort(files.begin(), files.end());
        }

        if (!files.empty())
        {
            ImGui::Separator();
            ImGui::TextDisabled("Saved in %s/", kCharactersDir);
            for (const auto &p : files)
            {
                ImGui::PushID(p.string().c_str());
                if (ImGui::BeginMenu(p.filename().string().c_str()))
                {
                    entities::Roster loaded;
                    if (CharacterSerializer::loadRosterFromFile(p, loaded))
                    {
                        for (std::size_t i = 0; i < loaded.size(); ++i)
                        {
                            const entities::Character &ch = loaded.at(i).character;
                            ImGui::PushID(static_cast<int>(i));
                            if (ImGui::MenuItem(gui::characterLabel(ch).c_str()))
                            {
                                EnemyTemplate enemy;
                                enemy.applyCharacter(ch);
                                enemy.id = makeEnemyId(graph, enemy.name);
                                m_selected = enemy.id;
                                graph.addEnemy(std::move(enemy));
                            }
                            ImGui::PopID();
                        }
                    }
                    else if (auto single = CharacterSerializer::loadFromFile(p))
                    {
                        if (ImGui::MenuItem(gui::characterLabel(*single).c_str()))
                        {
                            EnemyTemplate enemy;
                            enemy.applyCharacter(*single);
                            enemy.id = makeEnemyId(graph, enemy.name);
                            m_selected = enemy.id;
                            graph.addEnemy(std::move(enemy));
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::PopID();
            }
        }

        ImGui::EndPopup();
    }

    void BestiaryPanel::drawAbilities(EnemyTemplate &enemy)
    {
        if (!ImGui::BeginTable("enemy_abilities", static_cast<int>(entities::kAbilityCount),
                               ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
            return;

        for (Ability a : entities::kAllAbilities)
            ImGui::TableSetupColumn(entities::shortName(a));
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        for (std::size_t i = 0; i < entities::kAbilityCount; ++i)
        {
            ImGui::TableSetColumnIndex(static_cast<int>(i));
            ImGui::PushID(static_cast<int>(i));
            const Ability a = entities::kAllAbilities[i];
            int score = enemy.abilities.score(a);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (intField("##score", score, entities::AbilityScores::kMinScore, entities::AbilityScores::kMaxScore, 1))
            {
                enemy.abilities.setScore(a, score);
                enemy.recalculateStats();
            }
            ImGui::PopID();
        }

        const auto &saves = entities::classInfo(enemy.characterClass).savingThrows;
        const int prof = entities::Character::proficiencyBonusForLevel(enemy.level);

        ImGui::TableNextRow();
        for (std::size_t i = 0; i < entities::kAbilityCount; ++i)
        {
            ImGui::TableSetColumnIndex(static_cast<int>(i));
            ImGui::Text("Mod %+d", enemy.abilities.modifier(entities::kAllAbilities[i]));
        }

        ImGui::TableNextRow();
        for (std::size_t i = 0; i < entities::kAbilityCount; ++i)
        {
            ImGui::TableSetColumnIndex(static_cast<int>(i));
            const Ability a = entities::kAllAbilities[i];
            const bool proficient = (saves[0] == a || saves[1] == a);
            const int saveBonus = enemy.abilities.modifier(a) + (proficient ? prof : 0);
            ImGui::TextColored(proficient ? kGood : kMuted, "Save %+d%s", saveBonus, proficient ? " *" : "");
        }
        ImGui::EndTable();

        if (ImGui::Button("Standard array (15 14 13 12 10 8)"))
        {
            enemy.abilities = entities::AbilityScores(15, 14, 13, 12, 10, 8);
            enemy.recalculateStats();
        }
    }

    void BestiaryPanel::drawInventory(EnemyTemplate &enemy)
    {
        entities::Inventory &inv = enemy.inventory;

        int gold = inv.gold();
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.f);
        if (intField("Gold (gp)", gold, 0, 1000000, 1))
            inv.setGold(gold);

        const entities::Item *armor = inv.equippedArmor();
        const entities::Item *shield = inv.equippedShield();
        ImGui::TextColored(kMuted, "Armor: %s   Shield: %s",
                           armor ? armor->name.c_str() : "none",
                           shield ? shield->name.c_str() : "none");

        if (!inv.empty())
        {
            enum class Action
            {
                None,
                Equip,
                Unequip,
                DropOne,
                DropAll
            };
            Action action = Action::None;
            std::size_t actionIndex = 0;

            for (std::size_t i = 0; i < inv.size(); ++i)
            {
                const entities::Item &item = inv.items()[i];
                ImGui::PushID(static_cast<int>(i));

                if (item.equipped)
                    ImGui::TextColored(kGood, "%s (equipped)", item.name.c_str());
                else if (item.quantity > 1)
                    ImGui::Text("%s x%d", item.name.c_str(), item.quantity);
                else
                    ImGui::TextUnformatted(item.name.c_str());

                const std::string details = entities::describeItem(item);
                if (!details.empty())
                {
                    ImGui::SameLine();
                    ImGui::TextColored(kMuted, "(%s)", details.c_str());
                }

                if (item.isEquippable())
                {
                    ImGui::SameLine();
                    if (ImGui::SmallButton(item.equipped ? "Unequip" : "Equip"))
                    {
                        action = item.equipped ? Action::Unequip : Action::Equip;
                        actionIndex = i;
                    }
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Drop 1"))
                {
                    action = Action::DropOne;
                    actionIndex = i;
                }
                if (item.quantity > 1)
                {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Drop all"))
                    {
                        action = Action::DropAll;
                        actionIndex = i;
                    }
                }
                ImGui::PopID();
            }

            switch (action)
            {
            case Action::Equip:
                inv.equip(actionIndex);
                enemy.recalculateStats();
                break;
            case Action::Unequip:
                inv.unequip(actionIndex);
                enemy.recalculateStats();
                break;
            case Action::DropOne:
                inv.remove(actionIndex, 1);
                enemy.recalculateStats();
                break;
            case Action::DropAll:
                inv.remove(actionIndex, inv.items()[actionIndex].quantity);
                enemy.recalculateStats();
                break;
            case Action::None:
                break;
            }
        }
        else
        {
            ImGui::TextColored(kMuted, "Inventory is empty.");
        }

        drawAddItem(enemy);
    }

    void BestiaryPanel::drawAddItem(EnemyTemplate &enemy)
    {
        if (!ImGui::TreeNode("Add item to monster"))
            return;

        const std::vector<entities::Item> &presets = entities::itemPresets();
        if (!presets.empty())
        {
            m_presetIndex = std::clamp(m_presetIndex, 0, static_cast<int>(presets.size()) - 1);
            const entities::Item &selectedPreset = presets[static_cast<std::size_t>(m_presetIndex)];
            if (ImGui::BeginCombo("Catalog", selectedPreset.name.c_str()))
            {
                for (int i = 0; i < static_cast<int>(presets.size()); ++i)
                {
                    const entities::Item &p = presets[static_cast<std::size_t>(i)];
                    const std::string desc = entities::describeItem(p);
                    const std::string entryLabel = desc.empty() ? p.name : p.name + "  (" + desc + ")";
                    ImGui::PushID(i);
                    if (ImGui::Selectable(entryLabel.c_str(), i == m_presetIndex))
                        m_presetIndex = i;
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            intField("Quantity##preset", m_presetQuantity, 1, 999, 1);
            if (ImGui::Button("Add from catalog"))
            {
                entities::Item item = selectedPreset;
                item.quantity = m_presetQuantity;
                const std::size_t idx = enemy.inventory.add(std::move(item));
                if (selectedPreset.isEquippable() && idx < enemy.inventory.size())
                {
                    enemy.inventory.equip(idx);
                    enemy.recalculateStats();
                }
            }
        }

        ImGui::SeparatorText("Custom item");
        inputTextStd("Name##custom", m_customName);
        ImGui::Combo("Type##custom", &m_customType, kItemTypeNames, 5);
        intField("Quantity##custom", m_customQuantity, 1, 999, 1);
        ImGui::InputFloat("Weight (lb, each)", &m_customWeight, 0.f, 0.f, "%.1f");
        m_customWeight = std::max(0.f, m_customWeight);

        const entities::ItemType type = static_cast<entities::ItemType>(m_customType);
        switch (type)
        {
        case entities::ItemType::Weapon:
            inputTextStd("Damage", m_customDamage);
            inputTextStd("Damage type", m_customDamageType);
            ImGui::Checkbox("Finesse", &m_customFinesse);
            ImGui::SameLine();
            ImGui::Checkbox("Ranged", &m_customRanged);
            break;
        case entities::ItemType::Armor:
            ImGui::Combo("Armor category", &m_customArmorCategory, kArmorCategoryNames, 3);
            intField("Base AC", m_customBaseAc, 0, 30, 1);
            intField("Strength requirement", m_customStrReq, 0, 30, 1);
            ImGui::Checkbox("Stealth disadvantage", &m_customStealth);
            break;
        case entities::ItemType::Shield:
            intField("AC bonus", m_customShieldBonus, 0, 10, 1);
            break;
        case entities::ItemType::Consumable:
        case entities::ItemType::Misc:
            inputTextStd("Description", m_customDescription);
            break;
        }

        ImGui::BeginDisabled(m_customName.empty());
        if (ImGui::Button("Add custom item"))
        {
            entities::Item item;
            item.name = m_customName;
            item.type = type;
            item.weight = m_customWeight;
            item.quantity = m_customQuantity;
            switch (type)
            {
            case entities::ItemType::Weapon:
                item.weapon = entities::WeaponStats{m_customDamage, m_customDamageType,
                                                    m_customFinesse, m_customRanged};
                break;
            case entities::ItemType::Armor:
                item.armor = entities::ArmorStats{static_cast<entities::ArmorCategory>(m_customArmorCategory),
                                                  m_customBaseAc, m_customStrReq, m_customStealth};
                break;
            case entities::ItemType::Shield:
                item.shieldBonus = m_customShieldBonus;
                break;
            case entities::ItemType::Consumable:
            case entities::ItemType::Misc:
                item.description = m_customDescription;
                break;
            }
            entities::registerItemPreset(item);
            const bool equippable = item.isEquippable();
            const std::size_t idx = enemy.inventory.add(std::move(item));
            if (equippable && idx < enemy.inventory.size())
            {
                enemy.inventory.equip(idx);
                enemy.recalculateStats();
            }
            m_customName.clear();
        }
        ImGui::EndDisabled();

        ImGui::TreePop();
    }

    std::optional<NodeId> BestiaryPanel::draw(ScenarioGraph &graph,
                                              const NodeId &currentScene,
                                              const entities::Roster *roster)
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
        if (ImGui::Button("From Characters..."))
            ImGui::OpenPopup("import_from_characters");
        drawImportPopup(graph, roster);
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
            const std::string text = enemy.name + " - " +
                                     entities::classInfo(enemy.characterClass).displayName + " " +
                                     std::to_string(enemy.level) + "   AC " +
                                     std::to_string(enemy.armorClass) + "  HP " + std::to_string(enemy.maxHp);
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

        // --- класс и уровень (синхронизировано с Characters) ---
        const entities::ClassInfo &clsInfo = entities::classInfo(enemy->characterClass);
        ImGui::TextUnformatted("Class");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo("##class", clsInfo.displayName))
        {
            for (entities::CharacterClass cls : entities::kAllClasses)
            {
                const bool selected = (cls == enemy->characterClass);
                if (ImGui::Selectable(entities::classInfo(cls).displayName, selected))
                {
                    enemy->characterClass = cls;
                    enemy->recalculateStats();
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::TextUnformatted("Level");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.f);
        if (intField("##level", enemy->level, entities::Character::kMinLevel, entities::Character::kMaxLevel))
            enemy->recalculateStats();
        ImGui::SameLine();
        if (ImGui::Button("Level up") && enemy->level < entities::Character::kMaxLevel)
        {
            ++enemy->level;
            enemy->recalculateStats();
        }
        ImGui::TextColored(kMuted, "Hit die: d%d   Proficiency: %+d",
                           entities::classInfo(enemy->characterClass).hitDie,
                           entities::Character::proficiencyBonusForLevel(enemy->level));

        // --- боевые показатели ---
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

        if (ImGui::SmallButton("Recalculate AC / HP / Init from class & gear"))
            enemy->recalculateStats();

        if (ImGui::CollapsingHeader("Abilities"))
            drawAbilities(*enemy);

        if (ImGui::CollapsingHeader("Inventory & Equipment"))
            drawInventory(*enemy);

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
