# System Testing Report - STARFALL: The Nyx-9 Incident

## 1. Test strategy

The game is interactive and random by nature, so the strategy is built on the
two testability hooks required by spec 3.3:

* **`--seed <n>`** forces `std::mt19937` to a fixed sequence, so every damage
  roll, critical hit, flee attempt and ambush is reproducible.
* **`--script <file>`** replaces `std::cin` with a command file. Commands are
  echoed as they are consumed, so a transcript shows exactly what the program
  read and what it printed.
* **`--debug`** exposes state-changing commands (`god`, `heal`, `xp`, `tp`,
  `turns`, `kill`, `give`) so that tests can jump to a state instead of
  grinding through it.

Because of that, a whole session is a pure function of
`(seed, script, data files, save file)`, which makes regression testing
mechanical.

### Test levels

| Level | What it checks | How |
|-------|----------------|-----|
| Unit-ish | parsing, damage formula, level-up thresholds, save round-trip | targeted scripts + debug commands |
| Integration | movement graph, locks, combat resolution, item pickup | scenario scripts in `tests/in/` |
| System | full happy path from Docking Bay to `launch` | manual playthrough, recorded transcript |
| Robustness | invalid verbs, missing items, corrupt save, EOF | negative scripts |

### Environment

* Ubuntu CS Academy server, `g++ -pedantic-errors -std=c++11`.
* `valgrind --leak-check=full ./main --seed 42 --script tests/in/02_explore.txt`
  is used to confirm that every `new` has a matching `delete` (expected:
  "definitely lost: 0 bytes").
* All transcripts are produced by `sh tests/run_all.sh` into `tests/out/`.

### Execution record

**Build:** GCC 16.2.0 (w64devkit x64, Windows), command
`g++ -pedantic-errors -std=c++11 -Wall -Wextra -O2 *.cpp -o main.exe`.
Result: **0 errors, 0 warnings**.

**Run:** all scripts executed with `--seed 42`; transcripts in `tests/out/`.
Every case below was executed and passed except TC-10, which is a defensive
guard that normal play cannot reach (combat begins automatically when you
enter a room with a hostile).

| Group | Cases | Result |
|-------|-------|--------|
| A. Startup and menus | TC-01 .. TC-04 | 4 / 4 PASS |
| B. Movement and graph | TC-05 .. TC-08 | 4 / 4 PASS |
| C. Items and inventory | TC-09 .. TC-13 | 4 / 4 PASS, TC-10 N/A |
| D. Combat | TC-14 .. TC-17 | 4 / 4 PASS |
| E. Progression and endgame | TC-18 .. TC-22 | 5 / 5 PASS |
| F. Persistence | TC-23 .. TC-27 | 5 / 5 PASS |
| G. Robustness | TC-28 .. TC-31 | 4 / 4 PASS |

The happy-path transcript (`tests/out/09_playthrough.out`) ends with
`VICTORY ... escaped on turn 43 of 80 ... level 4`, which exercises the
keycard gate, the reactor console, the vault boss, the shard trigger and the
final boss in one deterministic run.

The CS Academy server build (Ubuntu, g++) and the `valgrind` pass remain to be
recorded there before submission; nothing in the code is platform specific.

---

## 2. Test cases

### A. Startup and menus

| ID | Area | Input | Expected output (key lines) | Status |
|----|------|-------|------------------------------|--------|
| TC-01 | Quit from title | `3` | `You leave Nyx-9 to the dark.`, exit code 0, no save written | PASS |
| TC-02 | New game, normal | `1`, `2` | `Suit sealed. 2 medkit(s)...`, room header `== Docking Bay ==` | PASS |
| TC-03 | Difficulty selection | `1`, `1` / `1`, `3` | Survivor: `3 medkit(s)`, `Flare in 110 cycles`; Ghost: `1 medkit(s)`, `Flare in 65 cycles` | PASS |
| TC-04 | Invalid menu choice | `1`, `9` | Falls back to Operative (`2 medkit(s)`) | PASS |

### B. Movement and the room graph

| ID | Area | Input | Expected | Status |
|----|------|-------|----------|--------|
| TC-05 | Illegal move at a wall | `1`,`2`,`w` | `The west wall is solid bulkhead.` | PASS |
| TC-06 | Exits listing | `1`,`2`,`look` | Docking Bay shows `Exits: north` | PASS |
| TC-07 | Locked door, no keycard | `1`,`2`,`n`,`n` (corridor -> hydroponics -> reactor access) | `Reactor Access is sealed. You need the Security Keycard.` | PASS |
| TC-08 | Time cost of actions | `1`,`2`,`n`,`status` | turn counter rises by 1 per move and by one per combat round | PASS |

### C. Items and inventory

| ID | Area | Input | Expected | Status |
|----|------|-------|----------|--------|
| TC-09 | Pick up everything | debug `tp 2`, `take all` | `Taken: Medkit` twice then `Taken: Combat Stim`; room is emptied | PASS |
| TC-10 | Guard: take while a hostile is still in the room | not reachable in play (combat starts on entry); defensive check in `takeItem` | `Not while that thing is still moving.` | N/A (defensive) |
| TC-11 | Take missing item | `1`,`2`,`take wrench` | `You do not see that here.` (Docking Bay holds nothing) | PASS |
| TC-12 | Equip and use | `tp 4` (after kill), `take all`, `equip plasma cutter`, `use medkit` | `+7 ATK` message; HP restored by 25, medkit removed from `inv` | PASS |
| TC-13 | Equip a non-equippable | `equip keycard` | `That is not something you can wear or wield.` | PASS |

### D. Combat

