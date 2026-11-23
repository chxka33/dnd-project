#pragma once

#include <SFML/System/Vector2.hpp>

namespace dnd::core
{

    /// Базовая высота окна, под которую рассчитан интерфейс.
    inline constexpr float kUiReferenceHeight = 1080.f;

    /// Масштаб интерфейса при kUiReferenceHeight (обычный экран 1920x1080).
    inline constexpr float kUiReferenceScale = 1.5f;

    /// Границы автоматического масштаба: не даём интерфейсу стать нечитаемым
    /// на маленьком окне или чрезмерно крупным на очень высоком разрешении.
    inline constexpr float kUiMinScale = 0.75f;
    inline constexpr float kUiMaxScale = 4.f;

    /// Масштаб, который подбирается автоматически по высоте окна.
    /// При изменении размера окна интерфейс пересчитывается в drawUi().
    float automaticUiScale(const sf::Vector2u &windowSize);

    /// Применяет масштаб к ImGui. overrideScale <= 0 включает автоматический режим.
    /// Возвращает фактически применённый масштаб.
    float applyUiScale(const sf::Vector2u &windowSize, float overrideScale = 0.f);

} // namespace dnd::core
