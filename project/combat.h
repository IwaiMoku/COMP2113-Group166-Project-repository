#ifndef NYX_COMBAT_H
#define NYX_COMBAT_H

#include <memory>
#include <string>

#include "core.h"
#include "entity.h"
#include "item.h"

namespace nyx {

enum CombatResult {
    COMBAT_WIN,
    COMBAT_LOSE,
    COMBAT_FLED
};

// ---------------------------------------------------------------------
// Turn-based combat.
//
// Combat owns the enemy it fights (unique_ptr), so the enemy dies with the
// encounter and cannot be resurrected by a stale pointer.
// ---------------------------------------------------------------------
class Combat {
public:
    Combat(Player& player, std::unique_ptr<Enemy> enemy, const ItemRegistry& items,
           Random& rng, LogList& log, Io& io);

    CombatResult run();

    // Number of rounds the fight lasted, so the caller can advance the
    // station clock: time passes while you trade blows.
    int rounds() const;

private:
    void printStatus() const;
    int playerDamage();
    int enemyDamage(bool defending);
    void enemyTurn(bool defending);
    bool useItem(const std::string& itemId);
    void playerWon();

    Player& player_;
    std::unique_ptr<Enemy> enemy_;
    const ItemRegistry& items_;
    Random& rng_;
    LogList& log_;
    Io& io_;
    int rounds_;
};

} // namespace nyx

#endif // NYX_COMBAT_H
