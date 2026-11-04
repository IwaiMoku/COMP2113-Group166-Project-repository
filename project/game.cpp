#include "game.h"
#include "combat.h"
#include "save.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>

namespace nyx {

namespace {

// Schematic coordinates of each room on the 5x4 deck grid.
const int kGridRows = 5;
const int kGridCols = 4;
const int kRoomRow[8] = {4, 3, 3, 2, 3, 1, 0, 2};
const int kRoomCol[8] = {2, 2, 3, 2, 1, 2, 2, 3};

const char* kSaveFile = "nyx9_save.txt";

} // namespace

Options::Options()
    : useSeed(false), seed(1), debug(false), scriptPath(), loadPath(),
      difficulty(DIFF_NORMAL) {}

Game::Game(const Options& options, std::istream& in)
    : options_(options),
      io_(in),
      seed_(options.useSeed ? options.seed : static_cast<unsigned int>(std::time(0))),
      rng_(seed_),
      items_(),
      enemies_(),
      world_(),
      player_(new Player("Rook")),
      log_(),
      flags_(),
      history_(),
      transmissions_(),
      roomId_(0),
      previousRoom_(0),
      turns_(0),
      flareLimit_(70),
      difficulty_(options.difficulty),
      running_(true),
      finished_(false) {
    options_.seed = seed_;
    io_.setEcho(!options_.scriptPath.empty());

    if (!items_.loadFromFile("data/items.txt")) {
        items_.addDefaultItems();
    }
    if (!enemies_.loadFromFile("data/enemies.txt")) {
        enemies_.addDefaults();
    }
    if (!world_.loadFromFile("data/rooms.txt")) {
        world_.buildDefault();
    }
}

Game::~Game() {}

// ---------------------------------------------------------------- title

void Game::printTitle() const {
    std::cout << "\n";
    std::cout << "  ############################################################\n";
    std::cout << "  #                                                          #\n";
    std::cout << "  #      S T A R F A L L   :   T H E   N Y X - 9   I N C I D E N T\n";
    std::cout << "  #                                                          #\n";
    std::cout << "  #   A text-based salvage RPG.  Year 2387.  Vela Collapsar.  #\n";
    std::cout << "  #                                                          #\n";
    std::cout << "  ############################################################\n";
    std::cout << "\n  The research station Nyx-9 has been dark for eleven days.\n";
    std::cout << "  You are Rook, the only salvager close enough to answer.\n";
    std::cout << "  The star will flare in a few hours. Make them count.\n";
}

bool Game::saveExists() const {
    std::ifstream in(kSaveFile);
    return in.good();
}

bool Game::startMenu() {
    std::cout << "\n  1) New game\n";
    std::cout << "  2) Continue" << (saveExists() ? "" : "   (no save found)") << "\n";
    std::cout << "  3) Quit\n";

    std::string line = io_.readLine("\n  choice> ");
    std::string choice = trim(line);

    if (choice == "3" || choice == "q" || choice == "quit") {
        return false;
    }
    if (choice == "2") {
        bool loaded = false;
        if (!options_.loadPath.empty()) {
            loaded = loadGame(options_.loadPath);
        }
        if (!loaded) {
            loaded = loadGame(kSaveFile);
        }
        if (loaded) {
            return true;
        }
        std::cout << "  No usable save file. Starting a new game instead.\n";
    }

    chooseDifficulty();
    newGame();
    return true;
}

void Game::chooseDifficulty() {
    std::cout << "\n  Select difficulty:\n";
    std::cout << "    1) Survivor  - softer enemies, longer flare clock\n";
    std::cout << "    2) Operative - the station as it really is\n";
    std::cout << "    3) Ghost     - harder enemies, shorter clock\n";

    std::string line = io_.readLine("\n  difficulty> ");
    int choice = toInt(trim(line), 2);
    if (choice < 1 || choice > 3) {
        choice = 2;
    }
    difficulty_ = choice - 1;
}

void Game::newGame() {
    player_.reset(new Player("Rook"));
    flags_.clear();
    while (!history_.empty()) {
        history_.pop();
    }
    while (!transmissions_.empty()) {
        transmissions_.pop();
    }

    turns_ = 0;
    flareLimit_ = (difficulty_ == DIFF_EASY) ? 110 : (difficulty_ == DIFF_HARD ? 65 : 80);

    const Item* wrench = items_.find("wrench");
    if (wrench != 0) {
        player_->inventory().add(*wrench);
        player_->equipWeapon("wrench");
    }

    int medkits = (difficulty_ == DIFF_EASY) ? 3 : (difficulty_ == DIFF_HARD ? 1 : 2);
    const Item* medkit = items_.find("medkit");
    if (medkit != 0) {
        for (int i = 0; i < medkits; ++i) {
            player_->inventory().add(*medkit);
        }
    }

    roomId_ = world_.startRoom();
    previousRoom_ = roomId_;
    running_ = true;
    finished_ = false;

    std::cout << "\n  Suit sealed. " << medkits << " medkit(s) in the pouch, salvage wrench on"
              << " your belt.\n";
    std::cout << "  Type 'help' for commands.\n";

    addTransmission("KESTREL: Nyx-9 reads as cold. Find the Kepler Shard and get back before the flare.");
    enterRoom();
}

bool Game::loadGame(const std::string& path) {
    SaveMeta meta;
    if (!SaveSystem::load(path, *player_, world_, meta, items_)) {
        return false;
    }

    flags_ = meta.flags;
    turns_ = meta.turns;
    flareLimit_ = meta.flareLimit;
    difficulty_ = meta.difficulty;
    seed_ = meta.seed;
    rng_ = Random(seed_);
    roomId_ = meta.roomId;
    previousRoom_ = roomId_;
    running_ = true;
    finished_ = false;

    std::cout << "  Save loaded: " << player_->name() << ", level " << player_->level()
              << ", " << player_->hp() << "/" << player_->maxHp() << " HP.\n";
    enterRoom();
    return true;
}

// ---------------------------------------------------------------- loop

int Game::run() {
    printTitle();
    if (!startMenu()) {
        std::cout << "\n  You leave Nyx-9 to the dark.\n";
        return 0;
    }
    mainLoop();
    return 0;
}

void Game::mainLoop() {
    while (running_ && !finished_) {
        if (io_.eof()) {
            break;
        }
        std::string line = io_.readLine("\n> ");
        if (trim(line).empty()) {
            if (io_.eof()) {
                break;
            }
            continue;
        }
        processCommand(line);
    }
}

void Game::processCommand(const std::string& raw) {
    std::string line = trim(raw);
    if (line.empty()) {
        return;
    }
    history_.push(line);

    std::string lower = toLower(line);
    std::string verb = lower;
    std::string argument;
    std::size_t space = lower.find(' ');
    if (space != std::string::npos) {
        verb = lower.substr(0, space);
        argument = trim(lower.substr(space + 1));
    }

    if (verb == "help" || verb == "?") {
        showHelp();
    } else if (verb == "look" || verb == "l") {
        look();
    } else if (verb == "status" || verb == "st" || verb == "sheet") {
        showStatus();
    } else if (verb == "inv" || verb == "inventory" || verb == "i") {
        showInventory();
    } else if (verb == "map" || verb == "m") {
        showMap();
    } else if (verb == "journal" || verb == "log" || verb == "j") {
        showJournal();
    } else if (verb == "history") {
        showHistory();
    } else if (verb == "take" || verb == "get" || verb == "grab") {
        takeItem(argument);
    } else if (verb == "use") {
        useItem(argument);
    } else if (verb == "equip" || verb == "wield" || verb == "wear") {
        equipItem(argument);
    } else if (verb == "attack" || verb == "fight" || verb == "a") {
        startCombat();
    } else if (verb == "rest" || verb == "sleep") {
        restHere();
    } else if (verb == "save") {
        saveGame();
    } else if (verb == "load") {
        std::string path = options_.loadPath.empty() ? std::string(kSaveFile) : options_.loadPath;
        if (!loadGame(path)) {
            std::cout << "  No save file to load.\n";
        }
    } else if (verb == "go") {
        handleMovement(argument);
    } else if (World::directionIndex(verb) >= 0) {
        handleMovement(verb);
    } else if (verb == "power" || verb == "console") {
        restorePower();
    } else if (verb == "launch" || verb == "escape") {
        tryLaunch();
    } else if (verb == "quit" || verb == "exit" || verb == "q") {
        saveGame();
        std::cout << "  Session closed.\n";
        running_ = false;
    } else if (options_.debug &&
               (verb == "god" || verb == "heal" || verb == "xp" || verb == "tp" ||
                verb == "turns" || verb == "kill" || verb == "give")) {
        runDebug(verb, argument);
    } else {
        std::cout << "  The suit computer does not recognise '" << verb << "'. Try 'help'.\n";
    }

    flushTransmissions();
}

// ---------------------------------------------------------------- views

void Game::describe(const Room& room) const {
    std::cout << "\n  == " << room.name << " ==\n";
    std::cout << "  " << room.description << "\n";

    std::string exits;
    for (int d = 0; d < 4; ++d) {
        if (room.exits[d] >= 0) {
            if (!exits.empty()) {
                exits += ", ";
            }
            exits += World::directionName(d);
            if (!unlocked(world_.room(room.exits[d]).lock) && !world_.room(room.exits[d]).lock.empty()) {
                exits += " (sealed)";
            }
        }
    }
    std::cout << "  Exits: " << (exits.empty() ? std::string("none") : exits) << "\n";

    if (!room.items.empty()) {
        std::cout << "  On the deck: " << describeItemList(room.items) << "\n";
    }
    if (!room.enemyId.empty()) {
        std::cout << "  Hostile: something is moving here. ('attack')\n";
    }
}

void Game::look() const {
    describe(world_.room(roomId_));
}

std::string Game::describeItemList(const std::vector<std::string>& ids) const {
    std::string out;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const Item* item = items_.find(ids[i]);
        std::string label = (item != 0) ? item->name : ids[i];
        if (!out.empty()) {
            out += ", ";
        }
        out += label;
    }
    return out;
}

