import os, sys

ROOT = r"C:\Users\lenovo\Documents\deepseek-harness\default-workspace\nyx9"
os.chdir(ROOT)
errs = []

# ---- items ----
items = {}
for line in open("data/items.txt", encoding="utf-8"):
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    f = [x.strip() for x in line.split("|")]
    if len(f) < 6:
        errs.append(f"items: bad field count {len(f)}: {line}")
        continue
    if f[0] in items:
        errs.append(f"items: duplicate id {f[0]}")
    if f[2] not in ("weapon", "armor", "consumable", "key", "artifact", "misc"):
        errs.append(f"items: bad type {f[2]} for {f[0]}")
    items[f[0]] = f

# ---- enemies ----
enemies = {}
for line in open("data/enemies.txt", encoding="utf-8"):
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    f = [x.strip() for x in line.split("|")]
    if len(f) < 9:
        errs.append(f"enemies: bad field count {len(f)}: {line}")
        continue
    if f[0] in enemies:
        errs.append(f"enemies: duplicate id {f[0]}")
    if f[2] not in ("drone", "sentry", "hound", "wraith", "warden", "overseer"):
        errs.append(f"enemies: bad kind {f[2]} for {f[0]}")
    for idx, label in ((3, "hp"), (4, "atk"), (5, "def"), (6, "xp"), (7, "cr"), (8, "special")):
        if not f[idx].lstrip("-").isdigit():
            errs.append(f"enemies: {label} not numeric for {f[0]}")
    enemies[f[0]] = f

# ---- rooms ----
rooms = []
for line in open("data/rooms.txt", encoding="utf-8"):
    line = line.strip()
    if not line or line.startswith("#"):
        continue
    f = [x.strip() for x in line.split("|")]
    if len(f) < 10:
        errs.append(f"rooms: bad field count {len(f)}: {line[:60]}")
        continue
    rooms.append(f)

if len(rooms) != 8:
    errs.append(f"rooms: expected 8, found {len(rooms)}")

for i, f in enumerate(rooms):
    if f[0] != str(i):
        errs.append(f"rooms: id {f[0]} at position {i} (ids must be 0..N-1 in order)")
    for col in range(3, 7):
        v = f[col]
        if v == "-":
            continue
        if not v.isdigit() or int(v) >= len(rooms):
            errs.append(f"rooms: bad exit '{v}' in room {f[0]}")
    if f[7] not in ("-", ""):
        for item in f[7].split(","):
            if item.strip() not in items:
                errs.append(f"rooms: unknown item '{item}' in room {f[0]}")
    if f[8] not in ("-", ""):
        if f[8] not in enemies:
            errs.append(f"rooms: unknown enemy '{f[8]}' in room {f[0]}")
    if f[9] not in ("-", ""):
        lock = f[9]
        if lock not in items and lock != "power_restored":
            errs.append(f"rooms: unresolved lock '{lock}' in room {f[0]}")

# ---- connectivity (ignore locks) ----
adj = {int(f[0]): [int(v) for v in f[3:7] if v != "-"] for f in rooms}
seen = {0}
stack = [0]
while stack:
    cur = stack.pop()
    for nxt in adj.get(cur, []):
        if nxt not in seen:
            seen.add(nxt)
            stack.append(nxt)
if len(seen) != len(rooms):
    errs.append(f"rooms: unreachable rooms {sorted(set(range(len(rooms))) - seen)}")

# ---- progression sanity ----
by_id = {int(f[0]): f for f in rooms}
def room_has(room_id, item_id):
    return item_id in [x.strip() for x in by_id[room_id][7].split(",")]

print("items:", len(items), "| enemies:", len(enemies), "| rooms:", len(rooms))
print("file sizes:", {p: os.path.getsize(p) for p in ("data/items.txt", "data/enemies.txt", "data/rooms.txt")})
if errs:
    print("\nPROBLEMS:")
    for e in errs:
        print("  -", e)
else:
    print("Data files are consistent: all ids resolve, exits in range, all rooms reachable.")
