#ifndef NYX_GAME_H
#define NYX_GAME_H

#include <string>
#include <map>
#include <stack>
#include <queue>
#include <memory>
#include <istream>

#include "core.h"
#include "item.h"
#include "entity.h"
#include "world.h"

namespace nyx {

enum Difficulty {
    DIFF_EASY = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD = 2
};

struct Options {
    bool useSeed;
    unsigned int seed;
    bool debug;
    std::string scriptPath;
    std::string loadPath;
    int difficulty;

    Options();
};

// ---------------------------------------------------------------------
// Game: the application shell.
//
// Owns every subsystem (registry, world, player) and drives the menu, the
// command loop and the difficulty settings. All randomness comes from the
// single Random member, so --seed reproduces a whole session.
// ---------------------------------------------------------------------
class Game {
public:
    Game(const Options& options, std::istream& in);
    ~Game();

    int run();

private:
    // --- setup / menus
    void printTitle() const;
    bool startMenu();
    void chooseDifficulty();
    void newGame();
    bool loadGame(const std::string& path);
    bool saveExists() const;

    // --- main loop
    void mainLoop();
    void processCommand(const std::string& raw);

    // --- views
    void look() const;
    void showStatus() const;
    void showInventory() const;
    void showMap() const;
    void showJournal() const;
    void showHistory() const;
    void showHelp() const;
    void describe(const Room& room) const;

    // --- actions
    void handleMovement(const std::string& argument);
    void moveTo(int destination);
    void takeItem(const std::string& argument);
    void collectItem(const std::string& id);
    void useItem(const std::string& argument);
    void equipItem(const std::string& argument);
    void restHere();
    void startCombat();
    void tryLaunch();
    void restorePower();
    void saveGame();
    void runDebug(const std::string& verb, const std::string& argument);

    // --- helpers
    bool unlocked(const std::string& key) const;
    const Item* resolveItem(const std::string& token) const;
    const Item* resolveInventoryItem(const std::string& token) const;
    std::string describeItemList(const std::vector<std::string>& ids) const;
    std::unique_ptr<Enemy> spawnEnemy(const std::string& id);
    void enterRoom();
    void tick(int amount);
    void addTransmission(const std::string& text);
    void flushTransmissions();
    void note(const std::string& text);
    void gameOver(const std::string& reason);
    void victory();

    // --- state
    Options options_;
    Io io_;
    unsigned int seed_;
    Random rng_;
    ItemRegistry items_;
    EnemyBook enemies_;
    World world_;
    std::unique_ptr<Player> player_;
    LogList log_;
    std::map<std::string, bool> flags_;
    std::stack<std::string> history_;
    std::queue<std::string> transmissions_;
    int roomId_;
    int previousRoom_;
    int turns_;
    int flareLimit_;
    int difficulty_;
    bool running_;
    bool finished_;
};

} // namespace nyx

#endif // NYX_GAME_H
