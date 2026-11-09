#pragma once

#include <array>
#include <string>

namespace dnd::entities
{

    /// Стандартные состояния D&D 5e (SRD). Названия латиницей — шрифт ImGui по умолчанию без кириллицы,
    /// они же сохраняются в .dndch / .dndsav. Персонаж хранит состояния как строки,
    /// поэтому Мастер по-прежнему может добавить собственный эффект (например, "Blessed").
    inline constexpr std::size_t kStandardConditionCount = 15;

    inline constexpr std::array<const char *, kStandardConditionCount> kStandardConditions = {
        "Blinded", "Charmed", "Deafened", "Exhaustion", "Frightened",
        "Grappled", "Incapacitated", "Invisible", "Paralyzed", "Petrified",
        "Poisoned", "Prone", "Restrained", "Stunned", "Unconscious"};

    /// true, если название совпадает со стандартным состоянием (регистр важен).
    bool isStandardCondition(const std::string &name);

} // namespace dnd::entities
