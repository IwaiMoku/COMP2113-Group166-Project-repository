# Learning Materials - Smart Pointers, RAII and Runtime Polymorphism

This document is the group's self-study notes for the non-syllabus C++ topics
used in **STARFALL: The Nyx-9 Incident**. It explains the concepts from first
principles, shows exactly where each one lives in the project, and lists the
pitfalls we hit while integrating them.

Primary topic: **Smart pointers and RAII** (spec section 3.2, bullet 2).
Secondary topic: **Object-oriented polymorphism** (spec section 3.2, bullet 1).

---

## 1. Why raw `new`/`delete` is not enough

The course introduces `new` and `delete`. The problem is that every `new` must
be matched by exactly one `delete` on **every** path out of a function,
including early `return`s, exceptions, and loops. A single missed `delete`
leaks; a single extra `delete` is undefined behaviour.

```cpp
// Fragile: leaks if anything between new and delete returns early.
Item* item = new Item(registry.find("medkit"));
if (!player.isAlive()) {
    return;              // <- leak
}
player.take(item);
delete item;
```

The C++ solution is **RAII** - *Resource Acquisition Is Initialisation*. Tie
the resource's lifetime to an object's lifetime: the object's destructor
releases it, and the compiler guarantees the destructor runs when the object
goes out of scope, including during stack unwinding.

---

## 2. `std::unique_ptr`: sole ownership

`std::unique_ptr<T>` is a move-only smart pointer. It owns one object and
deletes it in its destructor. Copying is disabled at compile time, which is
exactly the semantics we want for "the inventory owns this item".

From `item.h`:

```cpp
class Inventory {
public:
    void add(const Item& item);
private:
    std::vector<std::unique_ptr<Item> > items_;   // owns every item
};
```

```cpp
void Inventory::add(const Item& item) {
    items_.push_back(std::unique_ptr<Item>(new Item(item)));
}
```

Three lessons are visible here:

1. `push_back` of a `unique_ptr` performs a **move**, not a copy.
2. The vector's destructor destroys each `unique_ptr`, which deletes each
   `Item`. `Inventory::~Inventory()` therefore has an empty body - there is
   nothing left to free by hand.
3. Because ownership is unique, `Inventory` is not copyable by default. We
   wanted value semantics for the player's pouch, so we wrote a **deep copy**
   explicitly:

```cpp
Inventory::Inventory(const Inventory& other) : items_() {
    for (std::size_t i = 0; i < other.items_.size(); ++i) {
        items_.push_back(std::unique_ptr<Item>(new Item(*other.items_[i])));
    }
}
```

The same pattern drives the other two owners in the project:

* `Game` owns the player: `std::unique_ptr<Player> player_;` - recreated by
  `player_.reset(new Player("Rook"))` when a new game starts or a save loads.
* `Combat` owns the enemy it fights, injected through the constructor:

```cpp
Combat combat(*player_, std::move(enemy), items_, rng_, log_, io_);
```

  `std::move` is a cast to an rvalue reference; it tells the compiler "you may
  steal this pointer". After the call, `enemy` is empty, so the encounter
  cannot outlive its own fight, and there is no way to keep using a dead
  enemy through a stale pointer.

### Pitfall: `std::unique_ptr` in C++11

`std::make_unique` is **C++14**. Under `-std=c++11` it does not exist, so the
project always writes `std::unique_ptr<T>(new T(...))`. This is the single
most common compile error when copying modern examples from the web.

### Pitfall: ownership must be single

`Combat` takes the enemy by value and moves it in. If it had taken
`Enemy*` and also been given the room's pointer, both the room and the combat
object would believe they own it - a double delete waiting to happen. The
`unique_ptr` signature makes the transfer of ownership part of the API.

---

## 3. RAII without smart pointers: the rule of three

Smart pointers are one form of RAII. A class that manages a raw resource
itself must implement three functions, or the compiler-generated ones will
copy the *pointer* instead of the *resource*:

1. destructor (release),
2. copy constructor (deep copy),
3. copy assignment (release, then deep copy).

`DeckGrid` in `core.h`/`core.cpp` is the project's example: it owns a
`char**` grid allocated with `new[]`.

```cpp
DeckGrid::DeckGrid(int rows, int cols, char fill) : cells_(0), rows_(0), cols_(0) {
    allocate(rows, cols, fill);
}

DeckGrid::~DeckGrid() { release(); }

DeckGrid& DeckGrid::operator=(const DeckGrid& other) {
    if (this != &other) {       // self-assignment guard
        release();
        allocate(other.rows_, other.cols_, ' ');
        for (int r = 0; r < rows_; ++r)
            for (int c = 0; c < cols_; ++c)
                cells_[r][c] = other.cells_[r][c];
    }
    return *this;
}
```

Without the copy constructor, `DeckGrid a = b;` would copy the `char**`
address, and both destructors would `delete[]` the same block.

The same reasoning applies to `LogList`, a hand-written singly linked list of
journal entries:

```cpp
LogList::~LogList() {
    Node* current = head_;
    while (current != 0) {
        Node* next = current->next;
        delete current;
        current = next;
    }
}
```

Copying is *disabled* (private, undefined copy constructor and assignment)
because two logs sharing nodes would double-free. This is the C++03 idiom for
what `= delete` expresses in C++11.

---

## 4. Runtime polymorphism

`Entity` is an abstract base class: it declares behaviour without defining it.

```cpp
class Entity {
public:
    virtual ~Entity();
    virtual std::string kind() const = 0;
    virtual std::string battleCry() const = 0;
    virtual std::string special(Player& target, Random& rng);
    ...
};
```

