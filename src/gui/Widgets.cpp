#include "gui/Widgets.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace dnd::gui
{

    bool inputText(const char *label, std::string &value, ImGuiInputTextFlags flags)
    {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
        if (ImGui::InputText(label, buffer, sizeof(buffer), flags))
        {
            value = buffer;
            return true;
        }
        return false;
    }

    ImVec4 hpColor(float fraction)
    {
        if (fraction > 0.5f)
            return ImVec4(0.25f, 0.70f, 0.30f, 1.f);
        if (fraction > 0.25f)
            return ImVec4(0.85f, 0.70f, 0.15f, 1.f);
        return ImVec4(0.80f, 0.20f, 0.20f, 1.f);
    }

    void hpBar(const entities::Character &c, const ImVec2 &size)
    {
        const int maxHp = std::max(1, c.maxHp());
        const float fraction = std::clamp(static_cast<float>(c.currentHp()) / static_cast<float>(maxHp), 0.f, 1.f);

        char overlay[48];
        if (c.temporaryHp() > 0)
            std::snprintf(overlay, sizeof(overlay), "%d/%d (+%d)", c.currentHp(), c.maxHp(), c.temporaryHp());
        else
            std::snprintf(overlay, sizeof(overlay), "%d/%d", c.currentHp(), c.maxHp());

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor(fraction));
        ImGui::ProgressBar(fraction, size, overlay);
        ImGui::PopStyleColor();
    }

    std::string characterLabel(const entities::Character &c)
    {
        return c.name + " - " + c.classData().displayName + " " + std::to_string(c.level());
    }

    ImVec4 conditionColor(const std::string &condition)
    {
        static const char *const severe[] = {"Unconscious", "Paralyzed", "Petrified", "Stunned", "Incapacitated"};
        for (const char *name : severe)
            if (condition == name)
                return ImVec4(0.90f, 0.30f, 0.30f, 1.f);
        if (entities::isStandardCondition(condition))
            return ImVec4(0.95f, 0.65f, 0.25f, 1.f);
        return ImVec4(0.45f, 0.75f, 0.95f, 1.f);
    }

    void conditionBadges(const entities::Character &c)
    {
        bool first = true;
        for (const std::string &condition : c.conditions())
        {
            const std::string tag = "[" + condition + "]";
            if (!first)
            {
                // Следующий тег не помещается в строку — переносим на новую.
                const float needed = ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(tag.c_str()).x;
                if (needed <= ImGui::GetContentRegionAvail().x)
                    ImGui::SameLine();
            }
            ImGui::TextColored(conditionColor(condition), "%s", tag.c_str());
            first = false;
        }
    }

    bool conditionMenu(entities::Character &c)
    {
        bool changed = false;
        ImGui::TextDisabled("Conditions");
        ImGui::Separator();

        for (const char *name : entities::kStandardConditions)
        {
            bool active = c.hasCondition(name);
            if (ImGui::Checkbox(name, &active))
            {
                c.toggleCondition(name);
                changed = true;
            }
        }

        // Собственные состояния Мастера (не из каталога): показываем, чтобы их можно было снять.
        std::vector<std::string> custom;
        for (const std::string &condition : c.conditions())
            if (!entities::isStandardCondition(condition))
                custom.push_back(condition);
        if (!custom.empty())
        {
            ImGui::Separator();
            ImGui::TextDisabled("Custom");
            for (const std::string &name : custom)
            {
                bool active = true;
                if (ImGui::Checkbox(name.c_str(), &active))
                {
                    c.removeCondition(name);
                    changed = true;
                }
            }
        }

        if (!c.conditions().empty())
        {
            ImGui::Separator();
            if (ImGui::SmallButton("Clear all"))
            {
                c.clearConditions();
                changed = true;
            }
        }
        return changed;
    }

    std::string equipmentSummary(const entities::Character &c)
    {
        std::string text;
        for (const entities::Item &item : c.inventory().items())
        {
            if (!item.equipped)
                continue;
            if (!text.empty())
                text += ", ";
            text += item.name;
        }
        return text.empty() ? "-" : text;
    }

    void inventoryTooltip(const entities::Character &c)
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s - %d gp", c.name.c_str(), c.inventory().gold());
        if (c.inventory().empty())
        {
            ImGui::TextDisabled("Inventory is empty.");
        }
        else
        {
            ImGui::Separator();
            for (const entities::Item &item : c.inventory().items())
            {
                if (item.quantity > 1)
                    ImGui::Text("%s x%d%s", item.name.c_str(), item.quantity, item.equipped ? " (equipped)" : "");
                else
                    ImGui::Text("%s%s", item.name.c_str(), item.equipped ? " (equipped)" : "");
            }
        }
        ImGui::EndTooltip();
    }

    bool confirmRemovePopup(const char *popupId, const std::string &name)
    {
        bool confirmed = false;
        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(popupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Remove \"%s\" from the roster?", name.c_str());
            ImGui::Separator();
            if (ImGui::Button("Remove"))
            {
                confirmed = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        return confirmed;
    }

} // namespace dnd::gui
