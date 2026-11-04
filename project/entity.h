#ifndef NYX_ENTITY_H
#define NYX_ENTITY_H

#include <string>
#include <memory>
#include <map>
#include <cstddef>

#include "core.h"
#include "item.h"

namespace nyx {

// ---------------------------------------------------------------------
// Entity: abstract base class for everything that can fight.
//
// Pure virtual functions (kind, battleCry) make this an interface; the
// Player and the six enemy classes below are the concrete implementations,
// so every combat message is resolved by dynamic dispatch.
// ---------------------------------------------------------------------
class Entity {
public:
    Entity(const std::string& name, int maxHp, int attack, int defense);
    virtual ~Entity();

    const std::string& name() const;
    void setName(const std::string& name);
    int hp() const;
    int maxHp() const;
    int attack() const;
    int defense() const;
    bool alive() const;

    void takeDamage(int amount);
    void heal(int amount);
    void boostMaxHp(int amount);

    virtual std::string kind() const = 0;
    virtual std::string battleCry() const = 0;
    virtual int bonusDamage(Random& rng) const;

protected:
    std::string name_;
    int hp_;
    int maxHp_;
    int attack_;
    int defense_;
};

// ---------------------------------------------------------------------
// Player: the character sheet.
// ---------------------------------------------------------------------
class Player : public Entity {
public:
    explicit Player(const std::string& name);

    virtual std::string kind() const;
    virtual std::string battleCry() const;

    int level() const;
    int xp() const;
    int xpNeeded() const;
    int credits() const;
    void addCredits(int amount);

    // Adds experience and applies every level-up it triggers.
    // Returns true if at least one level was gained.
    bool gainXp(int amount);

    Inventory& inventory();
    const Inventory& inventory() const;

    void equipWeapon(const std::string& id);
    void equipArmor(const std::string& id);
    const std::string& weaponId() const;
    const std::string& armorId() const;

    int totalAttack(const ItemRegistry& registry) const;
    int totalDefense(const ItemRegistry& registry) const;

    void setHp(int value);
    void restoreState(int level, int xp, int credits, int maxHp, int attack, int defense);

private:
    int level_;
    int xp_;
    int credits_;
    std::string weaponId_;
    std::string armorId_;
    Inventory inventory_;
};

// ---------------------------------------------------------------------
// Enemy kinds and their data-driven templates (data/enemies.txt).
// ---------------------------------------------------------------------
enum EnemyKind {
    ENEMY_DRONE,
    ENEMY_SENTRY,
    ENEMY_HOUND,
    ENEMY_WRAITH,
    ENEMY_WARDEN,
    ENEMY_OVERSEER,
    ENEMY_UNKNOWN
};

EnemyKind parseEnemyKind(const std::string& text);
std::string enemyKindName(EnemyKind kind);

struct EnemyTemplate {
    std::string id;
    std::string name;
    EnemyKind kind;
    int hp;
    int attack;
    int defense;
    int xp;
    int credits;
    int specialChance;

    EnemyTemplate();
};

class Enemy : public Entity {
public:
    explicit Enemy(const EnemyTemplate& tmpl);
    virtual ~Enemy();

    const std::string& id() const;
    EnemyKind enemyKind() const;
    int xpReward() const;
    int creditReward() const;

    // Percentage chance per round of using the special move.
    virtual int specialChance() const;

    // Executes the special move and returns a one-line description.
    virtual std::string special(Player& target, Random& rng);

protected:
    std::string id_;
    EnemyKind kind_;
    int xpReward_;
    int creditReward_;
    int specialChance_;
};

// --- concrete enemies -------------------------------------------------
// Each subclass overrides kind()/battleCry() (pure virtual from Entity)
// and special(), which is what makes the combat log differ per enemy.

class DroneUnit : public Enemy {
public:
    explicit DroneUnit(const EnemyTemplate& tmpl);
    virtual std::string kind() const;
    virtual std::string battleCry() const;
    virtual std::string special(Player& target, Random& rng);
};

class SentryUnit : public Enemy {
public:
    explicit SentryUnit(const EnemyTemplate& tmpl);
    virtual std::string kind() const;
    virtual std::string battleCry() const;
    virtual std::string special(Player& target, Random& rng);
};

class HoundUnit : public Enemy {
public:
    explicit HoundUnit(const EnemyTemplate& tmpl);
    virtual std::string kind() const;
    virtual std::string battleCry() const;
    virtual std::string special(Player& target, Random& rng);
};

class WraithUnit : public Enemy {
public:
    explicit WraithUnit(const EnemyTemplate& tmpl);
    virtual std::string kind() const;
    virtual std::string battleCry() const;
    virtual std::string special(Player& target, Random& rng);
};

class WardenUnit : public Enemy {
public:
    explicit WardenUnit(const EnemyTemplate& tmpl);
    virtual std::string kind() const;
    virtual std::string battleCry() const;
    virtual std::string special(Player& target, Random& rng);
};

class OverseerUnit : public Enemy {
public:
    explicit OverseerUnit(const EnemyTemplate& tmpl);
    virtual std::string kind() const;
    virtual std::string battleCry() const;
    virtual std::string special(Player& target, Random& rng);
};

// Factory: maps a template to the matching concrete class.
std::unique_ptr<Enemy> makeEnemy(const EnemyTemplate& tmpl);

// ---------------------------------------------------------------------
// EnemyBook: id -> template lookup, loaded from data/enemies.txt.
// ---------------------------------------------------------------------
class EnemyBook {
public:
    EnemyBook();

    bool loadFromFile(const std::string& path);
    void addDefaults();

    const EnemyTemplate* find(const std::string& id) const;
    std::size_t size() const;

private:
    std::map<std::string, EnemyTemplate> table_;
};

} // namespace nyx

#endif // NYX_ENTITY_H
