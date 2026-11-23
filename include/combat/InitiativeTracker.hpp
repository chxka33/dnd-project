#pragma once

#include "entities/Roster.hpp"
#include "scenario/ScenarioGraph.hpp"
#include "utils/Random.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace dnd::combat
{

    /// Участник боя: ссылка на персонажа в Roster (HP, КД, состояния живут там) и его инициатива.
    struct Combatant
    {
        std::size_t rosterIndex = 0;
        int initiative = 10;
        int initiativeBonus = 0;
        std::string sourceEnemyId; ///< id шаблона из бестиария (пусто для героев)
    };

    /// Трекер инициативы (модуль 3.3): очередь ходов, раунды, загрузка врагов из сценария.
    class InitiativeTracker
    {
    public:
        const std::vector<Combatant> &combatants() const { return m_combatants; }
        std::size_t size() const { return m_combatants.size(); }
        bool empty() const { return m_combatants.empty(); }
        void clear();

        // --- участники -------------------------------------------------------
        /// Убирает записи с неверным индексом и обновляет бонусы инициативы.
        void syncWithRoster(const entities::Roster &roster);

        /// Добавляет участника (или обновляет инициативу существующего). Возвращает позицию в очереди.
        std::size_t addFromRoster(const entities::Roster &roster, std::size_t rosterIndex, int initiative,
                                  std::string sourceEnemyId = {});

        /// Добавляет героев, которых ещё нет в трекере: d20 + мод. (если rng) либо 10 + мод.
        void addPlayersFromRoster(const entities::Roster &roster, utils::Random *rng = nullptr);

        /// Создаёт противника из шаблона бестиария, кладёт в Roster (Side::Enemy) и в трекер.
        std::size_t spawnEnemyFromTemplate(entities::Roster &roster, const scenario::EnemyTemplate &enemyTemplate,
                                           utils::Random *rng = nullptr,
                                           std::optional<int> fixedInitiative = std::nullopt);

        /// Загружает врагов узла сценария (+ всех героев). Возвращает число добавленных врагов.
        std::size_t loadNodeEnemies(entities::Roster &roster, const scenario::ScenarioGraph &graph,
                                    const scenario::Node &node, utils::Random *rng = nullptr,
                                    bool clearExistingEnemies = false);

        bool removeCombatant(std::size_t trackerIndex);
        /// rosterEntryRemoved: запись уже удалена из Roster, индексы выше сдвигаются на 1.
        bool removeByRosterIndex(std::size_t rosterIndex, bool rosterEntryRemoved = false);
        /// Удаляет всех противников из Roster и из трекера.
        void removeEnemies(entities::Roster &roster);

        // --- инициатива ------------------------------------------------------
        bool setInitiative(std::size_t trackerIndex, int initiative, const entities::Roster *roster = nullptr);
        void rollInitiativeForEnemies(const entities::Roster &roster, utils::Random &rng);
        void rollInitiativeForAll(const entities::Roster &roster, utils::Random &rng);
        /// Убывание инициативы; при равенстве — бонус, затем игрок перед врагом, затем индекс.
        void sort(const entities::Roster *roster = nullptr);

        // --- ходы и раунды ---------------------------------------------------
        int round() const { return m_round; }
        void setRound(int round) { m_round = std::max(1, round); }
        int activeTurnIndex() const { return m_activeTurnIndex; }
        void setActiveTurnIndex(int index);
        const Combatant *activeCombatant() const;

        void startCombat(const entities::Roster *roster = nullptr);
        const Combatant *nextTurn(const entities::Roster *roster = nullptr, bool skipDown = true);
        const Combatant *previousTurn(const entities::Roster *roster = nullptr, bool skipDown = true);

    private:
        std::size_t findByRosterIndex(std::size_t rosterIndex) const;
        bool isDown(const entities::Roster *roster, const Combatant &c) const;
        bool anyAlive(const entities::Roster *roster) const;

        std::vector<Combatant> m_combatants;
        int m_round = 1;
        int m_activeTurnIndex = 0;
    };

} // namespace dnd::combat
