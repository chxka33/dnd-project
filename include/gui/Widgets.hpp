#pragma once

#include "entities/Character.hpp"

#include <imgui.h>

#include <string>

/// Небольшие общие виджеты ImGui для экранов персонажей.
namespace dnd::gui
{

    /// Поле ввода для std::string (stdlib-обёртка ImGui в проект не входит).
    bool inputText(const char *label, std::string &value, ImGuiInputTextFlags flags = 0);

    /// Цвет полоски здоровья: зелёный -> жёлтый -> красный.
    ImVec4 hpColor(float fraction);

    /// Полоска HP с подписью "12/27" (и "+5" для временных HP).
    void hpBar(const entities::Character &c, const ImVec2 &size = ImVec2(-FLT_MIN, 0.f));

    /// Подпись персонажа для списков: "Thorin - Fighter 3".
    std::string characterLabel(const entities::Character &c);

    /// Модальное окно подтверждения удаления. Вызывать каждый кадр в том же ID-контексте,
    /// где был OpenPopup(popupId). Возвращает true один раз — когда пользователь нажал "Remove".
    bool confirmRemovePopup(const char *popupId, const std::string &name);

} // namespace dnd::gui
