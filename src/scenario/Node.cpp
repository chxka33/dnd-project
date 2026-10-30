#include "scenario/Node.hpp"

#include <utility>

namespace dnd::scenario
{

    Node::Node(NodeId id, std::string title_)
        : title(std::move(title_)), skillCheck(check), m_id(std::move(id)) {}

    const char *toString(NodeType type)
    {
        switch (type)
        {
        case NodeType::Story:  return "story";
        case NodeType::Choice: return "choice";
        case NodeType::Random: return "random";
        }
        return "story";
    }

    const char *toString(Ability ability)
    {
        switch (ability)
        {
        case Ability::Strength:     return "strength";
        case Ability::Dexterity:    return "dexterity";
        case Ability::Constitution: return "constitution";
        case Ability::Intelligence: return "intelligence";
        case Ability::Wisdom:       return "wisdom";
        case Ability::Charisma:     return "charisma";
        }
        return "strength";
    }

    const char *toString(CheckType type)
    {
        switch (type)
        {
        case CheckType::SkillCheck:  return "skill_check";
        case CheckType::SavingThrow: return "saving_throw";
        }
        return "skill_check";
    }

    std::optional<NodeType> nodeTypeFromString(const std::string &s)
    {
        for (NodeType t : {NodeType::Story, NodeType::Choice, NodeType::Random})
            if (s == toString(t))
                return t;
        return std::nullopt;
    }

    std::optional<Ability> abilityFromString(const std::string &s)
    {
        for (Ability a : {Ability::Strength, Ability::Dexterity, Ability::Constitution,
                          Ability::Intelligence, Ability::Wisdom, Ability::Charisma})
            if (s == toString(a))
                return a;
        return std::nullopt;
    }

    std::optional<CheckType> checkTypeFromString(const std::string &s)
    {
        if (s == "skill_check" || s == "skill" || s == "SkillCheck")
            return CheckType::SkillCheck;
        if (s == "saving_throw" || s == "save" || s == "SavingThrow")
            return CheckType::SavingThrow;
        return std::nullopt;
    }

} // namespace dnd::scenario
