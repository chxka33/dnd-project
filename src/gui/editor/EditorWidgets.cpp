#include "gui/editor/EditorWidgets.hpp"

#include <algorithm>

namespace dnd::gui::editor
{
    using namespace dnd::scenario;

    namespace
    {
        int resizeCallback(ImGuiInputTextCallbackData *data)
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
            {
                auto *str = static_cast<std::string *>(data->UserData);
                str->resize(static_cast<std::size_t>(data->BufTextLen));
                data->Buf = str->data();
            }
            return 0;
        }
    } // namespace

    bool inputTextStd(const char *label, std::string &value, ImGuiInputTextFlags flags)
    {
        flags |= ImGuiInputTextFlags_CallbackResize;
        return ImGui::InputText(label, value.data(), value.capacity() + 1, flags, resizeCallback, &value);
    }

    bool inputMultilineStd(const char *label, std::string &value, int lines, ImGuiInputTextFlags flags)
    {
        flags |= ImGuiInputTextFlags_CallbackResize;
        const float height = ImGui::GetTextLineHeight() * static_cast<float>(lines) +
                             ImGui::GetStyle().FramePadding.y * 2.f;
        return ImGui::InputTextMultiline(label, value.data(), value.capacity() + 1,
                                         ImVec2(-FLT_MIN, height), flags, resizeCallback, &value);
    }

    bool intField(const char *label, int &value, int lo, int hi, int step)
    {
        bool changed = ImGui::InputInt(label, &value, step, step * 5);
        if (!ImGui::IsItemActive() && (value < lo || value > hi))
        {
            value = std::clamp(value, lo, hi);
            changed = true;
        }
        return changed;
    }

    bool nodeCombo(const char *label, const ScenarioGraph &graph, NodeId &target, const NodeId &self)
    {
        auto describe = [&](const NodeId &id)
        {
            const Node *node = graph.find(id);
            if (!node)
                return "(missing) " + id;
            return node->title.empty() ? id : node->title + "  [" + id + "]";
        };

        const std::string preview = target.empty() ? std::string("(none)") : describe(target);

        bool changed = false;
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo(label, preview.c_str()))
        {
            if (ImGui::Selectable("(none)", target.empty()))
            {
                target.clear();
                changed = true;
            }
            for (const NodeId &id : graph.order())
            {
                if (id == self)
                    continue;
                ImGui::PushID(id.c_str());
                if (ImGui::Selectable(describe(id).c_str(), id == target))
                {
                    target = id;
                    changed = true;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    const char *nodeTypeLabel(NodeType type)
    {
        switch (type)
        {
        case NodeType::Story:
            return "Story scene";
        case NodeType::Choice:
            return "Fork (choice)";
        case NodeType::Random:
            return "Random table";
        }
        return "";
    }

    ImU32 nodeTypeColor(NodeType type)
    {
        switch (type)
        {
        case NodeType::Story:
            return IM_COL32(70, 100, 140, 255);
        case NodeType::Choice:
            return IM_COL32(170, 120, 40, 255);
        case NodeType::Random:
            return IM_COL32(120, 70, 150, 255);
        }
        return IM_COL32(90, 90, 90, 255);
    }

    ImVec4 nodeTypeColorVec(NodeType type)
    {
        return ImGui::ColorConvertU32ToFloat4(nodeTypeColor(type));
    }

    std::string truncateUtf8(const std::string &text, std::size_t maxChars)
    {
        std::size_t chars = 0;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            // Байты-продолжения (10xxxxxx) не начинают нового символа.
            if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
            {
                if (chars == maxChars)
                    return text.substr(0, i) + "...";
                ++chars;
            }
        }
        return text;
    }

} // namespace dnd::gui::editor
