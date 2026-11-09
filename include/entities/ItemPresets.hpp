#pragma once

#include "entities/Item.hpp"

#include <vector>

namespace dnd::entities
{

    /// Каталог типовых предметов (SRD 5e) для быстрого добавления в инвентарь из GUI.
    /// Список создаётся один раз и не меняется.
    const std::vector<Item> &itemPresets();

    /// Предмет по названию: если оно есть в каталоге — копия с параметрами (доспех с КД, оружие
    /// с уроном), иначе обычный предмет Misc. Нужен для наград сценария, где указано только название.
    Item itemFromName(const std::string &name, int quantity = 1);

} // namespace dnd::entities
