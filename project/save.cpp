#include "save.h"
#include "core.h"

#include <fstream>
#include <sstream>
#include <vector>

namespace nyx {

SaveMeta::SaveMeta()
    : roomId(0), turns(0), flareLimit(70), seed(1), difficulty(1), flags() {}

namespace {

std::string flagList(const std::map<std::string, bool>& flags) {
    std::vector<std::string> names;
    std::map<std::string, bool>::const_iterator it = flags.begin();
    for (; it != flags.end(); ++it) {
        if (it->second) {
            names.push_back(it->first);
        }
    }
    if (names.empty()) {
        return "-";
    }
    return join(names, ",");
}

bool splitKeyValue(const std::string& line, std::string& key, std::string& value) {
    std::size_t space = line.find(' ');
    if (space == std::string::npos) {
        key = trim(line);
        value.clear();
        return !key.empty();
    }
    key = trim(line.substr(0, space));
    value = trim(line.substr(space + 1));
    return !key.empty();
}

std::string orDash(const std::string& value) {
    return value.empty() ? std::string("-") : value;
}

std::string undash(const std::string& value) {
    return (value == "-") ? std::string() : value;
}

} // namespace

bool SaveSystem::save(const std::string& path, const Player& player,
                      const World& world, const SaveMeta& meta) {
    std::ofstream out(path.c_str());
    if (!out) {
        return false;
    }

    out << "NYX9SAVE 1\n";
    out << "name " << player.name() << "\n";
    out << "level " << player.level() << "\n";
    out << "xp " << player.xp() << "\n";
    out << "credits " << player.credits() << "\n";
    out << "maxhp " << player.maxHp() << "\n";
    out << "atk " << player.attack() << "\n";
    out << "def " << player.defense() << "\n";
    out << "hp " << player.hp() << "\n";
    out << "weapon " << orDash(player.weaponId()) << "\n";
    out << "armor " << orDash(player.armorId()) << "\n";

    std::vector<std::string> carried;
    std::vector<const Item*> items = player.inventory().list();
    for (std::size_t i = 0; i < items.size(); ++i) {
        carried.push_back(items[i]->id);
    }
    out << "inv " << (carried.empty() ? std::string("-") : join(carried, ",")) << "\n";

    out << "room " << meta.roomId << "\n";
    out << "turns " << meta.turns << "\n";
    out << "flare " << meta.flareLimit << "\n";
    out << "seed " << meta.seed << "\n";
    out << "difficulty " << meta.difficulty << "\n";
    out << "flags " << flagList(meta.flags) << "\n";

    out << "rooms " << world.roomCount() << "\n";
    for (int i = 0; i < world.roomCount(); ++i) {
        const Room& room = world.room(i);
        out << room.id << "|" << (room.visited ? 1 : 0) << "|"
            << orDash(room.enemyId) << "|"
            << (room.items.empty() ? std::string("-") : join(room.items, ","))
            << "\n";
    }

    out.flush();
    return out.good();
}

bool SaveSystem::load(const std::string& path, Player& player, World& world,
                      SaveMeta& meta, const ItemRegistry& items) {
    std::ifstream in(path.c_str());
    if (!in) {
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }
    if (trim(line) != "NYX9SAVE 1") {
        return false;
    }

    std::string name = player.name();
    int level = 1;
    int xp = 0;
    int credits = 0;
    int maxHp = player.maxHp();
    int attack = player.attack();
    int defense = player.defense();
    int hp = player.hp();
    std::string weapon;
    std::string armor;
    std::vector<std::string> carried;
    int roomsExpected = -1;
    int roomsRead = 0;
    bool havePlayer = false;

    while (std::getline(in, line)) {
        std::string cleaned = trim(line);
        if (cleaned.empty()) {
            continue;
        }

        if (roomsExpected >= 0 && roomsRead < roomsExpected) {
            std::vector<std::string> fields = split(cleaned, '|');
            if (fields.size() < 4) {
                return false;
            }
            int roomId = toInt(trim(fields[0]), -1);
            if (!world.validRoom(roomId)) {
                return false;
            }
            Room& room = world.room(roomId);
            room.visited = (toInt(trim(fields[1]), 0) != 0);
            room.enemyId = undash(trim(fields[2]));
            room.items.clear();
            std::string csv = trim(fields[3]);
            if (!csv.empty() && csv != "-") {
                std::vector<std::string> ids = split(csv, ',');
                for (std::size_t i = 0; i < ids.size(); ++i) {
                    std::string id = trim(ids[i]);
                    if (!id.empty()) {
                        room.items.push_back(id);
                    }
                }
            }
            ++roomsRead;
            continue;
        }

        std::string key;
        std::string value;
        if (!splitKeyValue(cleaned, key, value)) {
            continue;
        }

        if (key == "name") {
            if (!value.empty()) {
                name = value;
            }
        } else if (key == "level") {
            level = toInt(value, 1);
            havePlayer = true;
        } else if (key == "xp") {
            xp = toInt(value, 0);
        } else if (key == "credits") {
            credits = toInt(value, 0);
        } else if (key == "maxhp") {
            maxHp = toInt(value, 30);
        } else if (key == "atk") {
            attack = toInt(value, 4);
        } else if (key == "def") {
            defense = toInt(value, 1);
        } else if (key == "hp") {
            hp = toInt(value, 30);
        } else if (key == "weapon") {
            weapon = undash(value);
        } else if (key == "armor") {
            armor = undash(value);
        } else if (key == "inv") {
            carried.clear();
            if (value != "-" && !value.empty()) {
                std::vector<std::string> ids = split(value, ',');
                for (std::size_t i = 0; i < ids.size(); ++i) {
                    std::string id = trim(ids[i]);
                    if (!id.empty()) {
                        carried.push_back(id);
                    }
                }
            }
        } else if (key == "room") {
            meta.roomId = toInt(value, 0);
        } else if (key == "turns") {
            meta.turns = toInt(value, 0);
        } else if (key == "flare") {
            meta.flareLimit = toInt(value, 70);
        } else if (key == "seed") {
            meta.seed = static_cast<unsigned int>(toInt(value, 1));
        } else if (key == "difficulty") {
            meta.difficulty = toInt(value, 1);
        } else if (key == "flags") {
            meta.flags.clear();
            if (value != "-" && !value.empty()) {
                std::vector<std::string> names = split(value, ',');
                for (std::size_t i = 0; i < names.size(); ++i) {
                    std::string flag = trim(names[i]);
                    if (!flag.empty()) {
                        meta.flags[flag] = true;
                    }
                }
            }
        } else if (key == "rooms") {
            roomsExpected = toInt(value, -1);
            if (roomsExpected != world.roomCount()) {
                return false;
            }
            roomsRead = 0;
        }
    }

    if (!havePlayer || roomsExpected < 0 || roomsRead != roomsExpected) {
        return false;
    }

    // Rebuild the character only after the whole file parsed successfully,
    // so a corrupt save leaves the running game untouched.
    player.setName(name);
    player.restoreState(level, xp, credits, maxHp, attack, defense);
    player.inventory().clear();

    if (!weapon.empty() && items.find(weapon) == 0) {
        weapon.clear();
    }
    if (!armor.empty() && items.find(armor) == 0) {
        armor.clear();
    }
    player.equipWeapon(weapon);
    player.equipArmor(armor);

    for (std::size_t i = 0; i < carried.size(); ++i) {
        const Item* item = items.find(carried[i]);
        if (item != 0) {
            player.inventory().add(*item);
        }
    }

    player.setHp(hp);
    if (!world.validRoom(meta.roomId)) {
        meta.roomId = world.startRoom();
    }
    return true;
}

} // namespace nyx