void Game::showStatus() const {
    std::cout << "\n  === " << player_->name() << ", " << player_->kind() << " ===\n";
    std::cout << "  Level " << player_->level() << "   XP " << player_->xp() << "/"
              << player_->xpNeeded() << "   Credits " << player_->credits() << "\n";
    std::cout << "  HP " << player_->hp() << "/" << player_->maxHp()
              << "   ATK " << player_->totalAttack(items_)
              << "   DEF " << player_->totalDefense(items_) << "\n";

    const Item* weapon = items_.find(player_->weaponId());
    const Item* armor = items_.find(player_->armorId());
    std::cout << "  Weapon: " << (weapon != 0 ? weapon->name : "bare hands")
              << "   Armour: " << (armor != 0 ? armor->name : "none") << "\n";

    int left = flareLimit_ - turns_;
    if (left < 0) {
        left = 0;
    }
    std::cout << "  Flare in " << left << " cycles   (turn " << turns_ << "/" << flareLimit_
              << ")\n";
}

void Game::showInventory() const {
    std::vector<const Item*> items = player_->inventory().list();
    std::cout << "\n  === Pouch (" << items.size() << " items) ===\n";
    if (items.empty()) {
        std::cout << "  Empty.\n";
        return;
    }
    for (std::size_t i = 0; i < items.size(); ++i) {
        std::cout << "  - " << items[i]->name << " [" << itemTypeName(items[i]->type) << "]";
        if (items[i]->power != 0) {
            std::cout << " +" << items[i]->power;
        }
        std::cout << "\n";
    }
}

