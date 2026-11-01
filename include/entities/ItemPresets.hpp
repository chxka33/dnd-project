#pragma once

#include "entities/Item.hpp"

#include <vector>

namespace dnd::entities
{

    /// Каталог типовых предметов (SRD 5e) для быстрого добавления в инвентарь из GUI.
    /// Список создаётся один раз и не меняется.
    const std::vector<Item> &itemPresets();

} // namespace dnd::entities
