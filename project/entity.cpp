#include "entity.h"
#include "core.h"

#include <sstream>
#include <fstream>

namespace nyx {

namespace {

// Applies defence but always lets at least 1 point through.
int reduce(int damage, int defense) {
    int result = damage - defense;
    if (result < 1) {
        result = 1;
    }
    return result;
}

} // namespace

// ---------------------------------------------------------------- Entity

Entity::Entity(const std::string& name, int maxHp, int attack, int defense)
    : name_(name), hp_(maxHp), maxHp_(maxHp), attack_(attack), defense_(defense) {}

Entity::~Entity() {}

const std::string& Entity::name() const { return name_; }

void Entity::setName(const std::string& name) {
    if (!name.empty()) {
        name_ = name;
    }
}

int Entity::hp() const { return hp_; }
int Entity::maxHp() const { return maxHp_; }
int Entity::attack() const { return attack_; }
int Entity::defense() const { return defense_; }
bool Entity::alive() const { return hp_ > 0; }

void Entity::takeDamage(int amount) {
    if (amount < 0) {
        amount = 0;
    }
    hp_ -= amount;
    if (hp_ < 0) {
        hp_ = 0;
    }
}

void Entity::heal(int amount) {
    if (amount < 0) {
        return;
    }
    hp_ += amount;
    if (hp_ > maxHp_) {
        hp_ = maxHp_;
    }
}

void Entity::boostMaxHp(int amount) {
    if (amount <= 0) {
        return;
    }
    maxHp_ += amount;
    hp_ += amount;
}

int Entity::bonusDamage(Random& rng) const {
    (void)rng;
    return 0;
}

// ---------------------------------------------------------------- Player

Player::Player(const std::string& name)
    : Entity(name, 30, 4, 1), level_(1), xp_(0), credits_(40),
      weaponId_(), armorId_(), inventory_() {}

std::string Player::kind() const { return "salvager"; }
std::string Player::battleCry() const { return "You raise your weapon."; }

int Player::level() const { return level_; }
int Player::xp() const { return xp_; }
int Player::xpNeeded() const { return level_ * 40; }
int Player::credits() const { return credits_; }

void Player::addCredits(int amount) {
    credits_ += amount;
    if (credits_ < 0) {
        credits_ = 0;
    }
}

bool Player::gainXp(int amount) {
    if (amount <= 0) {
        return false;
    }
    xp_ += amount;
    bool leveled = false;
    while (xp_ >= xpNeeded()) {
        xp_ -= xpNeeded();
        ++level_;
        maxHp_ += 8;
        attack_ += 2;
        defense_ += 1;
        hp_ = maxHp_;   // a level-up fully restores the suit
        leveled = true;
    }
    return leveled;
}

Inventory& Player::inventory() { return inventory_; }
const Inventory& Player::inventory() const { return inventory_; }

void Player::equipWeapon(const std::string& id) { weaponId_ = id; }
void Player::equipArmor(const std::string& id) { armorId_ = id; }
const std::string& Player::weaponId() const { return weaponId_; }
const std::string& Player::armorId() const { return armorId_; }

int Player::totalAttack(const ItemRegistry& registry) const {
    int total = attack_;
    if (!weaponId_.empty()) {
        const Item* weapon = registry.find(weaponId_);
        if (weapon != 0) {
            total += weapon->power;
        }
    }
    return total;
}

int Player::totalDefense(const ItemRegistry& registry) const {
    int total = defense_;
    if (!armorId_.empty()) {
        const Item* armor = registry.find(armorId_);
        if (armor != 0) {
            total += armor->power;
        }
    }
    return total;
}

void Player::setHp(int value) {
    hp_ = value;
    if (hp_ < 1) {
        hp_ = 1;
    }
    if (hp_ > maxHp_) {
        hp_ = maxHp_;
    }
}

void Player::restoreState(int level, int xp, int credits, int maxHp, int attack, int defense) {
    level_ = level;
    xp_ = xp;
    credits_ = credits;
    maxHp_ = maxHp;
    attack_ = attack;
    defense_ = defense;
    hp_ = maxHp;
}

// ---------------------------------------------------------------- Enemy

Enemy::Enemy(const EnemyTemplate& tmpl)
    : Entity(tmpl.name, tmpl.hp, tmpl.attack, tmpl.defense),
      id_(tmpl.id), kind_(tmpl.kind), xpReward_(tmpl.xp),
      creditReward_(tmpl.credits), specialChance_(tmpl.specialChance) {}

Enemy::~Enemy() {}

const std::string& Enemy::id() const { return id_; }
EnemyKind Enemy::enemyKind() const { return kind_; }
int Enemy::xpReward() const { return xpReward_; }
int Enemy::creditReward() const { return creditReward_; }
int Enemy::specialChance() const { return specialChance_; }

std::string Enemy::special(Player& target, Random& rng) {
    int damage = reduce(attack_ + rng.range(0, 3), target.defense());
    target.takeDamage(damage);
    std::ostringstream out;
    out << name_ << " strikes for " << damage << " damage.";
    return out.str();
}

// ---------------------------------------------------------------- Drone

DroneUnit::DroneUnit(const EnemyTemplate& tmpl) : Enemy(tmpl) {}

std::string DroneUnit::kind() const { return "maintenance drone"; }

std::string DroneUnit::battleCry() const {
    return "A maintenance drone unfolds from its charging cradle, welder arm snapping open.";
}

std::string DroneUnit::special(Player& target, Random& rng) {
    int first = reduce(attack_ / 2 + rng.range(1, 3), target.defense());
    int second = reduce(attack_ / 2 + rng.range(1, 3), target.defense());
    target.takeDamage(first);
    target.takeDamage(second);
    std::ostringstream out;
    out << name_ << " fires a twin pulse: " << first << " + " << second << " damage.";
    return out.str();
}

// ---------------------------------------------------------------- Sentry

SentryUnit::SentryUnit(const EnemyTemplate& tmpl) : Enemy(tmpl) {}

std::string SentryUnit::kind() const { return "security sentry"; }

std::string SentryUnit::battleCry() const {
    return "A security sentry pivots on its rail, painting you with a targeter.";
}

std::string SentryUnit::special(Player& target, Random& rng) {
    int damage = reduce(attack_ + rng.range(2, 5), target.defense());
    target.takeDamage(damage);
    std::ostringstream out;
    out << name_ << " fires a suppressive burst for " << damage << " damage.";
    return out.str();
}

// ---------------------------------------------------------------- Hound

HoundUnit::HoundUnit(const EnemyTemplate& tmpl) : Enemy(tmpl) {}

std::string HoundUnit::kind() const { return "spore hound"; }

std::string HoundUnit::battleCry() const {
    return "Something wet drags itself out of the hydroponics rows.";
}

std::string HoundUnit::special(Player& target, Random& rng) {
    int damage = reduce(attack_ + rng.range(1, 4), target.defense());
    target.takeDamage(damage);
    std::ostringstream out;
    out << name_ << " sprays acid for " << damage << " damage.";
    return out.str();
}

// ---------------------------------------------------------------- Wraith

WraithUnit::WraithUnit(const EnemyTemplate& tmpl) : Enemy(tmpl) {}

std::string WraithUnit::kind() const { return "overload wraith"; }

std::string WraithUnit::battleCry() const {
    return "The air ionises. An overload wraith peels itself off the reactor housing.";
}

std::string WraithUnit::special(Player& target, Random& rng) {
    int damage = reduce(attack_ + rng.range(1, 4), target.defense());
    target.takeDamage(damage);
    heal(damage / 2);
    std::ostringstream out;
    out << name_ << " drains " << damage << " damage and knits itself back together.";
    return out.str();
}

// ---------------------------------------------------------------- Warden

WardenUnit::WardenUnit(const EnemyTemplate& tmpl) : Enemy(tmpl) {}

std::string WardenUnit::kind() const { return "warden construct"; }

std::string WardenUnit::battleCry() const {
    return "A warden construct uncoils from the vault cradle, six optics lighting at once.";
}

std::string WardenUnit::special(Player& target, Random& rng) {
    bool enraged = (hp_ * 100 / maxHp_) < 35;
    int base = attack_ + rng.range(2, 6);
    if (enraged) {
        base += attack_ / 3;
    }
    int damage = reduce(base, target.defense());
    target.takeDamage(damage);
    std::ostringstream out;
    out << name_ << (enraged ? " slams the deck in a gravity surge for " : " slams you for ")
        << damage << " damage.";
    return out.str();
}

// ---------------------------------------------------------------- Overseer

OverseerUnit::OverseerUnit(const EnemyTemplate& tmpl) : Enemy(tmpl) {}

std::string OverseerUnit::kind() const { return "nyx overseer"; }

std::string OverseerUnit::battleCry() const {
    return "The docking bay speakers crackle: 'YOU ARE NOT AUTHORISED TO LEAVE.'";
}

std::string OverseerUnit::special(Player& target, Random& rng) {
    int damage = reduce(attack_ + rng.range(3, 8), target.defense());
    target.takeDamage(damage);
    heal(2);
    std::ostringstream out;
    out << name_ << " vents plasma from the docking clamps: " << damage << " damage.";
    return out.str();
}

// ---------------------------------------------------------------- factory

std::unique_ptr<Enemy> makeEnemy(const EnemyTemplate& tmpl) {
    switch (tmpl.kind) {
        case ENEMY_DRONE:    return std::unique_ptr<Enemy>(new DroneUnit(tmpl));
        case ENEMY_SENTRY:   return std::unique_ptr<Enemy>(new SentryUnit(tmpl));
        case ENEMY_HOUND:    return std::unique_ptr<Enemy>(new HoundUnit(tmpl));
        case ENEMY_WRAITH:   return std::unique_ptr<Enemy>(new WraithUnit(tmpl));
        case ENEMY_WARDEN:   return std::unique_ptr<Enemy>(new WardenUnit(tmpl));
        case ENEMY_OVERSEER: return std::unique_ptr<Enemy>(new OverseerUnit(tmpl));
        default:             return std::unique_ptr<Enemy>(new DroneUnit(tmpl));
    }
}

// ---------------------------------------------------------------- parsing

EnemyKind parseEnemyKind(const std::string& text) {
    std::string t = toLower(trim(text));
    if (t == "drone")    return ENEMY_DRONE;
    if (t == "sentry")   return ENEMY_SENTRY;
    if (t == "hound")    return ENEMY_HOUND;
    if (t == "wraith")   return ENEMY_WRAITH;
    if (t == "warden")   return ENEMY_WARDEN;
    if (t == "overseer") return ENEMY_OVERSEER;
    return ENEMY_UNKNOWN;
}

std::string enemyKindName(EnemyKind kind) {
    switch (kind) {
        case ENEMY_DRONE:    return "drone";
        case ENEMY_SENTRY:   return "sentry";
        case ENEMY_HOUND:    return "hound";
        case ENEMY_WRAITH:   return "wraith";
        case ENEMY_WARDEN:   return "warden";
        case ENEMY_OVERSEER: return "overseer";
        default:             return "unknown";
    }
}

EnemyTemplate::EnemyTemplate()
    : id(), name(), kind(ENEMY_UNKNOWN), hp(10), attack(3), defense(0),
      xp(5), credits(5), specialChance(20) {}

// ---------------------------------------------------------------- EnemyBook

EnemyBook::EnemyBook() : table_() {}

bool EnemyBook::loadFromFile(const std::string& path) {
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
        if (fields.size() < 9) {
            continue;
        }
        EnemyTemplate tmpl;
        tmpl.id = trim(fields[0]);
        tmpl.name = trim(fields[1]);
        tmpl.kind = parseEnemyKind(fields[2]);
        tmpl.hp = toInt(trim(fields[3]), 10);
        tmpl.attack = toInt(trim(fields[4]), 3);
        tmpl.defense = toInt(trim(fields[5]), 0);
        tmpl.xp = toInt(trim(fields[6]), 5);
        tmpl.credits = toInt(trim(fields[7]), 5);
        tmpl.specialChance = toInt(trim(fields[8]), 20);
        if (tmpl.id.empty() || tmpl.kind == ENEMY_UNKNOWN) {
            continue;
        }
        table_[tmpl.id] = tmpl;
        loadedAny = true;
    }
    return loadedAny;
}

