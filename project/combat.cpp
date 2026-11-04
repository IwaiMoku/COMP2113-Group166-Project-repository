#include "combat.h"
#include "core.h"

#include <iostream>
#include <sstream>
#include <utility>

namespace nyx {

Combat::Combat(Player& player, std::unique_ptr<Enemy> enemy, const ItemRegistry& items,
               Random& rng, LogList& log, Io& io)
    : player_(player), enemy_(std::move(enemy)), items_(items), rng_(rng), log_(log), io_(io),
      rounds_(0) {}

void Combat::printStatus() const {
    std::cout << "\n  " << player_.name() << "  HP " << player_.hp() << "/" << player_.maxHp()
              << "    |    " << enemy_->name() << "  HP " << enemy_->hp() << "/"
              << enemy_->maxHp() << "\n";
}

int Combat::playerDamage() {
    int damage = player_.totalAttack(items_) + rng_.range(0, 2) - enemy_->defense();
    if (damage < 1) {
        damage = 1;
    }
    if (rng_.chance(12)) {
        damage *= 2;
        std::cout << "  Critical hit! ";
    }
    return damage;
}

int Combat::enemyDamage(bool defending) {
    int damage = enemy_->attack() + rng_.range(0, 2) - player_.totalDefense(items_);
    if (defending) {
        damage = damage / 2;
    }
    if (damage < 1) {
        damage = 1;
    }
    return damage;
}

void Combat::enemyTurn(bool defending) {
    if (!enemy_->alive()) {
        return;
    }
    if (rng_.chance(enemy_->specialChance())) {
        std::string text = enemy_->special(player_, rng_);
        std::cout << "  " << text << "\n";
        log_.add(text);
        return;
    }
    int damage = enemyDamage(defending);
    player_.takeDamage(damage);
    std::ostringstream out;
    out << enemy_->name() << " hits you for " << damage << " damage.";
    std::cout << "  " << out.str() << "\n";
    log_.add(out.str());
}

bool Combat::useItem(const std::string& itemId) {
    const Item* item = player_.inventory().find(itemId);
    if (item == 0) {
        std::cout << "  You are not carrying that.\n";
        return false;
    }
    if (item->type != ITEM_CONSUMABLE) {
        std::cout << "  " << item->name << " cannot be used in combat.\n";
        return false;
    }
    int power = item->power;
    std::string name = item->name;
    player_.inventory().remove(itemId);
    player_.heal(power);
    std::cout << "  You use " << name << " and recover " << power << " HP.\n";
    std::ostringstream out;
    out << "Used " << name << " (+" << power << " HP).";
    log_.add(out.str());
    return true;
}

void Combat::playerWon() {
    std::cout << "\n  " << enemy_->name() << " goes dark.\n";
    std::ostringstream out;
    out << "Defeated " << enemy_->name() << " (+" << enemy_->xpReward() << " XP, +"
        << enemy_->creditReward() << " credits).";
    std::cout << "  " << out.str() << "\n";
    log_.add(out.str());

    player_.addCredits(enemy_->creditReward());
    bool leveled = player_.gainXp(enemy_->xpReward());
    if (leveled) {
        std::cout << "  *** LEVEL UP *** You are now level " << player_.level()
                  << ". Max HP " << player_.maxHp() << ", ATK " << player_.attack()
                  << ", DEF " << player_.defense() << ".\n";
        std::ostringstream up;
        up << "Reached level " << player_.level() << ".";
        log_.add(up.str());
    }
}

int Combat::rounds() const {
    return rounds_;
}

CombatResult Combat::run() {
    std::cout << "\n=== COMBAT ===\n" << enemy_->battleCry() << "\n";
    log_.add("Combat with " + enemy_->name() + ".");

    while (player_.alive() && enemy_->alive()) {
        ++rounds_;
        printStatus();
        std::string line = io_.readLine("action> ");
        if (trim(line).empty() && io_.eof()) {
            std::cout << "\n  The suit link drops mid-fight.\n";
            return COMBAT_FLED;
        }
        std::string lower = toLower(trim(line));

        if (lower.empty()) {
            continue;
        }

        std::string verb = lower;
        std::string argument;
        std::size_t space = lower.find(' ');
        if (space != std::string::npos) {
            verb = lower.substr(0, space);
            argument = trim(lower.substr(space + 1));
        }

        bool defending = false;

        if (verb == "attack" || verb == "a" || verb == "hit") {
            int damage = playerDamage();
            enemy_->takeDamage(damage);
            std::cout << "  You strike " << enemy_->name() << " for " << damage << " damage.\n";
            std::ostringstream out;
            out << "Hit " << enemy_->name() << " for " << damage << ".";
            log_.add(out.str());
        } else if (verb == "defend" || verb == "guard") {
            defending = true;
            std::cout << "  You brace behind your plating.\n";
        } else if (verb == "use") {
            if (!useItem(argument)) {
                continue;   // invalid item: do not give the enemy a free hit
            }
        } else if (verb == "scan") {
            std::cout << "  " << enemy_->name() << " (" << enemy_->kind() << ")  HP "
                      << enemy_->hp() << "/" << enemy_->maxHp() << "  ATK " << enemy_->attack()
                      << "  DEF " << enemy_->defense() << "\n";
            continue;
        } else if (verb == "flee" || verb == "run") {
            if (rng_.chance(45)) {
                std::cout << "  You break contact and fall back.\n";
                log_.add("Fled from " + enemy_->name() + ".");
                return COMBAT_FLED;
            }
            std::cout << "  The way is blocked - you cannot disengage.\n";
        } else if (verb == "help" || verb == "?") {
            std::cout << "  Combat commands: attack, defend, use <item>, scan, flee.\n";
            continue;
        } else {
            std::cout << "  Unknown combat action. Try attack, defend, use <item>, scan, flee.\n";
            continue;
        }

        if (enemy_->alive()) {
            enemyTurn(defending);
        }
    }

    if (player_.alive()) {
        playerWon();
        return COMBAT_WIN;
    }

    std::cout << "\n  Your suit telemetry flatlines.\n";
    log_.add("You were killed by " + enemy_->name() + ".");
    return COMBAT_LOSE;
}

} // namespace nyx
