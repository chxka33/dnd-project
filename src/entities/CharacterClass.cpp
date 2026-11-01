#include "entities/CharacterClass.hpp"

namespace dnd::entities
{

    namespace
    {
        using A = Ability;
        using C = CasterType;

        // Порядок строк совпадает с порядком значений CharacterClass.
        const std::array<ClassInfo, kClassCount> kClasses = {{
            {"barbarian", "Barbarian",     12, {A::Strength, A::Constitution},     C::None, std::nullopt},
            {"bard",      "Bard",        8, {A::Dexterity, A::Charisma},        C::Full, A::Charisma},
            {"cleric",    "Cleric",        8, {A::Wisdom, A::Charisma},           C::Full, A::Wisdom},
            {"druid",     "Druid",       8, {A::Intelligence, A::Wisdom},       C::Full, A::Wisdom},
            {"fighter",   "Fighter",       10, {A::Strength, A::Constitution},     C::None, std::nullopt},
            {"monk",      "Monk",       8, {A::Strength, A::Dexterity},        C::None, std::nullopt},
            {"paladin",   "Paladin",    10, {A::Wisdom, A::Charisma},           C::Half, A::Charisma},
            {"ranger",    "Ranger",   10, {A::Strength, A::Dexterity},        C::Half, A::Wisdom},
            {"rogue",     "Rogue",        8, {A::Dexterity, A::Intelligence},    C::None, std::nullopt},
            {"sorcerer",  "Sorcerer",     6, {A::Constitution, A::Charisma},     C::Full, A::Charisma},
            {"warlock",   "Warlock",      8, {A::Wisdom, A::Charisma},           C::Pact, A::Charisma},
            {"wizard",    "Wizard",   6, {A::Intelligence, A::Wisdom},       C::Full, A::Intelligence},
        }};
    } // namespace

    const ClassInfo &classInfo(CharacterClass cls)
    {
        return kClasses[static_cast<std::size_t>(cls)];
    }

    const char *toString(CharacterClass cls)
    {
        return classInfo(cls).name;
    }

    std::optional<CharacterClass> characterClassFromString(const std::string &s)
    {
        for (CharacterClass cls : kAllClasses)
            if (s == toString(cls))
                return cls;
        return std::nullopt;
    }

} // namespace dnd::entities
