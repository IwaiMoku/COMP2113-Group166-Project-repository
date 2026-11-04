#ifndef NYX_ITEM_H
#define NYX_ITEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstddef>

namespace nyx {

enum ItemType {
    ITEM_WEAPON,
    ITEM_ARMOR,
    ITEM_CONSUMABLE,
    ITEM_KEY,
    ITEM_ARTIFACT,
    ITEM_MISC,
    ITEM_UNKNOWN
};

std::string itemTypeName(ItemType type);
ItemType parseItemType(const std::string& text);

// ---------------------------------------------------------------------
// A single item. Plain value type: the registry stores copies and the
// inventory stores heap-allocated copies it owns.
// ---------------------------------------------------------------------
struct Item {
    std::string id;
    std::string name;
    ItemType type;
    int power;   // weapon: attack bonus, armour: defence bonus, consumable: heal
    int price;
    std::string description;

    Item();
    Item(const std::string& itemId, const std::string& itemName, ItemType itemType,
         int itemPower, int itemPrice, const std::string& itemDescription);
};

// ---------------------------------------------------------------------
// Custom hash functor (FNV-1a).
//
// Used as the third template argument of the unordered_map below, so the
// registry demonstrates a user-supplied hash rather than std::hash.
// ---------------------------------------------------------------------
struct StringHash {
    std::size_t operator()(const std::string& key) const;
};

// ---------------------------------------------------------------------
// ItemRegistry: id -> Item lookup, loaded from data/items.txt.
// ---------------------------------------------------------------------
class ItemRegistry {
public:
    ItemRegistry();

    bool loadFromFile(const std::string& path);
    void addDefaultItems();

    const Item* find(const std::string& id) const;
    std::vector<std::string> allIds() const;
    std::size_t size() const;

private:
    std::unordered_map<std::string, Item, StringHash> items_;
};

// ---------------------------------------------------------------------
// Inventory: owns heap-allocated Item copies through std::unique_ptr.
//
// unique_ptr is move-only, so the vector cannot be copied implicitly;
// the copy constructor and assignment below perform an explicit deep
// copy (a small RAII exercise).
// ---------------------------------------------------------------------
class Inventory {
public:
    Inventory();
    Inventory(const Inventory& other);
    Inventory& operator=(const Inventory& other);
    ~Inventory();

    void add(const Item& item);
    bool remove(const std::string& id);
    void clear();

    const Item* find(const std::string& id) const;
    bool has(const std::string& id) const;
    int count() const;
    bool empty() const;

    // Non-owning view of the contents; valid until the inventory changes.
    std::vector<const Item*> list() const;

private:
    std::vector<std::unique_ptr<Item> > items_;
};

} // namespace nyx

#endif // NYX_ITEM_H
