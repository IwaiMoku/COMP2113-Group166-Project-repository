#ifndef NYX_SAVE_H
#define NYX_SAVE_H

#include <string>
#include <map>

#include "entity.h"
#include "item.h"
#include "world.h"

namespace nyx {

// Everything the save file needs that is not already inside Player/World.
struct SaveMeta {
    int roomId;
    int turns;
    int flareLimit;
    unsigned int seed;
    int difficulty;
    std::map<std::string, bool> flags;

    SaveMeta();
};

// ---------------------------------------------------------------------
// SaveSystem: plain-text persistence.
//
// The format is line oriented ("key value") so that a saved file can be
// inspected and diffed by hand; room rows use '|' separators like the
// data files.
// ---------------------------------------------------------------------
class SaveSystem {
public:
    static bool save(const std::string& path, const Player& player,
                     const World& world, const SaveMeta& meta);

    static bool load(const std::string& path, Player& player, World& world,
                     SaveMeta& meta, const ItemRegistry& items);
};

} // namespace nyx

#endif // NYX_SAVE_H
