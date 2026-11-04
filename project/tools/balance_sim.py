import random

# Exact formulas mirrored from combat.cpp / entity.cpp
def rdmg(atk, dfn, rng):
    d = atk + rng.randint(0, 2) - dfn
    d = max(1, d)
    if rng.randint(1, 100) <= 12:
        d *= 2
    return d

def reduce(dmg, dfn):
    return max(1, dmg - dfn)

def special(kind, atk, hp, maxhp, pdf, rng):
    if kind == "drone":
        a = reduce(atk // 2 + rng.randint(1, 3), pdf)
        b = reduce(atk // 2 + rng.randint(1, 3), pdf)
        return a + b, 0
    if kind == "sentry":
        return reduce(atk + rng.randint(2, 5), pdf), 0
    if kind == "hound":
        return reduce(atk + rng.randint(1, 4), pdf), 0
    if kind == "wraith":
        d = reduce(atk + rng.randint(1, 4), pdf)
        return d, d // 2
    if kind == "warden":
        base = atk + rng.randint(2, 6)
        if hp * 100 // maxhp < 35:
            base += atk // 3
        return reduce(base, pdf), 0
    if kind == "overseer":
        return reduce(atk + rng.randint(3, 8), pdf), 2
    return 0, 0

ENEMIES = [
    # name, kind, hp, atk, def, xp, special%
    ("Maintenance Drone", "drone", 18, 5, 1, 15, 25),
    ("Sentry Drone", "sentry", 26, 6, 3, 25, 30),
    ("Spore Hound", "hound", 24, 6, 2, 20, 35),
    ("Overload Wraith", "wraith", 34, 9, 3, 35, 40),
    ("Warden Prime", "warden", 60, 10, 4, 70, 45),
    ("Nyx Warden", "overseer", 90, 12, 5, 120, 50),
]

def run(seed, medkits=4, verbose=False):
    rng = random.Random(seed)
    lvl, xp = 1, 0
    maxhp, hp = 30, 30
    atk, dfn = 4, 1
    weapon = 3          # wrench
    armor = 0
    kits = medkits
    need = lambda: lvl * 40
    log = []

    def maybe_level():
        nonlocal lvl, xp, maxhp, atk, dfn, hp
        leveled = False
        while xp >= need():
            xp -= need()
            lvl += 1
            maxhp += 8
            atk += 2
            dfn += 1
            hp = maxhp
            leveled = True
        return leveled

    for (name, kind, ehp, eatk, edef, exp, sp) in ENEMIES:
        if name == "Sentry Drone":
            weapon = 3           # still wrench
        if name == "Spore Hound":
            weapon, armor = 10, 4   # needler + riot plating
        if name == "Nyx Warden":
            armor = 6            # nanoforge
        pa = atk + weapon
        pd = dfn + armor

        if verbose:
            log.append(f"vs {name}: player {hp}/{maxhp} atk {pa} def {pd}")

        rounds = 0
        while hp > 0 and ehp > 0 and rounds < 200:
            rounds += 1
            # player action: heal if low, else attack
            if hp < maxhp * 40 // 100 and kits > 0:
                kits -= 1
                hp = min(maxhp, hp + 25)
            else:
                ehp -= rdmg(pa, edef, rng)
            if ehp <= 0:
                break
            # enemy action
            if rng.randint(1, 100) <= sp:
                d, heal = special(kind, eatk, ehp, 110 if kind == "overseer" else (70 if kind == "warden" else ehp), pd, rng)
                hp -= d
                ehp = min(ehp + heal, 999)
            else:
                hp -= max(1, eatk + rng.randint(0, 2) - pd)
        if hp <= 0:
            return False, log + [f"died to {name} at level {lvl} after {rounds} rounds"]
        xp += exp
        if maybe_level() and verbose:
            log.append(f"level up -> {lvl}")
    return True, log

wins = sum(1 for s in range(4000) if run(s)[0])
print(f"win rate over 4000 seeds: {wins/40:.1f}%")
for name, res in (("winning run", None),):
    pass
ok, log = run(7, verbose=True)
print("sample:", "WIN" if ok else "LOSS")
for line in log[:14]:
    print("   ", line)