void Game::showMap() const {
    DeckGrid grid(kGridRows, kGridCols, '.');
    for (int i = 0; i < world_.roomCount() && i < 8; ++i) {
        char mark = world_.room(i).visited ? static_cast<char>('0' + i) : '?';
        if (i == roomId_) {
            mark = '*';
        }
        grid.set(kRoomRow[i], kRoomCol[i], mark);
    }
    std::cout << "\n  --- Nyx-9 deck schematic ---\n";
    std::string rendered = grid.render();
    std::istringstream lines(rendered);
    std::string row;
    while (std::getline(lines, row)) {
        std::cout << "    " << row << "\n";
    }
    std::cout << "  * = you   digit = visited   ? = unexplored\n";
    std::cout << "  0 Docking Bay   1 Main Corridor   2 Medbay   3 Hydroponics\n";
    std::cout << "  4 Armoury       5 Reactor Access   6 Reactor Core   7 Server Vault\n";
}

void Game::showJournal() const {
    std::vector<std::string> lines = log_.recent(10);
    std::cout << "\n  === Journal (last " << lines.size() << " entries) ===\n";
    if (lines.empty()) {
        std::cout << "  Nothing recorded yet.\n";
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::cout << "  " << lines[i] << "\n";
    }
}

void Game::showHistory() const {
    std::stack<std::string> copy = history_;
    std::vector<std::string> lines;
    while (!copy.empty()) {
        lines.push_back(copy.top());
        copy.pop();
    }
    std::cout << "\n  === Command history ===\n";
    std::size_t start = 0;
    if (lines.size() > 10) {
        start = lines.size() - 10;
    }
    for (std::size_t i = lines.size(); i > start; --i) {
        std::cout << "  " << lines[i - 1] << "\n";
    }
}

