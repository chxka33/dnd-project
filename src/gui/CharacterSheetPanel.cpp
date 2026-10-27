#include "gui/CharacterSheetPanel.hpp"

#include "entities/ItemPresets.hpp"
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

        std::string describe(const Item &item)
        {
            char buf[160];
            switch (item.type)
            {
            case ItemType::Weapon:
                if (!item.weapon)
                    return "";
                std::snprintf(buf, sizeof(buf), "%s %s%s%s", item.weapon->damage.c_str(),
                              item.weapon->damageType.c_str(),
                              item.weapon->finesse ? ", finesse" : "",
                              item.weapon->ranged ? ", ranged" : "");
                return buf;
            case ItemType::Armor:
                if (!item.armor)
                    return "(no armor stats)";
                {
                    std::string text = "AC " + std::to_string(item.armor->baseAc) + " (" +
                                       toString(item.armor->category) + ")";
                    if (item.armor->strengthRequirement > 0)
                        text += ", needs STR " + std::to_string(item.armor->strengthRequirement);
                    if (item.armor->stealthDisadvantage)
                        text += ", stealth disadv.";
                    return text;
                }
            case ItemType::Shield:
                std::snprintf(buf, sizeof(buf), "+%d AC", item.shieldBonus);
                return buf;
            case ItemType::Consumable:
            case ItemType::Misc:
                return item.description;
            }
            return "";
        }

        const char *kItemTypeNames[] = {"Misc", "Weapon", "Armor", "Shield", "Consumable"};
        const char *kArmorCategoryNames[] = {"Light", "Medium", "Heavy"};
    } // namespace

    void CharacterSheetPanel::draw(Roster &roster)
    {
        if (roster.empty())
        {
            ImGui::TextWrapped("The roster is empty. Use \"Add character\" or \"Add demo party\" above.");
            return;
        }

        m_selected = std::clamp(m_selected, 0, static_cast<int>(roster.size()) - 1);
        if (m_selected != m_lastSelected)
        {
            m_status.clear();
            m_lastSelected = m_selected;
        }

        // Выбор персонажа (в режиме Setup выбор делается списком на экране Characters)
        if (m_mode == Mode::Play)
        {
            const RosterEntry &current = roster.at(static_cast<std::size_t>(m_selected));
            const std::string preview = std::string("[") + toString(current.side) + "] " +
                                        characterLabel(current.character);
            if (ImGui::BeginCombo("Character", preview.c_str()))
            {
                for (int i = 0; i < static_cast<int>(roster.size()); ++i)
                {
                    const RosterEntry &e = roster.at(static_cast<std::size_t>(i));
                    const std::string label = std::string("[") + toString(e.side) + "] " +
                                              characterLabel(e.character);
                    ImGui::PushID(i);
                    if (ImGui::Selectable(label.c_str(), i == m_selected))
                        m_selected = i;
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
        }

        Character &c = roster.at(static_cast<std::size_t>(m_selected)).character;
        ImGui::PushID(m_selected);
        const int maxHpBefore = c.maxHp();

        drawHeader(roster, c);
        ImGui::SeparatorText("Abilities");
        drawAbilities(c);
        ImGui::SeparatorText("Combat");
        drawCombatStats(c);
        ImGui::SeparatorText("Health");
        if (m_mode == Mode::Play)
            drawHealth(c);
        else
            ImGui::Text("Max HP: %d  (d%d per level, CON %+d)", c.maxHp(), c.hitDieSize(),
                        c.modifier(Ability::Constitution));
        if (c.isSpellcaster())
        {
            ImGui::SeparatorText("Spellcasting");
            drawSpells(c);
        }
        ImGui::SeparatorText("Inventory");
        drawInventory(c);
        drawAddItem(c);

        ImGui::PopID();

        // Подготовка к игре: персонаж остаётся полностью здоровым, если максимум HP вырос.
        if (m_mode == Mode::Setup && c.maxHp() > maxHpBefore)
            c.heal(c.maxHp() - maxHpBefore);

        // Удаление — в самом конце: после него ссылка c недействительна.
        if (m_removeRequested)
        {
            m_removeRequested = false;
            roster.remove(static_cast<std::size_t>(m_selected));
            m_lastSelected = -1;
        }
    }

    void CharacterSheetPanel::drawHeader(Roster &roster, Character &c)
    {
        RosterEntry &entry = roster.at(static_cast<std::size_t>(m_selected));

        inputText("Name", c.name);

        int level = c.level();
        if (ImGui::InputInt("Level", &level))
            c.setLevel(level);

        ImGui::Text("Class: %s   Hit die: d%d   Side:", c.classData().displayName, c.hitDieSize());
        ImGui::SameLine();
        int side = entry.side == Side::Player ? 0 : 1;
        const char *sides[] = {"Player", "Enemy"};
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 7.f);
        if (ImGui::Combo("##side", &side, sides, 2))
            entry.side = side == 0 ? Side::Player : Side::Enemy;

        if (ImGui::Button("Level up"))
            c.levelUp();
        ImGui::SameLine();
        if (ImGui::Button("Remove character"))
            ImGui::OpenPopup("Remove character?");
        if (confirmRemovePopup("Remove character?", c.name))
            m_removeRequested = true;
    }

    void CharacterSheetPanel::drawAbilities(Character &c)
    {
        if (!ImGui::BeginTable("abilities", static_cast<int>(kAbilityCount),
                               ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
            return;

        for (Ability a : kAllAbilities)
            ImGui::TableSetupColumn(shortName(a));
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        for (std::size_t i = 0; i < kAbilityCount; ++i)
        {
            ImGui::TableSetColumnIndex(static_cast<int>(i));
            ImGui::PushID(static_cast<int>(i));
            int score = c.abilities().score(kAllAbilities[i]);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##score", &score))
                c.abilities().setScore(kAllAbilities[i], score);
            ImGui::PopID();
        }

        ImGui::TableNextRow();
        for (std::size_t i = 0; i < kAbilityCount; ++i)
        {
            ImGui::TableSetColumnIndex(static_cast<int>(i));
            ImGui::Text("Mod %+d", c.modifier(kAllAbilities[i]));
        }

        ImGui::TableNextRow();
        for (std::size_t i = 0; i < kAbilityCount; ++i)
        {
            ImGui::TableSetColumnIndex(static_cast<int>(i));
            const Ability a = kAllAbilities[i];
            const bool proficient = c.isProficientInSave(a);
            ImGui::TextColored(proficient ? kGood : kMuted, "Save %+d%s", c.savingThrowBonus(a),
                               proficient ? " *" : "");
        }
        ImGui::EndTable();
        ImGui::TextColored(kMuted, "* proficient saving throw (from class)");
    }

    void CharacterSheetPanel::drawCombatStats(Character &c)
    {
        if (ImGui::BeginTable("combat", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("Armor Class");
            ImGui::TableSetupColumn("Initiative");
            ImGui::TableSetupColumn("Proficiency");
            ImGui::TableSetupColumn("Hit die");
            ImGui::TableSetupColumn("Carrying (lb)");
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", c.armorClass());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%+d", c.initiativeModifier());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%+d", c.proficiencyBonus());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%dd%d", c.level(), c.hitDieSize());
            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(c.isEncumbered() ? kBad : ImGui::GetStyleColorVec4(ImGuiCol_Text),
                               "%.1f / %.0f", c.inventory().totalWeight(), c.carryCapacity());
            ImGui::EndTable();
        }

        const Item *armor = c.inventory().equippedArmor();
        const Item *shield = c.inventory().equippedShield();
        ImGui::TextColored(kMuted, "Armor: %s   Shield: %s", armor ? armor->name.c_str() : "none",
                           shield ? shield->name.c_str() : "none");

        int bonus = c.miscArmorClassBonus();
        if (ImGui::InputInt("Misc AC bonus", &bonus))
            c.setMiscArmorClassBonus(std::clamp(bonus, -10, 10));
    }

    void CharacterSheetPanel::drawHealth(Character &c)
    {
        hpBar(c, ImVec2(-FLT_MIN, ImGui::GetFrameHeight() * 1.4f));
        if (c.isDown())
            ImGui::TextColored(kBad, "DOWN - 0 HP");
        else
            ImGui::Text("HP %d / %d   Temp HP %d", c.currentHp(), c.maxHp(), c.temporaryHp());

        int hp = c.currentHp();
        if (ImGui::InputInt("Set current HP", &hp))
            c.setCurrentHp(hp);

        ImGui::InputInt("Amount", &m_amount);
        m_amount = std::max(0, m_amount);

        char msg[96];
        if (ImGui::Button("Damage"))
        {
            const int tempBefore = c.temporaryHp();
            const int lost = c.applyDamage(m_amount);
            const int absorbed = tempBefore - c.temporaryHp();
            if (absorbed > 0)
                std::snprintf(msg, sizeof(msg), "Took %d damage (%d absorbed by temp HP).", lost + absorbed, absorbed);
            else
                std::snprintf(msg, sizeof(msg), "Took %d damage.", lost);
            m_status = msg;
        }
        ImGui::SameLine();
        if (ImGui::Button("Heal"))
        {
            std::snprintf(msg, sizeof(msg), "Healed %d HP.", c.heal(m_amount));
            m_status = msg;
        }
        ImGui::SameLine();
        if (ImGui::Button("Grant temp HP"))
        {
            c.grantTemporaryHp(m_amount);
            std::snprintf(msg, sizeof(msg), "Temp HP is now %d.", c.temporaryHp());
            m_status = msg;
        }

        if (ImGui::Button("Long rest"))
        {
            c.longRest();
            m_status = "Long rest: HP and spell slots restored.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Short rest"))
        {
            c.shortRest();
            m_status = c.classData().caster == CasterType::Pact ? "Short rest: pact slots restored."
                                                                : "Short rest.";
        }
        if (!m_status.empty())
            ImGui::TextColored(kMuted, "%s", m_status.c_str());
    }

    void CharacterSheetPanel::drawSpells(Character &c)
    {
        const ClassInfo &info = c.classData();
        const Ability key = info.spellcastingAbility.value_or(Ability::Intelligence);
        ImGui::Text("Spellcasting ability: %s   Save DC: %d   Attack bonus: %+d", shortName(key),
                    c.spellSaveDc().value_or(0), c.spellAttackBonus().value_or(0));

        SpellSlots &slots = c.spellSlots();
        if (!slots.hasAny())
        {
            ImGui::TextColored(kMuted, "No spell slots at this level.");
            return;
        }
        if (info.caster == CasterType::Pact)
            ImGui::TextColored(kMuted, "Pact magic: slots return on a short rest.");

        const bool play = m_mode == Mode::Play;
        if (!ImGui::BeginTable("slots", play ? 3 : 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp))
            return;
        ImGui::TableSetupColumn("Spell level");
        ImGui::TableSetupColumn(play ? "Slots" : "Slots (max)");
        if (play)
            ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();
        for (int level = 1; level <= SpellSlots::kMaxSpellLevel; ++level)
        {
            if (slots.max(level) == 0)
                continue;
            ImGui::PushID(level);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Level %d", level);
            ImGui::TableSetColumnIndex(1);
            if (!play)
            {
                ImGui::Text("%d", slots.max(level));
                ImGui::PopID();
                continue;
            }
            ImGui::Text("%d / %d", slots.current(level), slots.max(level));
            ImGui::TableSetColumnIndex(2);
            ImGui::BeginDisabled(!slots.hasSlot(level));
            if (ImGui::SmallButton("Use"))
                slots.use(level);
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(slots.current(level) >= slots.max(level));
            if (ImGui::SmallButton("Restore"))
                slots.restore(level);
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    void CharacterSheetPanel::drawInventory(Character &c)
    {
        Inventory &inv = c.inventory();
        if (inv.empty())
        {
            ImGui::TextColored(kMuted, "Inventory is empty.");
            return;
        }

        enum class Action
        {
            None,
            Equip,
            Unequip,
            Use,
            DropOne,
            DropAll
        };
        Action action = Action::None;
        std::size_t actionIndex = 0;

        if (ImGui::BeginTable("inventory", 6,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 2.f);
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Qty");
            ImGui::TableSetupColumn("Weight");
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 3.f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch, 3.f);
            ImGui::TableHeadersRow();

            for (std::size_t i = 0; i < inv.size(); ++i)
            {
                const Item &item = inv.items()[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                if (item.equipped)
                    ImGui::TextColored(kGood, "%s (equipped)", item.name.c_str());
                else
                    ImGui::TextUnformatted(item.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(kItemTypeNames[static_cast<int>(item.type)]);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", item.quantity);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.1f", item.totalWeight());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextWrapped("%s", describe(item).c_str());

                ImGui::TableSetColumnIndex(5);
                if (item.isEquippable())
                {
                    if (ImGui::SmallButton(item.equipped ? "Unequip" : "Equip"))
                    {
                        action = item.equipped ? Action::Unequip : Action::Equip;
                        actionIndex = i;
                    }
                    ImGui::SameLine();
                }
                if (m_mode == Mode::Play && item.type == ItemType::Consumable)
                {
                    if (ImGui::SmallButton("Use"))
                    {
                        action = Action::Use;
                        actionIndex = i;
                    }
                    ImGui::SameLine();
                }
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
            ImGui::EndTable();
        }

        // Изменения применяются после таблицы, чтобы не менять вектор во время обхода.
        switch (action)
        {
        case Action::Equip:   inv.equip(actionIndex); break;
        case Action::Unequip: inv.unequip(actionIndex); break;
        case Action::Use:     inv.consume(actionIndex); break;
        case Action::DropOne: inv.remove(actionIndex, 1); break;
        case Action::DropAll: inv.remove(actionIndex, inv.items()[actionIndex].quantity); break;
        case Action::None:    break;
        }
    }

    void CharacterSheetPanel::drawAddItem(Character &c)
    {
        if (!ImGui::CollapsingHeader("Add item"))
            return;

        // --- Из каталога ---
        const std::vector<Item> &presets = itemPresets();
        m_presetIndex = std::clamp(m_presetIndex, 0, static_cast<int>(presets.size()) - 1);
        if (ImGui::BeginCombo("Catalog", presets[static_cast<std::size_t>(m_presetIndex)].name.c_str()))
        {
            for (int i = 0; i < static_cast<int>(presets.size()); ++i)
            {
                ImGui::PushID(i);
                if (ImGui::Selectable(presets[static_cast<std::size_t>(i)].name.c_str(), i == m_presetIndex))
                    m_presetIndex = i;
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::InputInt("Quantity##preset", &m_presetQuantity);
        m_presetQuantity = std::clamp(m_presetQuantity, 1, 999);
        if (ImGui::Button("Add from catalog"))
        {
            Item item = presets[static_cast<std::size_t>(m_presetIndex)];
            item.quantity = m_presetQuantity;
            c.inventory().add(std::move(item));
        }

        // --- Свой предмет ---
        ImGui::SeparatorText("Custom item");
        inputText("Name##custom", m_customName);
        ImGui::Combo("Type##custom", &m_customType, kItemTypeNames, 5);
        ImGui::InputInt("Quantity##custom", &m_customQuantity);
        m_customQuantity = std::clamp(m_customQuantity, 1, 999);
        ImGui::InputFloat("Weight (lb, each)", &m_customWeight, 0.f, 0.f, "%.1f");
        m_customWeight = std::max(0.f, m_customWeight);

        const ItemType type = static_cast<ItemType>(m_customType);
        switch (type)
        {
        case ItemType::Weapon:
            inputText("Damage", m_customDamage);
            inputText("Damage type", m_customDamageType);
            ImGui::Checkbox("Finesse", &m_customFinesse);
            ImGui::SameLine();
            ImGui::Checkbox("Ranged", &m_customRanged);
            break;
        case ItemType::Armor:
            ImGui::Combo("Armor category", &m_customArmorCategory, kArmorCategoryNames, 3);
            ImGui::InputInt("Base AC", &m_customBaseAc);
            m_customBaseAc = std::clamp(m_customBaseAc, 0, 30);
            ImGui::InputInt("Strength requirement", &m_customStrReq);
            m_customStrReq = std::clamp(m_customStrReq, 0, 30);
            ImGui::Checkbox("Stealth disadvantage", &m_customStealth);
            break;
        case ItemType::Shield:
            ImGui::InputInt("AC bonus", &m_customShieldBonus);
            m_customShieldBonus = std::clamp(m_customShieldBonus, 0, 10);
            break;
        case ItemType::Consumable:
        case ItemType::Misc:
            inputText("Description", m_customDescription);
            break;
        }

        ImGui::BeginDisabled(m_customName.empty());
        if (ImGui::Button("Add custom item"))
        {
            Item item;
            item.name = m_customName;
            item.type = type;
            item.weight = m_customWeight;
            item.quantity = m_customQuantity;
            switch (type)
            {
            case ItemType::Weapon:
                item.weapon = WeaponStats{m_customDamage, m_customDamageType, m_customFinesse, m_customRanged};
                break;
            case ItemType::Armor:
                item.armor = ArmorStats{static_cast<ArmorCategory>(m_customArmorCategory), m_customBaseAc,
                                        m_customStrReq, m_customStealth};
                break;
            case ItemType::Shield:
                item.shieldBonus = m_customShieldBonus;
                break;
            case ItemType::Consumable:
            case ItemType::Misc:
                item.description = m_customDescription;
                break;
            }
            c.inventory().add(std::move(item));
        }
        ImGui::EndDisabled();
    }

} // namespace dnd::gui
