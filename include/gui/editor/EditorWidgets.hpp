#pragma once

#include "scenario/ScenarioGraph.hpp"

#include <imgui.h>

#include <string>

/// Общие виджеты редактора сценариев (модуль 2.2).
namespace dnd::gui::editor
{

    /// Однострочное поле для std::string произвольной длины (аналог imgui_stdlib).
    bool inputTextStd(const char *label, std::string &value, ImGuiInputTextFlags flags = 0);

    /// Многострочное поле для std::string произвольной длины.
    bool inputMultilineStd(const char *label, std::string &value, int lines, ImGuiInputTextFlags flags = 0);

    /// Целое поле: значение зажимается в [lo, hi], когда поле теряет фокус
    /// (пока Мастер печатает, число не «прыгает»).
    bool intField(const char *label, int &value, int lo, int hi, int step = 1);

    /// Выбор целевого узла из списка сцен сценария (пункт «(none)» — убрать связь).
    /// self исключается из списка (переходы «в себя» редактор не создаёт).
    bool nodeCombo(const char *label, const scenario::ScenarioGraph &graph,
                   scenario::NodeId &target, const scenario::NodeId &self);

    const char *nodeTypeLabel(scenario::NodeType type);
    ImU32 nodeTypeColor(scenario::NodeType type);
    ImVec4 nodeTypeColorVec(scenario::NodeType type);

    /// Обрезает UTF-8 строку до maxChars символов (а не байтов), добавляя "..." при обрезке.
    std::string truncateUtf8(const std::string &text, std::size_t maxChars);

} // namespace dnd::gui::editor
