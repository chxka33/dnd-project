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

    /// Цвет бейджа состояния: тяжёлые (без сознания, паралич...) — красный, прочие стандартные
    /// эффекты — оранжевый, собственные состояния Мастера — голубой.
    ImVec4 conditionColor(const std::string &condition);

    /// Цветные теги состояний персонажа в одну строку с переносом по ширине. Ничего не рисует,
    /// если состояний нет — вызывающий сам решает, что показать вместо этого.
    void conditionBadges(const entities::Character &c);

    /// Содержимое выпадающего окна «Состояния»: флажки для всех стандартных состояний D&D
    /// и для собственных состояний персонажа. Вызывать внутри BeginPopup. Возвращает true,
    /// если набор состояний изменился в этом кадре.
    bool conditionMenu(entities::Character &c);

    /// Экипированные доспех, щит и оружие через запятую ("Chain Mail, Shield, Longsword") или "-".
    std::string equipmentSummary(const entities::Character &c);

    /// Всплывающая подсказка со всем инвентарём и золотом. Вызывать после виджета, над которым
    /// навели мышь (внутри if (ImGui::IsItemHovered())).
    void inventoryTooltip(const entities::Character &c);

    /// Модальное окно подтверждения удаления. Вызывать каждый кадр в том же ID-контексте,
    /// где был OpenPopup(popupId). Возвращает true один раз — когда пользователь нажал "Remove".
    bool confirmRemovePopup(const char *popupId, const std::string &name);

} // namespace dnd::gui
