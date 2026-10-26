#pragma once

#include "entities/AbilityScores.hpp"

#include <array>
#include <optional>
#include <string>

namespace dnd::entities
{

    enum class CharacterClass
    {
        Barbarian,
        Bard,
        Cleric,
        Druid,
        Fighter,
        Monk,
        Paladin,
        Ranger,
        Rogue,
        Sorcerer,
        Warlock,
        Wizard
    };

    inline constexpr std::size_t kClassCount = 12;

    inline constexpr std::array<CharacterClass, kClassCount> kAllClasses = {
        CharacterClass::Barbarian, CharacterClass::Bard, CharacterClass::Cleric,
        CharacterClass::Druid, CharacterClass::Fighter, CharacterClass::Monk,
        CharacterClass::Paladin, CharacterClass::Ranger, CharacterClass::Rogue,
        CharacterClass::Sorcerer, CharacterClass::Warlock, CharacterClass::Wizard};

    /// Как класс получает ячейки заклинаний.
    enum class CasterType
    {
        None, ///< не колдует
        Full, ///< полный заклинатель (бард, жрец, друид, чародей, волшебник)
        Half, ///< полузаклинатель (паладин, следопыт): заклинания с 2-го уровня
        Pact  ///< магия договора (колдун): мало ячеек одного уровня
    };

    /// Статические данные класса (правила D&D 5e).
    struct ClassInfo
    {
        const char *name;                           ///< идентификатор ("fighter") — для .dndsc
        const char *displayName;                    ///< название для UI
        int hitDie;                                 ///< кость хитов: 6, 8, 10 или 12
        std::array<Ability, 2> savingThrows;        ///< спасброски, в которых класс владеет
        CasterType caster;
        std::optional<Ability> spellcastingAbility; ///< ключевая характеристика заклинаний
    };

    const ClassInfo &classInfo(CharacterClass cls);

    const char *toString(CharacterClass cls);
    std::optional<CharacterClass> characterClassFromString(const std::string &s);

} // namespace dnd::entities
