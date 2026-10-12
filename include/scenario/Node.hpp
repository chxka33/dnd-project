#pragma once

#include "utils/Random.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace dnd::scenario
{

    /// Строковый идентификатор узла (уникален в пределах сценария).
    /// Строки, а не числа: удобно читать в .dndsc и не ломаются при удалении узлов.
    using NodeId = std::string;

    enum class NodeType
    {
        Story,  ///< обязательное сюжетное событие (линейный переход)
        Choice, ///< развилка: игроки выбирают один из вариантов
        Random  ///< вероятностный узел: случайная встреча / событие
    };

    /// Шесть характеристик D&D.
    enum class Ability
    {
        Strength,
        Dexterity,
        Constitution,
        Intelligence,
        Wisdom,
        Charisma
    };

    // Преобразования для сериализации (.dndsc) и отображения в UI.
    const char *toString(NodeType type);
    const char *toString(Ability ability);
    std::optional<NodeType> nodeTypeFromString(const std::string &s);
    std::optional<Ability> abilityFromString(const std::string &s);

    /// Проверка навыка, привязанная к узлу: бросок d20 + модификатор против DC.
    /// Сам бросок и подсчёт исхода делает симулятор (Ярошук) — здесь только параметры.
    struct SkillCheck
    {
        Ability ability = Ability::Strength;
        int dc = 10;       ///< Класс Сложности (в D&D обычно 1..30)
        std::string skill; ///< необязательно: "Stealth", "Perception", ...
    };

    /// Медиаданные узла. Пути относительны корня ассетов (ResourceManager::assetRoot()).
    struct NodeMedia
    {
        std::string mapPath;   ///< 2D-карта локации
        std::string artPath;   ///< арт локации / персонажа
        std::string musicPath; ///< фоновая музыка
    };

    /// Позиция узла на холсте редактора (чтобы граф не «слипался» после загрузки).
    struct EditorPosition
    {
        float x = 0.f;
        float y = 0.f;
    };

    /// Базовый узел графа сценария.
    ///
    /// Узел знает только СВОИ исходящие связи (по идентификаторам); владельцем узлов
    /// и связей между ними является ScenarioGraph.
    class Node
    {
    public:
        explicit Node(NodeId id, std::string title = {});
        virtual ~Node() = default;

        Node(const Node &) = delete;
        Node &operator=(const Node &) = delete;

        virtual NodeType type() const = 0;

        /// Все непустые исходящие связи (для обхода графа, отрисовки стрелок, валидации).
        virtual std::vector<NodeId> targets() const = 0;

        /// Куда перейти дальше.
        ///  - Story:  единственный следующий узел (пустая строка — конец сценария);
        ///  - Choice: узел выбранного варианта (choiceIndex);
        ///  - Random: бросает кубик через rng и выбирает ветку.
        /// Пустой результат означает «сценарий закончен».
        virtual NodeId resolveNext(utils::Random &rng, std::size_t choiceIndex = 0) const = 0;

        /// Заменяет все связи from -> to (to может быть пустым: «связь удалена»).
        /// Возвращает число заменённых связей.
        virtual std::size_t replaceTarget(const NodeId &from, const NodeId &to) = 0;

        const NodeId &id() const { return m_id; }

        // Общие данные узла. Публичные поля: их напрямую редактирует GUI редактора.
        std::string title;                       ///< короткое название (для графа)
        std::string text;                        ///< текст, который читает Мастер/игроки
        NodeMedia media;                         ///< карта, арт, музыка
        std::optional<SkillCheck> skillCheck;    ///< проверка навыка при входе в узел
        EditorPosition position;                 ///< положение в редакторе

    private:
        const NodeId m_id;
    };

} // namespace dnd::scenario
