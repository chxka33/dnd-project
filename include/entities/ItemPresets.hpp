#pragma once

#include "entities/Item.hpp"

#include <string>
#include <vector>

namespace dnd::entities
{
    class Inventory;
    class Roster;

    /// Каталог предметов (базовые SRD 5e + собственные предметы, созданные на экране
    /// Characters или в Scenario Editor). Общий для всех экранов приложения.
    const std::vector<Item> &itemPresets();

    /// Регистрирует пользовательский предмет в общем каталоге (quantity приводится к 1,
    /// equipped — к false). Если предмет с таким названием уже есть, обновляет его параметры.
    /// Пустое название игнорируется.
    void registerItemPreset(Item item);

    /// Регистрирует в общем каталоге все предметы из инвентаря персонажа или группы (Roster).
    void registerInventoryItems(const Inventory &inventory);
    void registerRosterItems(const Roster &roster);

    /// Краткое описание параметров предмета для таблиц и списков GUI:
    /// урон и свойства оружия, КД и категория доспеха, бонус щита или описание.
    std::string describeItem(const Item &item);

    /// Предмет по названию: если оно есть в каталоге — копия с параметрами (доспех с КД, оружие
    /// с уроном), иначе обычный предмет Misc. Нужен для наград сценария, где указано только название.
    Item itemFromName(const std::string &name, int quantity = 1);

} // namespace dnd::entities
