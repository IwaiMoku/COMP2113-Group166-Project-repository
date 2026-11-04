# STARFALL: The Nyx-9 Incident

A text-based science-fiction role-playing game written in C++11 for COMP2113
Programming Technologies (Group Project, 2026-27 Semester 1).

You are **Rook**, a salvage operative boarding the derelict research station
**Nyx-9** as the Vela Collapsar begins to flare. Fight through the station,
restore power, recover the **Kepler Shard** from the AI vault, and launch
before the flare consumes the hull.

---

## 1. Team

| Name | Student ID | Role |
|------|------------|------|
|      |            |      |
|      |            |      |
|      |            |      |

Video demonstration: `<paste the 2-minute video URL here>`

---

## 2. Build and run

The project is plain C++11 with **no external libraries** and builds with the
flags required by the specification.

```bash
make                 # g++ -pedantic-errors -std=c++11 -Wall -Wextra -O2 *.cpp -o main
./main               # play
make clean           # remove objects, binary and the save file
```

On Windows, `run.bat` does the same thing and then starts the game, so a
double-click is enough once `g++` is on `PATH`.

Verified build: GCC 16.2.0 (w64devkit, x64) with
`-pedantic-errors -std=c++11 -Wall -Wextra` - 0 errors, 0 warnings.

### Troubleshooting the build

| Message | Fix |
|---|---|
| `Cannot create temporary file in ...\Temp\: Permission denied` | `%TEMP%` is locked down. `run.bat` already redirects `TMP`/`TEMP`/`TMPDIR` to `build\tmp`; if you invoke `g++` by hand, run `set TMP=%CD%\build\tmp` first and create that folder. |
| `'g++' is not recognized` | No compiler on `PATH`; install w64devkit or MSYS2, or keep the portable copy in `..\.tools\w64devkit`. |
| `make: command not found` | Use the one-liner `g++ ... *.cpp -o main.exe`, or run `run.bat`. |
| Room text looks generic | The program was started outside this folder, so `data\` was not found and the built-in fallback station was used. |

The one-liner from section 7.1 of the specification also works, because every
`.cpp` and `.h` file lives in the repository root:

```bash
g++ -pedantic-errors -std=c++11 *.cpp -o main
```

Run from the repository root so that `data/items.txt`, `data/enemies.txt` and
`data/rooms.txt` are found. If the data files are missing the game falls back
to a built-in station definition, so it still runs.

### Command-line options (testability)

| Option | Effect |
|--------|--------|
| `--seed <n>` | Fixes the RNG seed, making every dice roll reproducible |
| `--script <file>` | Reads commands from a file instead of the keyboard and echoes them |
| `--load <file>` | Loads a save file at start-up |
| `--debug` | Enables debug commands (`god`, `heal`, `xp`, `tp`, `turns`, `kill`, `give`) |
| `--help` | Prints usage |

Example deterministic session:

```bash
./main --seed 42 --script tests/in/01_quit.txt
```

---

## 3. How to play

| Command | Meaning |
|---------|---------|
| `look` / `l` | Describe the current room |
| `n`, `s`, `e`, `w` | Move north / south / east / west |
| `go <direction>` | Move by name |
| `take <item>` / `take all` | Pick up items |
| `inv` / `i` | List carried items |
| `equip <item>` | Wield a weapon or wear armour |
| `use <item>` | Consume a medkit or stim |
| `attack` / `a` | Engage the hostile in this room |
| `status` / `st` | Character sheet and flare clock |
| `map` / `m` | Deck schematic (dynamic 2D array) |
| `journal` / `j` | Recent events (linked list) |
| `history` | Recent commands (stack) |
| `rest` | Recover HP; costs 3 cycles and may attract attention |
| `power` | Seat a fusion cell in the reactor console |
| `launch` | Leave the station (Docking Bay + shard required) |
| `save` / `load` | Write / read `nyx9_save.txt` |
| `quit` | Save and exit |

In combat: `attack`, `defend`, `use <item>`, `scan`, `flee`, `help`.

### Winning and losing

* **Win** - take the Kepler Shard, defeat the Nyx Warden in the Docking Bay,
  then `launch`.
* **Lose** - HP reaches 0, or the flare clock reaches its limit. The counter
  advances on every move, every combat round, every item use and every rest.

### Walkthrough (spoilers)

1. North to the **Main Corridor** - a maintenance drone attacks.
2. West to the **Armoury** - beat the sentry, take the plasma cutter, needler,
   riot plating and the **security keycard**.
3. East to the **Medbay** for medkits; `equip` the best weapon and armour.
4. North to **Hydroponics** (spore hound), then north to **Reactor Access**
   (the keycard opens the bulkhead).
5. North to the **Reactor Core** - beat the wraith, take the fusion cell and
   type `power`. This unlocks the vault and grants +5 max HP.
6. Back south twice, then east to the **Server Vault** - defeat Warden Prime,
   take the shard (and the nanoforge weave).
7. Return to the **Docking Bay**; the Nyx Warden is waiting. Kill it and
   `launch`.

---

## 4. Mapping to the mandatory requirements

| Requirement (spec 3.1) | Where it is implemented |
|---|---|
| **Non-syllabus C++ topic** | *Smart pointers and RAII* (spec 3.2, bullet 2): `Inventory` owns `std::vector<std::unique_ptr<Item>>` (`item.h`), `Game` owns `std::unique_ptr<Player>`, `Combat` owns `std::unique_ptr<Enemy>`, and enemies are transferred with `std::move` (`combat.cpp`). *Runtime polymorphism* (spec 3.2, bullet 1) is the second topic: abstract `Entity` with pure virtual `kind()` / `battleCry()` / `special()`, overridden by `Player` and six enemy classes (`entity.h` / `entity.cpp`). |
| **Structured data organisation** | `std::vector<Room>` graph (`world.h`), `std::unordered_map` item registry with a **custom FNV-1a hash** (`item.h`), `std::map` for flags and the enemy book, `std::queue` for ship transmissions, `std::stack` for command history, plus a hand-written singly linked list `LogList` (`core.h`). |
| **Dynamic memory management** | `DeckGrid` allocates a `char**` grid with `new[]` / `delete[]` and implements the rule of three (`core.cpp`); `LogList` allocates and frees `Node` objects with `new` / `delete`; `unique_ptr` provides automatic lifetime management everywhere else. |
| **File I/O** | `data/items.txt`, `data/enemies.txt`, `data/rooms.txt` are parsed at start-up (`ItemRegistry::loadFromFile`, `EnemyBook::loadFromFile`, `World::loadFromFile`); `SaveSystem::save/load` writes and reads `nyx9_save.txt`, and `quit` saves before exiting. |
| **Multi-file architecture** | 7 headers (`core.h`, `item.h`, `entity.h`, `world.h`, `combat.h`, `save.h`, `game.h`) + 8 implementation files (`.cpp`) + `Makefile`. Headers declare interfaces; `.cpp` files define them. |
| **Modular functions / menus / modes** | Title menu, difficulty menu, combat menu, 20+ command verbs, three difficulty levels, save/continue, `--script` and `--debug` operating modes. |
| **Design for testability** (spec 3.3) | `--seed` fixes the RNG; `--script` replays a command file deterministically and echoes input; `--debug` exposes state-changing commands; the save file is human-readable text. |

Header-only libraries: **none** - the project is self-contained, so nothing has
to be installed on the CS Academy server.

---

## 5. Repository layout

```
.
|-- main.cpp            argument parsing, owns the Game object
|-- game.h / game.cpp   menus, command loop, rules, save orchestration
|-- combat.h / .cpp     turn-based combat, polymorphic enemy turns
|-- entity.h / .cpp     Entity / Player / six Enemy subclasses / factory
|-- item.h / item.cpp   Item, ItemRegistry (unordered_map + custom hash), Inventory
|-- world.h / world.cpp Room graph, directions, data-file loading
|-- save.h / save.cpp   text persistence for player, world and flags
|-- core.h / core.cpp   strings, RNG, Io, LogList (linked list), DeckGrid (2D array)
|-- data/               items.txt, enemies.txt, rooms.txt
|-- tests/              scripted command files for deterministic runs
|-- Makefile
|-- README.md
|-- LEARNING_MATERIALS.md
|-- TESTING_REPORT.md
`-- AI_USAGE.md
```

