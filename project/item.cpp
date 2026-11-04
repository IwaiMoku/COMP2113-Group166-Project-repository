#include "item.h"
#include "core.h"

#include <fstream>

namespace nyx {

std::string itemTypeName(ItemType type) {
    switch (type) {
        case ITEM_WEAPON:     return "weapon";
        case ITEM_ARMOR:      return "armor";
        case ITEM_CONSUMABLE: return "consumable";
        case ITEM_KEY:        return "key";
        case ITEM_ARTIFACT:   return "artifact";
        case ITEM_MISC:       return "misc";
        default:              return "unknown";
    }
}

ItemType parseItemType(const std::string& text) {
    std::string t = toLower(trim(text));
    if (t == "weapon")     return ITEM_WEAPON;
    if (t == "armor")      return ITEM_ARMOR;
    if (t == "armour")     return ITEM_ARMOR;
    if (t == "consumable") return ITEM_CONSUMABLE;
    if (t == "key")        return ITEM_KEY;
    if (t == "artifact")   return ITEM_ARTIFACT;
    if (t == "misc")       return ITEM_MISC;
    return ITEM_UNKNOWN;
}

Item::Item()
    : id(), name(), type(ITEM_MISC), power(0), price(0), description() {}

Item::Item(const std::string& itemId, const std::string& itemName, ItemType itemType,
           int itemPower, int itemPrice, const std::string& itemDescription)
    : id(itemId), name(itemName), type(itemType), power(itemPower),
      price(itemPrice), description(itemDescription) {}

std::size_t StringHash::operator()(const std::string& key) const {
    std::size_t hash = static_cast<std::size_t>(1469598103934665603ULL);
    for (std::size_t i = 0; i < key.size(); ++i) {
        hash ^= static_cast<std::size_t>(static_cast<unsigned char>(key[i]));
        hash *= static_cast<std::size_t>(1099511628211ULL);
    }
    return hash;
}

// ------------------------------------------------------------ ItemRegistry

ItemRegistry::ItemRegistry() : items_() {}

bool ItemRegistry::loadFromFile(const std::string& path) {
    std::ifstream in(path.c_str());
    if (!in) {
        return false;
    }

    std::string line;
    bool loadedAny = false;
    while (std::getline(in, line)) {
        std::string cleaned = trim(line);
        if (cleaned.empty() || cleaned[0] == '#') {
            continue;
        }
        std::vector<std::string> fields = split(cleaned, '|');
        if (fields.size() < 6) {
            continue;
        }
        Item item(trim(fields[0]), trim(fields[1]), parseItemType(fields[2]),
                  toInt(trim(fields[3]), 0), toInt(trim(fields[4]), 0), trim(fields[5]));
        if (item.id.empty()) {
            continue;
        }
        items_[item.id] = item;
        loadedAny = true;
    }
    return loadedAny;
}

void ItemRegistry::addDefaultItems() {
    // Minimal fallback set: enough to finish the game if data/ is missing.
    items_["medkit"] = Item("medkit", "Medkit", ITEM_CONSUMABLE, 25, 30,
                            "A pressurised dermal patch. Restores 25 HP.");
    items_["wrench"] = Item("wrench", "Salvage Wrench", ITEM_WEAPON, 3, 20,
                            "Heavy maintenance tool. Better than bare fists.");
    items_["plasma_cutter"] = Item("plasma_cutter", "Plasma Cutter", ITEM_WEAPON, 7, 120,
                                   "Cuts bulkheads - and drones - with equal ease.");
    items_["riot_plating"] = Item("riot_plating", "Riot Plating", ITEM_ARMOR, 4, 140,
                                  "Ceramic plates stripped from station security.");
    items_["keycard"] = Item("keycard", "Security Keycard", ITEM_KEY, 0, 0,
                             "Level-3 clearance. Opens the reactor bulkhead.");
    items_["fusion_cell"] = Item("fusion_cell", "Fusion Cell", ITEM_MISC, 0, 0,
                                 "A charged cell for the reactor console.");
    items_["shard"] = Item("shard", "Kepler Shard", ITEM_ARTIFACT, 0, 0,
                           "The station AI's core fragment. It hums against your palm.");
}

const Item* ItemRegistry::find(const std::string& id) const {
    std::unordered_map<std::string, Item, StringHash>::const_iterator it = items_.find(id);
    if (it == items_.end()) {
        return 0;
    }
    return &(it->second);
}

std::vector<std::string> ItemRegistry::allIds() const {
    std::vector<std::string> ids;
    std::unordered_map<std::string, Item, StringHash>::const_iterator it = items_.begin();
    for (; it != items_.end(); ++it) {
        ids.push_back(it->first);
    }
    return ids;
}

std::size_t ItemRegistry::size() const {
    return items_.size();
}

// ------------------------------------------------------------ Inventory

Inventory::Inventory() : items_() {}

Inventory::Inventory(const Inventory& other) : items_() {
    for (std::size_t i = 0; i < other.items_.size(); ++i) {
        items_.push_back(std::unique_ptr<Item>(new Item(*other.items_[i])));
    }
}

Inventory& Inventory::operator=(const Inventory& other) {
    if (this != &other) {
        items_.clear();
        for (std::size_t i = 0; i < other.items_.size(); ++i) {
            items_.push_back(std::unique_ptr<Item>(new Item(*other.items_[i])));
        }
    }
    return *this;
}

Inventory::~Inventory() {
    // std::unique_ptr members delete their items automatically.
}

void Inventory::add(const Item& item) {
    items_.push_back(std::unique_ptr<Item>(new Item(item)));
}

bool Inventory::remove(const std::string& id) {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i]->id == id) {
            items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

void Inventory::clear() {
    items_.clear();
}

const Item* Inventory::find(const std::string& id) const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i]->id == id) {
            return items_[i].get();
        }
    }
    return 0;
}

bool Inventory::has(const std::string& id) const {
    return find(id) != 0;
}

int Inventory::count() const {
    return static_cast<int>(items_.size());
}

bool Inventory::empty() const {
    return items_.empty();
}

std::vector<const Item*> Inventory::list() const {
    std::vector<const Item*> out;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        out.push_back(items_[i].get());
    }
    return out;
}

} // namespace nyx