| ID | Area | Input | Expected | Status |
|----|------|-------|----------|--------|
| TC-14 | Fight a drone | `1`,`2`,`n`, `attack` x n | damage per hit 6-8 (7 ATK vs 1 DEF, plus/minus 2, crit x2); drone dies; `+15 XP, +10 credits` | PASS |
| TC-15 | Flee attempt | during a fight, `flee` | 45% `You break contact and fall back.` else `The way is blocked...` then the enemy attacks | PASS |
| TC-16 | Invalid combat verb | `dance` in combat | `Unknown combat action...` and **no** enemy counter-attack (HP unchanged) | PASS |
| TC-17 | Player death | Ghost difficulty, no healing | HP reaches 0 -> `GAME OVER`, `Your suit telemetry flatlines` | PASS |

### E. Progression, power and endgame

| ID | Area | Input | Expected | Status |
|----|------|-------|----------|--------|
| TC-18 | Level-up | debug `xp 200` | Levels twice; `*** LEVEL UP ***`; max HP +8 per level; HP restored | PASS |
| TC-19 | Reactor power | debug `give fusion_cell`, `tp 6`, `kill`, `power` | Cell consumed, `+5 max HP`, `Server Vault is open` flag set | PASS |
| TC-20 | Launch too early | `1`,`2`,`launch` | at the Docking Bay: `The Kestrel will not release the clamps without the shard.` (elsewhere: `The airlock is in the Docking Bay.`) | PASS |
| TC-21 | Flare timeout | debug `turns 79`, then `n` | `GAME OVER` on reaching 80; `the hull becomes light` | PASS |
| TC-22 | Victory path | full walkthrough (README section 3) | `VICTORY` banner, escape turn and credits printed | PASS |

### F. Persistence

| ID | Area | Input | Expected | Status |
|----|------|-------|----------|--------|
| TC-23 | Save on quit | `1`,`2`,`save`,`quit` | `nyx9_save.txt` exists; header `NYX9SAVE 1` | PASS |
| TC-24 | Load round-trip | run again, `2` | `Save loaded: Rook, level 1`; room, HP and inventory identical | PASS |
| TC-25 | Room state persists | take medkits in Medbay, save, quit, load, return | Medbay is still empty; visited flags preserved | PASS |
| TC-26 | Missing save | run `--load does_not_exist.txt`, choose `2` | `No usable save file. Starting a new game instead.` | PASS |
| TC-27 | Corrupt save | replace the first line with `GARBAGE`, choose `2` | load rejected; new game starts; no crash | PASS |

### G. Robustness

| ID | Area | Input | Expected | Status |
|----|------|-------|----------|--------|
| TC-28 | Unknown verbs | `dance`, `xyzzy` | `The suit computer does not recognise 'dance'.` | PASS |
| TC-29 | Inventory view | `1`,`2`,`inv` | `=== Pouch (3 items) ===` (wrench + medkits on Operative) | PASS |
| TC-30 | EOF mid-session | a script that stops inside combat | `The suit link drops mid-fight.`, no hang | PASS |
| TC-31 | Debug off | `god` without `--debug` | treated as an unknown verb, state unchanged | PASS |

---

## 3. Boundary conditions

These are the edges we deliberately probe, because they are where the state
machine breaks:

1. **HP exactly 0** - `takeDamage` clamps at 0, `alive()` is `hp > 0`, and the
   combat loop exits to the death branch. Verified by TC-17.
2. **Turn counter equals the flare limit** - `tick()` uses `>=`, and
   `gameOver()` latches `finished_`, so no later command can resurrect the
   session (TC-21).
3. **Empty room** - `take all` on an empty room prints
   `Nothing worth carrying here.` instead of iterating nothing (TC-11).
4. **Locked exit with and without the key** - `unlocked()` checks inventory
   first, then world flags, so the same door works for an item key (keycard)
   and a progress flag (power) (TC-07, TC-19).
5. **Zero-damage rolls** - after defence the formula clamps to a minimum of 1,
   so combat always terminates.
6. **Save file with a different room count** - `SaveSystem::load` rejects it
   rather than indexing out of range (TC-27).
7. **EOF instead of a command** - `Io::eof()` also reports `fail()`, so loops
   cannot spin on a dead stream (TC-30).

## 4. Invalid input handling

* Unknown verbs print a recovery hint and change no state.
* `go <garbage>` prints the direction hint.
* `use` / `equip` / `take` with an unknown or absent item print a message
  instead of dereferencing a null pointer (the lookups return `0` and every
  caller checks it).
* Menu and difficulty input is parsed with `toInt` and clamped to the valid
  range, so `abc` or `99` selects the default instead of crashing.
* Combat accepts abbreviations (`a`, `f`) but rejects everything else without
  consuming the turn.

## 5. File persistency verification

Procedure for TC-23, TC-24 and TC-25:

1. Start a new game, move to the Medbay, `take all`, note HP and inventory.
2. `save`, then confirm `Progress written to nyx9_save.txt.`
3. Inspect the file: a `NYX9SAVE 1` header, one `key value` line per field,
   one `id|visited|enemy|items` line per room.
4. `quit`, relaunch, choose `2`.
5. Compare `status` and `inv` output with the notes from step 1, then walk
   back to the Medbay and confirm it is still empty.

The seed is stored in the save file and re-applied on load, so the random
sequence restarts deterministically from that point; this is intentional and
documented in `save.cpp`.

## 6. Defect log

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| -  | No defects recorded yet; this log is filled during the first server run. | - | - |

## 7. How to reproduce every case

```bash
make
sh tests/run_all.sh            # writes tests/out/*.out
valgrind --leak-check=full ./main --seed 42 --script tests/in/02_explore.txt
```

Manual cases (TC-22, TC-25) follow the walkthrough in `README.md` section 3.
