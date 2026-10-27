#include "gui/Widgets.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

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
