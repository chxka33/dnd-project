#include "scenario/CharacterSerializer.hpp"

#include <fstream>
#include <system_error>

namespace dnd::scenario
{

    namespace
    {

        using namespace dnd::entities;

        nlohmann::json serializeWeapon(const WeaponStats &w)
        {
            return {
                {"damage", w.damage},
                {"damageType", w.damageType},
                {"finesse", w.finesse},
                {"ranged", w.ranged}};
        }

        WeaponStats deserializeWeapon(const nlohmann::json &j)
        {
            WeaponStats w;
            if (j.is_object())
            {
                w.damage = j.value("damage", "1d6");
                w.damageType = j.value("damageType", "");
                w.finesse = j.value("finesse", false);
                w.ranged = j.value("ranged", false);
            }
            return w;
        }

        nlohmann::json serializeArmor(const ArmorStats &a)
        {
            return {
                {"category", toString(a.category)},
                {"baseAc", a.baseAc},
                {"strengthRequirement", a.strengthRequirement},
                {"stealthDisadvantage", a.stealthDisadvantage}};
        }

        ArmorStats deserializeArmor(const nlohmann::json &j)
        {
            ArmorStats a;
            if (j.is_object())
            {
                if (j.contains("category") && j["category"].is_string())
                {
                    if (auto cat = armorCategoryFromString(j["category"].get<std::string>()))
                        a.category = *cat;
                }
                a.baseAc = j.value("baseAc", 10);
                a.strengthRequirement = j.value("strengthRequirement", 0);
                a.stealthDisadvantage = j.value("stealthDisadvantage", false);
            }
            return a;
        }

        nlohmann::json serializeItem(const Item &item)
        {
            nlohmann::json j;
            j["name"] = item.name;
            j["type"] = toString(item.type);
            j["description"] = item.description;
            j["weight"] = item.weight;
            j["quantity"] = item.quantity;
            j["equipped"] = item.equipped;
            j["shieldBonus"] = item.shieldBonus;

            if (item.weapon)
                j["weapon"] = serializeWeapon(*item.weapon);
            if (item.armor)
                j["armor"] = serializeArmor(*item.armor);

            return j;
        }

        Item deserializeItem(const nlohmann::json &j)
        {
            Item item;
            if (j.is_object())
            {
                item.name = j.value("name", "");
                if (j.contains("type") && j["type"].is_string())
                {
                    if (auto t = itemTypeFromString(j["type"].get<std::string>()))
                        item.type = *t;
                }
                item.description = j.value("description", "");
                item.weight = j.value("weight", 0.0);
                item.quantity = j.value("quantity", 1);
                item.equipped = j.value("equipped", false);
                item.shieldBonus = j.value("shieldBonus", 0);

                if (j.contains("weapon") && j["weapon"].is_object())
                    item.weapon = deserializeWeapon(j["weapon"]);
                if (j.contains("armor") && j["armor"].is_object())
                    item.armor = deserializeArmor(j["armor"]);
            }
            return item;
        }

        nlohmann::json serializeAbilities(const AbilityScores &a)
        {
            return {
                {"strength", a.score(Ability::Strength)},
                {"dexterity", a.score(Ability::Dexterity)},
                {"constitution", a.score(Ability::Constitution)},
                {"intelligence", a.score(Ability::Intelligence)},
                {"wisdom", a.score(Ability::Wisdom)},
                {"charisma", a.score(Ability::Charisma)}};
        }

        AbilityScores deserializeAbilities(const nlohmann::json &j)
        {
            AbilityScores a;
            if (j.is_object())
            {
                a.setScore(Ability::Strength, j.value("strength", 10));
                a.setScore(Ability::Dexterity, j.value("dexterity", 10));
                a.setScore(Ability::Constitution, j.value("constitution", 10));
                a.setScore(Ability::Intelligence, j.value("intelligence", 10));
                a.setScore(Ability::Wisdom, j.value("wisdom", 10));
                a.setScore(Ability::Charisma, j.value("charisma", 10));
            }
            return a;
        }

    } // namespace