void Game::showHelp() const {
    std::cout << "\n  === Commands ===\n";
    std::cout << "  look / l              describe the current room\n";
    std::cout << "  n, s, e, w            move north / south / east / west\n";
    std::cout << "  go <direction>        move by name\n";
    std::cout << "  take <item> | take all\n";
    std::cout << "  inv / i               list carried items\n";
    std::cout << "  equip <item>          wield a weapon or wear armour\n";
    std::cout << "  use <item>            consume a medkit or stim\n";
    std::cout << "  attack / a            engage anything hostile here\n";
    std::cout << "  status / st           character sheet and flare clock\n";
    std::cout << "  map / m               deck schematic\n";
    std::cout << "  journal / j           recent events\n";
    std::cout << "  history               recent commands\n";
    std::cout << "  rest                  recover HP (costs 3 cycles, may attract attention)\n";
    std::cout << "  power                 seat a fusion cell in the reactor console\n";
    std::cout << "  launch                leave the station (Docking Bay, shard required)\n";
    std::cout << "  save / load           write or read " << kSaveFile << "\n";
    std::cout << "  quit                  save and exit\n";
    if (options_.debug) {
        std::cout << "  [debug] god, heal, xp <n>, tp <room>, turns <n>, kill, give <id>\n";
    }
}

// ---------------------------------------------------------------- movement

void Game::handleMovement(const std::string& argument) {
    int dir = World::directionIndex(argument);
    if (dir < 0) {
        std::cout << "  Go where? (north, east, south, west)\n";
        return;
    }
    const Room& here = world_.room(roomId_);
    int destination = here.exits[dir];
    if (destination < 0) {
        std::cout << "  The " << World::directionName(dir) << " wall is solid bulkhead.\n";
        return;
    }
    moveTo(destination);
}

void Game::moveTo(int destination) {
    if (!world_.validRoom(destination)) {
        std::cout << "  There is no way through.\n";
        return;
    }

    Room& target = world_.room(destination);
    if (!unlocked(target.lock)) {
        const Item* key = items_.find(target.lock);
        std::cout << "  " << target.name << " is sealed. ";
        if (key != 0) {
            std::cout << "You need the " << key->name << ".\n";
        } else {
            std::cout << "The door has no power.\n";
        }
        return;
    }

    previousRoom_ = roomId_;
    roomId_ = destination;
    tick(1);
    if (finished_) {
        return;
    }
    enterRoom();
}

void Game::enterRoom() {
    Room& room = world_.room(roomId_);
    room.visited = true;
    describe(room);
    flushTransmissions();
    if (!room.enemyId.empty()) {
        startCombat();
    }
}

// ---------------------------------------------------------------- combat

