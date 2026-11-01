#pragma once

#include "entities/Item.hpp"

#include <cstddef>
#include <vector>

namespace dnd::entities
{

    /// Инвентарь персонажа. Предметы адресуются индексом (удобно для списка в ImGui).
    /// Индексы действительны до следующего add/remove.
    class Inventory
    {
    public:
        /// Добавляет предмет. Misc/Consumable с тем же названием и типом складываются в стопку;
        /// оружие, доспехи и щиты добавляются отдельными записями (по одной на штуку).
        /// Предметы с quantity <= 0 игнорируются. Возвращает индекс (первой добавленной) записи
        /// или size() при игнорировании.
        std::size_t add(Item item);

        /// Убирает quantity штук предмета (по умолчанию 1). Если запись опустела — удаляет её.
        /// false, если индекс вне диапазона или quantity <= 0 / больше имеющегося количества.
        bool remove(std::size_t index, int quantity = 1);

        /// Тратит одну штуку расходуемого предмета. false, если это не Consumable или индекс неверный.
        bool consume(std::size_t index);

        /// Надевает/берёт в руки предмет. Только одна броня и один щит одновременно:
        /// надевание новой снимает прежнюю. false, если предмет нельзя экипировать.
        bool equip(std::size_t index);
        bool unequip(std::size_t index);

        const std::vector<Item> &items() const { return m_items; }
        std::vector<Item> &items() { return m_items; } ///< для редактирования в GUI
        std::size_t size() const { return m_items.size(); }
        bool empty() const { return m_items.empty(); }

        /// Индекс первого предмета с таким названием или size(), если не найден.
        std::size_t find(const std::string &name) const;

        const Item *equippedArmor() const;
        const Item *equippedShield() const;

        double totalWeight() const;

        /// Грузоподъёмность по правилам D&D 5e: 15 × значение Силы (в фунтах).
        static double carryCapacity(int strengthScore) { return 15.0 * strengthScore; }

    private:
        const Item *equippedOfType(ItemType type) const;

        std::vector<Item> m_items;
    };

} // namespace dnd::entities
