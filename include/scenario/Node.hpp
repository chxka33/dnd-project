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

    /// Тип испытания в узле: проверка навыка или спасбросок.
    enum class CheckType
    {
        SkillCheck,  ///< обычная проверка характеристики / навыка
        SavingThrow  ///< спасбросок (от ловушки, заклинания и т.д.)
    };

    // Преобразования для сериализации (.dndsc) и отображения в UI.
    const char *toString(NodeType type);
    const char *toString(Ability ability);
    const char *toString(CheckType type);
    std::optional<NodeType> nodeTypeFromString(const std::string &s);
    std::optional<Ability> abilityFromString(const std::string &s);
    std::optional<CheckType> checkTypeFromString(const std::string &s);

    /// Испытание (проверка навыка или спасбросок), привязанное к узлу.
    struct CheckDefinition
    {
        CheckType type = CheckType::SkillCheck;
        Ability ability = Ability::Strength;
        int dc = 10;       ///< Класс Сложности (в D&D обычно 1..30)
        std::string skill; ///< необязательно: "Stealth", "Perception", ...
    };
    using SkillCheck = CheckDefinition; ///< для полной обратной совместимости со старым кодом

    /// Предмет в наградах узла сценария.
    struct LootItem
    {
        std::string name;
        int quantity = 1;
    };

    /// Награды и сокровища в узле (золото и предметы).
    struct Loot
    {
        int gold = 0;
        std::vector<LootItem> items;
    };

    /// Медиаданные узла. Пути относительны корня ассетов (ResourceManager::assetRoot()).
    struct NodeMedia
    {
        std::string mapPath; ///< 2D-карта локации
        std::string artPath; ///< арт локации / персонажа
    };

    /// Позиция узла на холсте редактора (чтобы нелинейный сюжет не «слипался» после загрузки).
    struct EditorPosition
    {
        float x = 0.f;
        float y = 0.f;
    };

    /// Базовый узел нелинейного сюжета.
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

        /// Все непустые исходящие связи (для обхода нелинейного сюжета, отрисовки стрелок, валидации).
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
        std::string title;                          ///< название сцены
        std::string summary;                        ///< краткое саммари для карты сценария
        std::string text;                           ///< художественный текст (для зачитывания игрокам)
        std::string dmNotes;                        ///< секретные заметки и подсказки Мастера
        NodeMedia media;                            ///< карта, арт
        std::optional<CheckDefinition> check;       ///< проверка навыка / спасбросок при входе в узел
        std::optional<CheckDefinition> &skillCheck; ///< ссылка для совместимости со старым кодом
        std::optional<Loot> loot;                   ///< сокровища и награды в узле
        std::vector<std::string> enemyIds;          ///< ID привязанных врагов из бестиария сценария
        EditorPosition position;                    ///< положение в редакторе

    private:
        const NodeId m_id;
    };

} // namespace dnd::scenario
