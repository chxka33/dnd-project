#include "entities/Inventory.hpp"

#include <utility>

namespace dnd::entities
{

    std::size_t Inventory::add(Item item)
    {
        if (item.quantity <= 0)
            return m_items.size();

        if (item.isStackable())
        {
            for (std::size_t i = 0; i < m_items.size(); ++i)
            {
                Item &existing = m_items[i];
                if (existing.type == item.type && existing.name == item.name)
                {
                    existing.quantity += item.quantity;
                    return i;
                }
            }
            m_items.push_back(std::move(item));
            return m_items.size() - 1;
        }

        // Нестекуемые предметы: по одной записи на штуку.
        const std::size_t first = m_items.size();
        const int count = item.quantity;
        item.quantity = 1;
        item.equipped = false;
        for (int i = 0; i < count; ++i)
            m_items.push_back(item);
        return first;
    }

    bool Inventory::remove(std::size_t index, int quantity)
    {
        if (index >= m_items.size() || quantity <= 0 || quantity > m_items[index].quantity)
            return false;

        m_items[index].quantity -= quantity;
        if (m_items[index].quantity == 0)
            m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    bool Inventory::consume(std::size_t index)
    {
        if (index >= m_items.size() || m_items[index].type != ItemType::Consumable)
            return false;
        return remove(index, 1);
    }

    bool Inventory::equip(std::size_t index)
    {
        if (index >= m_items.size() || !m_items[index].isEquippable())
            return false;

        const ItemType type = m_items[index].type;
        if (type == ItemType::Armor || type == ItemType::Shield)
        {
            for (Item &other : m_items)
                if (other.type == type)
                    other.equipped = false;
        }
        m_items[index].equipped = true;
        return true;
    }

    bool Inventory::unequip(std::size_t index)
    {
        if (index >= m_items.size() || !m_items[index].equipped)
            return false;
        m_items[index].equipped = false;
        return true;
    }

    std::size_t Inventory::find(const std::string &name) const
    {
        for (std::size_t i = 0; i < m_items.size(); ++i)
            if (m_items[i].name == name)
                return i;
        return m_items.size();
    }

    const Item *Inventory::equippedOfType(ItemType type) const
    {
        for (const Item &item : m_items)
            if (item.type == type && item.equipped)
                return &item;
        return nullptr;
    }

    const Item *Inventory::equippedArmor() const
    {
        const Item *armor = equippedOfType(ItemType::Armor);
        // Доспех без параметров (создан вручную в GUI) не считается надетым.
        return (armor && armor->armor) ? armor : nullptr;
    }

    const Item *Inventory::equippedShield() const
    {
        return equippedOfType(ItemType::Shield);
    }

    double Inventory::totalWeight() const
    {
        double total = 0.0;
        for (const Item &item : m_items)
            total += item.totalWeight();
        return total;
    }

} // namespace dnd::entities