### Data formats

Items (`data/items.txt`):

```
id|name|type|power|price|description
medkit|Medkit|consumable|25|30|Restores 25 HP.
```

Enemies (`data/enemies.txt`):

```
id|name|kind|hp|attack|defense|xp|credits|special%
sentry_drone|Sentry Drone|sentry|26|6|3|25|20|30
```

Rooms (`data/rooms.txt`):

```
id|name|description|north|east|south|west|items|enemy|lock
1|Main Corridor|...|3|2|0|4|-|maintenance_drone|-
```

`lock` is either an item id (`keycard`) or a world flag (`power_restored`);
`-` means the door is open. Room ids must be `0..N-1` in order because exits
are indices into the room vector.

---

## 6. Game systems in brief

* **Stats** - HP, ATK, DEF, level, XP, credits. Levelling up adds +8 max HP,
  +2 ATK, +1 DEF and fully heals.
* **Combat** - player and enemy alternate; damage is
  `attack + 0..2 - defence`, minimum 1, with a 12% critical chance. Each enemy
  kind has its own special move (twin pulse, suppressive burst, acid spray,
  drain, gravity slam, plasma vent).
* **The clock** - the station has a fixed number of cycles before the flare.
  Moving, fighting, using items and resting all consume cycles, so long fights
  are expensive.
* **Difficulty** - Survivor (80% enemy HP, 75% damage, 110 cycles), Operative
  (100%, 80 cycles), Ghost (125% HP, 130% damage, 65 cycles).
* **Persistence** - the save file stores the character, every room's visited
  flag, remaining items and enemies, world flags, turn count and seed.

---

## 7. Known limitations

* Single-player, terminal only; no graphical output by design.
* The map is a fixed eight-room station (data-driven, but the bundled room
  file is the only campaign).
* Save files are versioned by a single header line (`NYX9SAVE 1`); there is no
  migration for older formats.
