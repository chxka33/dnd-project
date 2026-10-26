#include "entities/CharacterClass.hpp"

namespace dnd::entities
{

    namespace
    {
        using A = Ability;
        using C = CasterType;

        // Порядок строк совпадает с порядком значений CharacterClass.
        const std::array<ClassInfo, kClassCount> kClasses = {{
            {"barbarian", "Варвар",     12, {A::Strength, A::Constitution},     C::None, std::nullopt},
            {"bard",      "Бард",        8, {A::Dexterity, A::Charisma},        C::Full, A::Charisma},
            {"cleric",    "Жрец",        8, {A::Wisdom, A::Charisma},           C::Full, A::Wisdom},
            {"druid",     "Друид",       8, {A::Intelligence, A::Wisdom},       C::Full, A::Wisdom},
            {"fighter",   "Воин",       10, {A::Strength, A::Constitution},     C::None, std::nullopt},
            {"monk",      "Монах",       8, {A::Strength, A::Dexterity},        C::None, std::nullopt},
            {"paladin",   "Паладин",    10, {A::Wisdom, A::Charisma},           C::Half, A::Charisma},
            {"ranger",    "Следопыт",   10, {A::Strength, A::Dexterity},        C::Half, A::Wisdom},
            {"rogue",     "Плут",        8, {A::Dexterity, A::Intelligence},    C::None, std::nullopt},
            {"sorcerer",  "Чародей",     6, {A::Constitution, A::Charisma},     C::Full, A::Charisma},
            {"warlock",   "Колдун",      8, {A::Wisdom, A::Charisma},           C::Pact, A::Charisma},
            {"wizard",    "Волшебник",   6, {A::Intelligence, A::Wisdom},       C::Full, A::Intelligence},
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
