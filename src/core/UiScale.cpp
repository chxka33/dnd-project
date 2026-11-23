#include "core/UiScale.hpp"

#include <imgui.h>

#include <algorithm>

namespace dnd::core
{

    float automaticUiScale(const sf::Vector2u &windowSize)
    {
        if (windowSize.y == 0)
            return kUiReferenceScale;

        const float relativeHeight = static_cast<float>(windowSize.y) / kUiReferenceHeight;
        return std::clamp(kUiReferenceScale * relativeHeight, kUiMinScale, kUiMaxScale);
    }

    float applyUiScale(const sf::Vector2u &windowSize, float overrideScale)
    {
        const float scale = overrideScale > 0.f ? overrideScale : automaticUiScale(windowSize);
        ImGui::GetIO().FontGlobalScale = scale;
        return scale;
    }

} // namespace dnd::core