    nlohmann::json CharacterSerializer::toJson(const entities::Character &character)
    {
        nlohmann::json j;
        j["name"] = character.name;
        j["class"] = toString(character.characterClass());
        j["level"] = character.level();
        j["abilities"] = serializeAbilities(character.abilities());
        j["currentHp"] = character.currentHp();
        j["tempHp"] = character.temporaryHp();
        j["miscAcBonus"] = character.miscArmorClassBonus();
        j["conditions"] = character.conditions();

        // Инвентарь
        nlohmann::json itemsArr = nlohmann::json::array();
        for (const auto &item : character.inventory().items())
        {
            itemsArr.push_back(serializeItem(item));
        }
        j["inventory"] = itemsArr;

        // Ячейки заклинаний
        if (character.isSpellcaster())
        {
            nlohmann::json slotsArr = nlohmann::json::array();
            for (int lvl = 1; lvl <= SpellSlots::kMaxSpellLevel; ++lvl)
            {
                slotsArr.push_back({
                    {"level", lvl},
                    {"current", character.spellSlots().current(lvl)},
                    {"max", character.spellSlots().max(lvl)}});
            }
            j["spellSlots"] = slotsArr;
        }

        return j;
    }

    std::unique_ptr<entities::Character> CharacterSerializer::fromJson(const nlohmann::json &j,
                                                                      std::string *outError)
    {
        try
        {
            if (!j.is_object())
            {
                if (outError)
                    *outError = "Character JSON must be an object.";
                return nullptr;
            }

            const std::string name = j.value("name", "Unnamed");
            const std::string clsStr = j.value("class", "fighter");
            auto optCls = characterClassFromString(clsStr);
            if (!optCls)
            {
                if (outError)
                    *outError = "Unknown character class: " + clsStr;
                return nullptr;
            }

            const int level = j.value("level", 1);
            AbilityScores abilities;
            if (j.contains("abilities"))
                abilities = deserializeAbilities(j["abilities"]);

            auto character = std::make_unique<entities::Character>(name, *optCls, level, abilities);

            if (j.contains("currentHp") && j["currentHp"].is_number_integer())
                character->setCurrentHp(j["currentHp"].get<int>());

            if (j.contains("tempHp") && j["tempHp"].is_number_integer())
                character->grantTemporaryHp(j["tempHp"].get<int>());

            if (j.contains("miscAcBonus") && j["miscAcBonus"].is_number_integer())
                character->setMiscArmorClassBonus(j["miscAcBonus"].get<int>());

            if (j.contains("conditions") && j["conditions"].is_array())
            {
                for (const auto &condJ : j["conditions"])
                {
                    if (condJ.is_string())
                        character->addCondition(condJ.get<std::string>());
                }
            }

            if (j.contains("inventory") && j["inventory"].is_array())
            {
                for (const auto &itemJ : j["inventory"])
                {
                    Item item = deserializeItem(itemJ);
                    if (!item.name.empty())
                    {
                        const bool wasEquipped = item.equipped;
                        item.equipped = false;
                        const auto idx = character->inventory().add(item);
                        if (wasEquipped && idx < character->inventory().size())
                            character->inventory().equip(idx);
                    }
                }
            }

            if (j.contains("spellSlots") && j["spellSlots"].is_array())
            {
                for (const auto &slotJ : j["spellSlots"])
                {
                    if (slotJ.is_object())
                    {
                        int lvl = slotJ.value("level", 0);
                        int cur = slotJ.value("current", -1);
                        if (lvl >= 1 && lvl <= SpellSlots::kMaxSpellLevel && cur >= 0)
                        {
                            while (character->spellSlots().current(lvl) > cur)
                            {
                                if (!character->spellSlots().use(lvl))
                                    break;
                            }
                        }
                    }
                }
            }

            return character;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Character deserialize error: ") + ex.what();
            return nullptr;
        }
    }

    bool CharacterSerializer::saveToFile(const entities::Character &character,
                                         const std::filesystem::path &filePath,
                                         std::string *outError,
                                         int indent)
    {
        try
        {
            if (filePath.has_parent_path())
            {
                std::error_code ec;
                std::filesystem::create_directories(filePath.parent_path(), ec);
                if (ec)
                {
                    if (outError)
                        *outError = "Cannot create directories for " + filePath.string() + ": " + ec.message();
                    return false;
                }
            }

            std::ofstream out(filePath, std::ios::out | std::ios::trunc);
            if (!out.is_open())
            {
                if (outError)
                    *outError = "Cannot open file for writing: " + filePath.string();
                return false;
            }

            nlohmann::json root;
            root["formatVersion"] = kCurrentFormatVersion;
            root["type"] = "single_character";
            root["character"] = toJson(character);

            out << root.dump(indent) << '\n';
            return true;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Failed to save character: ") + ex.what();
            return false;
        }
    }