std::unique_ptr<Enemy> Game::spawnEnemy(const std::string& id) {
    const EnemyTemplate* tmpl = enemies_.find(id);
    if (tmpl == 0) {
        return std::unique_ptr<Enemy>();
    }

    EnemyTemplate scaled = *tmpl;
    int hpPercent = (difficulty_ == DIFF_EASY) ? 80 : (difficulty_ == DIFF_HARD ? 125 : 100);
    int atkPercent = (difficulty_ == DIFF_EASY) ? 75 : (difficulty_ == DIFF_HARD ? 130 : 100);

    scaled.hp = scaled.hp * hpPercent / 100;
    if (scaled.hp < 1) {
        scaled.hp = 1;
    }
    scaled.attack = scaled.attack * atkPercent / 100;
    if (scaled.attack < 1) {
        scaled.attack = 1;
    }
    return makeEnemy(scaled);
}

void Game::startCombat() {
    Room& room = world_.room(roomId_);
    if (room.enemyId.empty()) {
        std::cout << "  Nothing hostile here. Yet.\n";
        return;
    }

    std::string enemyId = room.enemyId;
    std::unique_ptr<Enemy> enemy = spawnEnemy(enemyId);
    if (!enemy) {
        room.enemyId.clear();
        return;
    }

    Combat combat(*player_, std::move(enemy), items_, rng_, log_, io_);
    CombatResult result = combat.run();
    if (result != COMBAT_LOSE) {
        tick(combat.rounds());   // blows traded cost station cycles
    }

    if (result == COMBAT_WIN) {
        room.enemyId.clear();
        if (enemyId == "nyx_warden") {
            note("The docking clamps release with a groan. The Kestrel is ready.");
        }
    } else if (result == COMBAT_LOSE) {
        gameOver("Your suit telemetry flatlines. Nyx-9 keeps its secrets.");
    } else {
        if (world_.validRoom(previousRoom_) && previousRoom_ != roomId_) {
            roomId_ = previousRoom_;
            std::cout << "  You fall back to the " << world_.room(roomId_).name << ".\n";
            look();
        } else {
            look();
        }
    }
}

// ---------------------------------------------------------------- actions

void Game::takeItem(const std::string& argument) {
    Room& room = world_.room(roomId_);
    if (!room.enemyId.empty()) {
        std::cout << "  Not while that thing is still moving.\n";
        return;
    }
    if (room.items.empty()) {
        std::cout << "  Nothing worth carrying here.\n";
        return;
    }

    if (argument.empty() || argument == "all") {
        std::vector<std::string> taken = room.items;
        room.items.clear();
        for (std::size_t i = 0; i < taken.size(); ++i) {
            collectItem(taken[i]);
        }
        tick(1);
        return;
    }

    const Item* wanted = resolveItem(argument);
    if (wanted == 0) {
        std::cout << "  You do not see that here.\n";
        return;
    }

    for (std::size_t i = 0; i < room.items.size(); ++i) {
        if (room.items[i] == wanted->id) {
            room.items.erase(room.items.begin() + static_cast<std::ptrdiff_t>(i));
            collectItem(wanted->id);
            tick(1);
            return;
        }
    }
    std::cout << "  You do not see that here.\n";
}

void Game::collectItem(const std::string& id) {
    const Item* item = items_.find(id);
    if (item == 0) {
        return;
    }
    player_->inventory().add(*item);
    std::cout << "  Taken: " << item->name << "\n";
    log_.add("Took " + item->name + ".");

    if (id == "shard") {
        flags_["shard_taken"] = true;
        note("The shard wakes. Every light on the station turns toward you.");
        addTransmission("KESTREL: Something just unlocked the docking clamps. Run for the bay!");
        Room& dock = world_.room(world_.startRoom());
        if (dock.enemyId.empty()) {
            dock.enemyId = "nyx_warden";
        }
    } else if (id == "fusion_cell") {
        note("The cell hums. The reactor console in the core could use this.");
    } else if (id == "keycard") {
        note("Level-3 clearance. The reactor bulkhead will open now.");
    }
}

