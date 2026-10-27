#pragma once

#include "entities/Character.hpp"

#include <cstddef>
#include <vector>

namespace dnd::entities
{

    enum class Side
    {
        Player,
        Enemy
    };

    const char *toString(Side side);

    struct RosterEntry
    {
        Character character;
        Side side = Side::Player;
    };

    /// Все персонажи текущей игровой сессии: группа игроков и противники.
    /// Один общий экземпляр живёт в AppContext, чтобы лист персонажа, панель Мастера
    /// и симулятор видели одних и тех же героев.
    class Roster
    {
    public:
        /// Добавляет персонажа, возвращает его индекс.
        std::size_t add(Character character, Side side);
        /// false, если индекс вне диапазона.
        bool remove(std::size_t index);
        void clear() { m_entries.clear(); }

        std::size_t size() const { return m_entries.size(); }
        bool empty() const { return m_entries.empty(); }
        RosterEntry &at(std::size_t index) { return m_entries.at(index); }
        const RosterEntry &at(std::size_t index) const { return m_entries.at(index); }

        const std::vector<RosterEntry> &entries() const { return m_entries; }

        std::size_t count(Side side) const;
        /// Сколько персонажей стороны ещё на ногах (HP > 0).
        std::size_t countAlive(Side side) const;

        /// Длинный отдых всем игрокам (противники не отдыхают).
        void longRestPlayers();

        /// Добавляет готовую демонстрационную группу: 4 героя и 2 противника со снаряжением.
        void addDemoParty();

    private:
        std::vector<RosterEntry> m_entries;
    };

} // namespace dnd::entities