    std::unique_ptr<entities::Character> CharacterSerializer::loadFromFile(const std::filesystem::path &filePath,
                                                                          std::string *outError)
    {
        try
        {
            std::ifstream in(filePath, std::ios::in);
            if (!in.is_open())
            {
                if (outError)
                    *outError = "File not found: " + filePath.string();
                return nullptr;
            }

            nlohmann::json root;
            in >> root;

            if (root.contains("character") && root["character"].is_object())
                return fromJson(root["character"], outError);

            // Если сам root является объектом персонажа
            return fromJson(root, outError);
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Failed to load character: ") + ex.what();
            return nullptr;
        }
    }

    nlohmann::json CharacterSerializer::rosterToJson(const entities::Roster &roster)
    {
        nlohmann::json root;
        root["formatVersion"] = kCurrentFormatVersion;
        root["type"] = "roster";

        nlohmann::json entriesArr = nlohmann::json::array();
        for (const auto &entry : roster.entries())
        {
            nlohmann::json entryJ;
            entryJ["side"] = (entry.side == Side::Player) ? "player" : "enemy";
            entryJ["character"] = toJson(entry.character);
            entriesArr.push_back(entryJ);
        }
        root["entries"] = entriesArr;

        return root;
    }

    bool CharacterSerializer::rosterFromJson(const nlohmann::json &j,
                                            entities::Roster &outRoster,
                                            std::string *outError)
    {
        try
        {
            if (!j.is_object())
            {
                if (outError)
                    *outError = "Roster JSON must be an object.";
                return false;
            }

            outRoster.clear();

            if (!j.contains("entries") || !j["entries"].is_array())
            {
                if (outError)
                    *outError = "Missing 'entries' array in roster JSON.";
                return false;
            }

            for (const auto &entryJ : j["entries"])
            {
                if (entryJ.is_object() && entryJ.contains("character"))
                {
                    Side side = Side::Player;
                    if (entryJ.value("side", "player") == "enemy")
                        side = Side::Enemy;

                    auto ch = fromJson(entryJ["character"], outError);
                    if (ch)
                        outRoster.add(std::move(*ch), side);
                }
            }
            return true;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Failed to parse roster: ") + ex.what();
            return false;
        }
    }

    bool CharacterSerializer::saveRosterToFile(const entities::Roster &roster,
                                              const std::filesystem::path &filePath,
                                              std::string *outError,
                                              int indent)
    {
        try
        {
            if (filePath.has_parent_path())
            {
                std::error_code ec;
                std::filesystem::create_directories(filePath.parent_path(), ec);
                if (ec)
                {
                    if (outError)
                        *outError = "Cannot create directories for " + filePath.string() + ": " + ec.message();
                    return false;
                }
            }

            std::ofstream out(filePath, std::ios::out | std::ios::trunc);
            if (!out.is_open())
            {
                if (outError)
                    *outError = "Cannot open file for writing: " + filePath.string();
                return false;
            }

            const nlohmann::json root = rosterToJson(roster);
            out << root.dump(indent) << '\n';
            return true;
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Failed to save roster: ") + ex.what();
            return false;
        }
    }

    bool CharacterSerializer::loadRosterFromFile(const std::filesystem::path &filePath,
                                                entities::Roster &outRoster,
                                                std::string *outError)
    {
        try
        {
            std::ifstream in(filePath, std::ios::in);
            if (!in.is_open())
            {
                if (outError)
                    *outError = "File not found: " + filePath.string();
                return false;
            }

            nlohmann::json root;
            in >> root;
            return rosterFromJson(root, outRoster, outError);
        }
        catch (const std::exception &ex)
        {
            if (outError)
                *outError = std::string("Failed to load roster: ") + ex.what();
            return false;
        }
    }

} // namespace dnd::scenario