void Game::useItem(const std::string& argument) {
    const Item* item = resolveInventoryItem(argument);
    if (item == 0) {
        std::cout << "  You are not carrying that.\n";
        return;
    }

    if (item->type == ITEM_CONSUMABLE) {
        int power = item->power;
        std::string id = item->id;
        std::string name = item->name;
        player_->inventory().remove(id);
        player_->heal(power);
        std::cout << "  You use " << name << " and recover " << power << " HP.\n";
        tick(1);
    } else if (item->type == ITEM_ARTIFACT) {
        std::cout << "  The shard is not a tool. It is a passenger.\n";
    } else if (item->type == ITEM_KEY) {
        std::cout << "  The keycard belongs in a door slot, not in your hand.\n";
    } else {
        std::cout << "  Nothing happens.\n";
    }
}

void Game::equipItem(const std::string& argument) {
    const Item* item = resolveInventoryItem(argument);
    if (item == 0) {
        std::cout << "  You are not carrying that.\n";
        return;
    }
    if (item->type == ITEM_WEAPON) {
        player_->equipWeapon(item->id);
        std::cout << "  You ready the " << item->name << " (+" << item->power << " ATK).\n";
    } else if (item->type == ITEM_ARMOR) {
        player_->equipArmor(item->id);
        std::cout << "  You strap on the " << item->name << " (+" << item->power << " DEF).\n";
    } else {
        std::cout << "  That is not something you can wear or wield.\n";
    }
}

void Game::restHere() {
    Room& room = world_.room(roomId_);
    if (!room.enemyId.empty()) {
        std::cout << "  Not with that thing watching.\n";
        return;
    }

    tick(3);
    if (finished_) {
        return;
    }

    int recovered = player_->maxHp() / 5;
    if (recovered < 1) {
        recovered = 1;
    }
    player_->heal(recovered);
    std::cout << "  You catch your breath (+" << recovered << " HP).\n";

    if (rng_.chance(25)) {
        int damage = rng_.range(3, 7);
        player_->takeDamage(damage);
        std::cout << "  A conduit blows and sprays you with coolant (-" << damage << " HP).\n";
        if (!player_->alive()) {
            gameOver("The coolant finishes what the station started.");
        }
    }
}

void Game::restorePower() {
    if (roomId_ != 6) {
        std::cout << "  There is no reactor console here.\n";
        return;
    }
    if (flags_["power_restored"]) {
        std::cout << "  The torus is already awake and humming.\n";
        return;
    }
    if (!player_->inventory().has("fusion_cell")) {
        std::cout << "  The console is dead. It needs a fusion cell.\n";
        return;
    }

    player_->inventory().remove("fusion_cell");
    flags_["power_restored"] = true;
    player_->boostMaxHp(5);
    player_->heal(10);
    tick(2);

    std::cout << "  You seat the cell. The torus spins up and the deck lights return.\n";
    std::cout << "  Somewhere above you, a vault bulkhead unlocks.\n";
    log_.add("Restored station power. The Server Vault is open.");
}

void Game::tryLaunch() {
    if (roomId_ != world_.startRoom()) {
        std::cout << "  The airlock is in the Docking Bay.\n";
        return;
    }
    if (!flags_["shard_taken"]) {
        std::cout << "  The Kestrel will not release the clamps without the shard.\n";
        return;
    }
    Room& dock = world_.room(roomId_);
    if (!dock.enemyId.empty()) {
        std::cout << "  The Nyx Warden stands between you and the airlock.\n";
        return;
    }
    victory();
}

// ---------------------------------------------------------------- endgame

void Game::gameOver(const std::string& reason) {
    finished_ = true;
    running_ = false;
    std::cout << "\n  ############################################################\n";
    std::cout << "  #                      GAME OVER                           #\n";
    std::cout << "  ############################################################\n";
    std::cout << "\n  " << reason << "\n";
    std::cout << "  Final level " << player_->level() << ", credits " << player_->credits()
              << ", turn " << turns_ << ".\n";
}