* `= 0` makes the function **pure virtual**; `Entity` cannot be instantiated.
* The **virtual destructor** is mandatory: `delete` through an `Entity*`
  (which is what `unique_ptr<Entity>` does) must run the derived destructor.
* `special()` has a default implementation, so enemies that do not override it
  still behave sensibly.

Six classes derive from `Enemy`, each overriding `kind()`, `battleCry()` and
`special()`:

| Class | Enemy | Special move |
|-------|-------|--------------|
| `DroneUnit` | Maintenance Drone | twin pulse (two hits) |
| `SentryUnit` | Sentry Drone | suppressive burst |
| `HoundUnit` | Spore Hound | acid spray |
| `WraithUnit` | Overload Wraith | drain (heals itself) |
| `WardenUnit` | Warden Prime | gravity slam, stronger below 35% HP |
| `OverseerUnit` | Nyx Warden | plasma vent |

The combat loop never mentions a concrete class:

```cpp
std::string text = enemy_->special(player_, rng_);
```

The call is resolved at run time from the object's dynamic type, so adding a
seventh enemy only means adding one class and one line in the factory.

### The factory

`makeEnemy` maps a data-driven template to the right concrete type:

```cpp
std::unique_ptr<Enemy> makeEnemy(const EnemyTemplate& tmpl) {
    switch (tmpl.kind) {
        case ENEMY_DRONE:  return std::unique_ptr<Enemy>(new DroneUnit(tmpl));
        case ENEMY_SENTRY: return std::unique_ptr<Enemy>(new SentryUnit(tmpl));
        ...
    }
}
```

Returning `unique_ptr<Enemy>` (not `Enemy*`) means the caller cannot forget to
delete, and returning a derived `unique_ptr` converts to the base automatically.

### Why virtual dispatch matters here

Enemy statistics live in `data/enemies.txt`; behaviour lives in C++. Balancing
the game is a text edit; adding behaviour is a class. Neither requires touching
the other, which is the point of separating data from behaviour.

---

## 5. Custom hash for `std::unordered_map`

`std::unordered_map` needs a hash function. `std::hash<std::string>` exists,
but the specification asks for a user-supplied hash, so the registry uses
FNV-1a:

```cpp
struct StringHash {
    std::size_t operator()(const std::string& key) const {
        std::size_t hash = static_cast<std::size_t>(1469598103934665603ULL);
        for (std::size_t i = 0; i < key.size(); ++i) {
            hash ^= static_cast<std::size_t>(static_cast<unsigned char>(key[i]));
            hash *= static_cast<std::size_t>(1099511628211ULL);
        }
        return hash;
    }
};

std::unordered_map<std::string, Item, StringHash> items_;
```

FNV-1a is: start from an offset basis, XOR each byte, multiply by a prime.
It is not cryptographic, but it is fast, simple and spreads short ASCII keys
well. The functor must be `const`-callable and return `std::size_t`.

Note the `static_cast<unsigned char>` before hashing: `char` may be signed, and
hashing negative values is implementation-defined.

Because the hash is used, `ItemRegistry::find` is average O(1), which matters
when every room description and every `take` command looks items up by id.

---

## 6. Where the standard containers fit

| Container | Use in the project | Why |
|-----------|--------------------|-----|
| `std::vector` | rooms, inventory, journal results, id lists | contiguous, cache-friendly, index access |
| `std::map` | world flags, enemy templates | ordered, stable iteration, `find` is O(log n) |
| `std::unordered_map` | item registry | O(1) average lookup by id |
| `std::queue` | ship transmissions | FIFO: messages arrive and are shown in order |
| `std::stack` | command history | LIFO: the most recent command is the interesting one |
| linked list | journal (`LogList`) | O(1) append, no reallocation, demonstrates manual nodes |

The journal intentionally uses a hand-written list rather than `std::list` so
that node allocation and destruction are visible in the code.

---

## 7. Determinism as a design constraint

Randomness makes games hard to test. The project funnels **every** random
decision through one `Random` object seeded once:

```cpp
Random rng_;                        // std::mt19937 inside
rng_.chance(25);                    // rest ambush
rng_.range(0, 2);                   // damage jitter
```

`--seed 42` therefore reproduces an entire session exactly, and `--script`
replays a fixed command file. This is the mechanism the testing report relies
on, and it is why no code calls `rand()` directly.

`std::mt19937` is used instead of `rand()` because `rand()`'s algorithm is
implementation-defined: the same seed produces different numbers on different
standard libraries, which would break reproducibility across machines.

---

## 8. Self-check questions

1. Why can `std::vector<std::unique_ptr<Item>>` not be copied, and what does
   the project do instead?
2. What happens if `Entity`'s destructor is not virtual and a `DroneUnit` is
   deleted through `Entity*`?
3. Why does `DeckGrid::operator=` check `this != &other` first?
4. Why must the hash functor's `operator()` be `const`?
5. Explain why `--seed` alone is not enough for reproducible tests without a
   scripted input mode.
6. What would break if `makeEnemy` returned `Enemy*` instead of
   `std::unique_ptr<Enemy>`?

---

## 9. References used while learning

* cppreference.com - `std::unique_ptr`, `std::unordered_map`, `std::mt19937`,
  rule of three/five/zero.
* ISO C++ Core Guidelines - R.11 (avoid explicit `new`/`delete`), C.21
  (define or delete copy operations), C.35 (virtual destructor).
* Course notes, COMP2113 weeks 7-10 (classes, inheritance, dynamic memory).
