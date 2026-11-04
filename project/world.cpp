#include "world.h"
#include "core.h"

#include <fstream>

namespace nyx {

Room::Room() : id(-1), name(), description(), items(), enemyId(), lock(), visited(false) {
    exits[0] = -1;
    exits[1] = -1;
    exits[2] = -1;
    exits[3] = -1;
}

World::World() : rooms_(), start_(0) {}

bool World::loadFromFile(const std::string& path) {
    std::ifstream in(path.c_str());
    if (!in) {
        return false;
    }

    std::vector<Room> loaded;
    std::string line;
    while (std::getline(in, line)) {
        std::string cleaned = trim(line);
        if (cleaned.empty() || cleaned[0] == '#') {
            continue;
        }
        std::vector<std::string> fields = split(cleaned, '|');
        if (fields.size() < 10) {
            continue;
        }

        Room room;
        room.id = toInt(trim(fields[0]), -1);
        room.name = trim(fields[1]);
        room.description = trim(fields[2]);
        for (int d = 0; d < 4; ++d) {
            std::string cell = trim(fields[3 + static_cast<std::size_t>(d)]);
            room.exits[d] = (cell == "-" || cell.empty()) ? -1 : toInt(cell, -1);
        }

        std::string itemCsv = trim(fields[7]);
        if (!itemCsv.empty() && itemCsv != "-") {
            std::vector<std::string> ids = split(itemCsv, ',');
            for (std::size_t i = 0; i < ids.size(); ++i) {
                std::string id = trim(ids[i]);
                if (!id.empty()) {
                    room.items.push_back(id);
                }
            }
        }

        std::string enemyId = trim(fields[8]);
        room.enemyId = (enemyId == "-") ? std::string() : enemyId;

        std::string lock = trim(fields[9]);
        room.lock = (lock == "-") ? std::string() : lock;

        if (room.id < 0) {
            continue;
        }
        loaded.push_back(room);
    }

    if (loaded.empty()) {
        return false;
    }

    // Room ids double as vector indices, and exits refer to those indices.
    // Reject a file whose ids are not 0..N-1 in order, so the game falls
    // back to the built-in station instead of walking off the vector.
    for (std::size_t i = 0; i < loaded.size(); ++i) {
        if (loaded[i].id != static_cast<int>(i)) {
            return false;
        }
    }

    rooms_ = loaded;
    start_ = 0;
    return true;
}

void World::buildDefault() {
    // id, name, description, north, east, south, west, enemy, lock
    struct Seed {
        int id;
        const char* name;
        const char* description;
        int north;
        int east;
        int south;
        int west;
        const char* enemy;
        const char* lock;
    };

    static const Seed seeds[8] = {
        {0, "Docking Bay",
         "Your shuttle Kestrel waits in the launch cradle, hull ticking as it cools. "
         "Beyond the viewport the collapsar's accretion ring burns a dull orange.",
         -1, -1, -1, -1, "", ""},
        {1, "Main Corridor",
         "Emergency strips pulse along the deck plating. Doors branch west to the armoury, "
         "east to the medbay and north to hydroponics.",
         3, 2, 0, 4, "", ""},
        {2, "Medbay",
         "Autoclave lights flicker over overturned gurneys. The cabinets hang open.",
         -1, -1, -1, 1, "", ""},
        {3, "Hydroponics",
         "Rows of engineered soy twist under violet lamps. Something has been feeding on them.",
         5, 7, 1, -1, "", ""},
        {4, "Armoury",
         "Racks of spent casings. The security locker stands open behind a targeting rail.",
         -1, 1, -1, -1, "", ""},
        {5, "Reactor Access",
         "A bulkhead stamped SECURITY CLEARANCE REQUIRED. Heat rolls off the deck plates.",
         6, -1, 3, -1, "", ""},
        {6, "Reactor Core",
         "The fusion torus hangs silent in its cradle, cold. A console blinks amber.",
         -1, -1, 5, -1, "", ""},
        {7, "Server Vault",
         "The AI's core housing. Cables hang like roots. Something vast unfolds in the dark.",
         -1, -1, -1, 3, "", ""}
    };

    rooms_.clear();
    for (int i = 0; i < 8; ++i) {
        Room room;
        room.id = seeds[i].id;
        room.name = seeds[i].name;
        room.description = seeds[i].description;
        room.exits[DIR_NORTH] = seeds[i].north;
        room.exits[DIR_EAST] = seeds[i].east;
        room.exits[DIR_SOUTH] = seeds[i].south;
        room.exits[DIR_WEST] = seeds[i].west;
        room.enemyId = seeds[i].enemy;
        room.lock = seeds[i].lock;
        rooms_.push_back(room);
    }
    start_ = 0;
}

Room& World::room(int index) {
    return rooms_[static_cast<std::size_t>(index)];
}

const Room& World::room(int index) const {
    return rooms_[static_cast<std::size_t>(index)];
}

int World::roomCount() const {
    return static_cast<int>(rooms_.size());
}

int World::startRoom() const {
    return start_;
}

bool World::validRoom(int index) const {
    return index >= 0 && index < static_cast<int>(rooms_.size());
}

std::string World::directionName(int dir) {
    switch (dir) {
        case DIR_NORTH: return "north";
        case DIR_EAST:  return "east";
        case DIR_SOUTH: return "south";
        case DIR_WEST:  return "west";
        default:        return "nowhere";
    }
}

int World::directionIndex(const std::string& text) {
    std::string t = toLower(trim(text));
    if (t == "n" || t == "north") return DIR_NORTH;
    if (t == "e" || t == "east")  return DIR_EAST;
    if (t == "s" || t == "south") return DIR_SOUTH;
    if (t == "w" || t == "west")  return DIR_WEST;
    return -1;
}

const char* World::directionLetter(int dir) {
    switch (dir) {
        case DIR_NORTH: return "N";
        case DIR_EAST:  return "E";
        case DIR_SOUTH: return "S";
        case DIR_WEST:  return "W";
        default:        return "?";
    }
}

} // namespace nyx