void EnemyBook::addDefaults() {
    EnemyTemplate drone;
    drone.id = "maintenance_drone";
    drone.name = "Maintenance Drone";
    drone.kind = ENEMY_DRONE;
    drone.hp = 18;
    drone.attack = 5;
    drone.defense = 1;
    drone.xp = 15;
    drone.credits = 10;
    drone.specialChance = 25;
    table_[drone.id] = drone;

    EnemyTemplate sentry;
    sentry.id = "sentry_drone";
    sentry.name = "Sentry Drone";
    sentry.kind = ENEMY_SENTRY;
    sentry.hp = 26;
    sentry.attack = 6;
    sentry.defense = 3;
    sentry.xp = 25;
    sentry.credits = 20;
    sentry.specialChance = 30;
    table_[sentry.id] = sentry;

    EnemyTemplate hound;
    hound.id = "spore_hound";
    hound.name = "Spore Hound";
    hound.kind = ENEMY_HOUND;
    hound.hp = 24;
    hound.attack = 6;
    hound.defense = 2;
    hound.xp = 20;
    hound.credits = 15;
    hound.specialChance = 35;
    table_[hound.id] = hound;

    EnemyTemplate wraith;
    wraith.id = "overload_wraith";
    wraith.name = "Overload Wraith";
    wraith.kind = ENEMY_WRAITH;
    wraith.hp = 34;
    wraith.attack = 9;
    wraith.defense = 3;
    wraith.xp = 35;
    wraith.credits = 30;
    wraith.specialChance = 40;
    table_[wraith.id] = wraith;

    EnemyTemplate warden;
    warden.id = "warden_prime";
    warden.name = "Warden Prime";
    warden.kind = ENEMY_WARDEN;
    warden.hp = 60;
    warden.attack = 10;
    warden.defense = 4;
    warden.xp = 70;
    warden.credits = 80;
    warden.specialChance = 45;
    table_[warden.id] = warden;

    EnemyTemplate overseer;
    overseer.id = "nyx_warden";
    overseer.name = "Nyx Warden";
    overseer.kind = ENEMY_OVERSEER;
    overseer.hp = 90;
    overseer.attack = 12;
    overseer.defense = 5;
    overseer.xp = 120;
    overseer.credits = 150;
    overseer.specialChance = 50;
    table_[overseer.id] = overseer;
}

const EnemyTemplate* EnemyBook::find(const std::string& id) const {
    std::map<std::string, EnemyTemplate>::const_iterator it = table_.find(id);
    if (it == table_.end()) {
        return 0;
    }
    return &(it->second);
}

std::size_t EnemyBook::size() const {
    return table_.size();
}

} // namespace nyx