void Game::victory() {
    finished_ = true;
    running_ = false;
    std::cout << "\n  The Kestrel tears free of the cradle as the flare front hits the hull.\n";
    std::cout << "  Behind you Nyx-9 comes apart in silence, the shard warm against your ribs.\n";
    std::cout << "\n  ############################################################\n";
    std::cout << "  #                      VICTORY                             #\n";
    std::cout << "  ############################################################\n";
    std::cout << "\n  " << player_->name() << " escaped on turn " << turns_ << " of "
              << flareLimit_ << " with " << player_->credits() << " credits and level "
              << player_->level() << ".\n";
}

// ---------------------------------------------------------------- save

void Game::saveGame() {
    SaveMeta meta;
    meta.roomId = roomId_;
    meta.turns = turns_;
    meta.flareLimit = flareLimit_;
    meta.seed = seed_;
    meta.difficulty = difficulty_;
    meta.flags = flags_;

    if (SaveSystem::save(kSaveFile, *player_, world_, meta)) {
        std::cout << "  Progress written to " << kSaveFile << ".\n";
    } else {
        std::cout << "  Could not write " << kSaveFile << ".\n";
    }
}

// ---------------------------------------------------------------- helpers

bool Game::unlocked(const std::string& key) const {
    if (key.empty()) {
        return true;
    }
    if (player_->inventory().has(key)) {
        return true;
    }
    std::map<std::string, bool>::const_iterator it = flags_.find(key);
    return it != flags_.end() && it->second;
}

const Item* Game::resolveItem(const std::string& token) const {
    std::string key = toLower(trim(token));
    if (key.empty()) {
        return 0;
    }
    const Item* byId = items_.find(key);
    if (byId != 0) {
        return byId;
    }
    std::vector<std::string> ids = items_.allIds();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const Item* candidate = items_.find(ids[i]);
        if (candidate != 0 && toLower(candidate->name) == key) {
            return candidate;
        }
    }
    return 0;
}

const Item* Game::resolveInventoryItem(const std::string& token) const {
    const Item* item = resolveItem(token);
    if (item == 0) {
        return 0;
    }
    if (!player_->inventory().has(item->id)) {
        return 0;
    }
    return item;
}

void Game::tick(int amount) {
    turns_ += amount;
    if (turns_ >= flareLimit_ && !finished_) {
        gameOver("The collapsar's flare washes over Nyx-9 and the hull becomes light.");
    }
}

void Game::addTransmission(const std::string& text) {
    transmissions_.push(text);
}

void Game::flushTransmissions() {
    while (!transmissions_.empty()) {
        std::cout << "  [comm] " << transmissions_.front() << "\n";
        transmissions_.pop();
    }
}

void Game::note(const std::string& text) {
    std::cout << "  " << text << "\n";
    log_.add(text);
}

void Game::runDebug(const std::string& verb, const std::string& argument) {
    if (verb == "god" || verb == "heal") {
        player_->setHp(player_->maxHp());
        std::cout << "  [debug] HP restored to " << player_->hp() << ".\n";
    } else if (verb == "xp") {
        int amount = toInt(argument, 0);
        player_->gainXp(amount);
        std::cout << "  [debug] XP +" << amount << ", level " << player_->level() << ".\n";
    } else if (verb == "tp") {
        int target = toInt(argument, -1);
        if (!world_.validRoom(target)) {
            std::cout << "  [debug] no such room.\n";
            return;
        }
        roomId_ = target;
        previousRoom_ = target;
        enterRoom();
    } else if (verb == "turns") {
        turns_ = toInt(argument, 0);
        std::cout << "  [debug] turn counter set to " << turns_ << ".\n";
    } else if (verb == "kill") {
        world_.room(roomId_).enemyId.clear();
        std::cout << "  [debug] hostiles in this room removed.\n";
    } else if (verb == "give") {
        const Item* item = resolveItem(argument);
        if (item == 0) {
            std::cout << "  [debug] unknown item.\n";
            return;
        }
        player_->inventory().add(*item);
        std::cout << "  [debug] added " << item->name << ".\n";
    }
}

} // namespace nyx
