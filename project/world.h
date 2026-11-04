#ifndef NYX_WORLD_H
#define NYX_WORLD_H

#include <string>
#include <vector>
#include <cstddef>

namespace nyx {

// Exits are stored as {north, east, south, west}; -1 means "no door".
const int DIR_NORTH = 0;
const int DIR_EAST = 1;
const int DIR_SOUTH = 2;
const int DIR_WEST = 3;

struct Room {
    int id;
    std::string name;
    std::string description;
    int exits[4];
    std::vector<std::string> items;  // ids still lying on the deck
    std::string enemyId;             // empty when the room is clear
    std::string lock;                // required item id or world flag ("" = open)
    bool visited;

    Room();
};

// ---------------------------------------------------------------------
// World: the station graph.
//
// Rooms are stored in a std::vector and linked by integer indices, so the
// map is a small adjacency-list graph rather than a 2D array.
// ---------------------------------------------------------------------
class World {
public:
    World();

    bool loadFromFile(const std::string& path);
    void buildDefault();

    Room& room(int index);
    const Room& room(int index) const;
    int roomCount() const;
    int startRoom() const;
    bool validRoom(int index) const;

    static std::string directionName(int dir);
    static int directionIndex(const std::string& text);
    static const char* directionLetter(int dir);

private:
    std::vector<Room> rooms_;
    int start_;
};

} // namespace nyx

#endif // NYX_WORLD_H
