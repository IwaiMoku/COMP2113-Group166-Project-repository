# AI Usage Declaration and Log

## 1. Declaration

AI tools were used during this project for brainstorming, for learning the
non-syllabus C++ topics, for drafting parts of the implementation, and for
designing test cases. Every AI-generated suggestion was read, compiled,
executed and reviewed by a team member before it was committed.

| Tool | Model / version | Used for |
|------|-----------------|----------|
| DeepSeek Harness (chat agent) | DeepSeek chat model | architecture, code drafting, documentation |
| (add) | | |
| (add) | | |

No AI tool was given repository write access. All commits were made by team
members, and each commit message notes whether the code was AI-assisted
(`[ai-draft]`, `[ai-review]`) or written by hand.

## 2. Workflow integration

```
idea --> AI dialogue --> draft (markdown) --> team review --> compile --> test --> commit
                                  |
                                  +--> rejected ideas logged below
```

Rules the team agreed on:

1. An AI answer is a *proposal*, never a fact. Anything about the C++ standard
   is checked against cppreference or the compiler.
2. AI-drafted code is only accepted once it compiles with
   `-pedantic-errors -std=c++11` and passes the relevant scripted test.
3. Comments in committed code explain *why*, not *what*; AI boilerplate
   comments were rewritten or removed.
4. Any AI-suggested test case is executed before it enters the testing report.

## 3. Prompt history (abridged)

The full transcript is attached to the Moodle submission; this is the
representative subset with what we actually did with each answer.

### Session 1 - architecture

> **Prompt:** "We are building a text RPG in C++11, limited to one main.cpp
> plus headers, with polymorphism, dynamic memory, file I/O and a save file.
> Suggest a file split that keeps headers free of implementation."

*Used:* the six-module split (`core`, `item`, `entity`, `world`, `combat`,
`game`) and the idea of a `SaveMeta` struct to carry non-player state.

*Changed:* the AI proposed `std::optional` for lookups. That is C++17, and the
brief requires C++11, so the team replaced it with a `const Item*` returning
nullptr plus null checks at every call site.

### Session 2 - non-syllabus topic

> **Prompt:** "Explain `std::unique_ptr` and RAII with a small example of an
> inventory that owns items, and list the mistakes beginners make."

*Used:* the FNV-1a hash functor, the rule-of-three explanation for `DeckGrid`,
and the `std::move` ownership transfer into `Combat`.

*Changed:* the AI's first version used `std::make_unique`. It does not exist in
C++11, so it was rewritten as `std::unique_ptr<T>(new T(...))` in every place.

### Session 3 - game loop and commands

> **Prompt:** "Design a command dispatcher for a text game with verbs,
> arguments and aliases, without a giant if-chain if possible."

*Used:* the verb/argument split on the first space, and the alias table.

*Rejected:* a `std::map<std::string, std::function<void()>>` dispatcher - it
captures `this` and makes the loop harder to read for a first-year project. The
team kept a readable `if/else if` chain in `Game::processCommand`.

### Session 4 - persistence format

> **Prompt:** "Suggest a human-readable save format for a small RPG that is
> easy to parse in C++11 without third-party libraries."

*Used:* the line-oriented `key value` format and the `id|visited|enemy|items`
room rows (same separators as the data files).

*Changed:* the AI suggested JSON with `nlohmann/json`. That is a header-only
library, but it would have to be vendored and increases the build risk on the
CS Academy server, so the team kept plain text.

### Session 5 - test design

> **Prompt:** "List boundary cases worth testing in a turn-based game with a
> save/load system and a turn limit."

*Used:* most of the boundary list in `TESTING_REPORT.md` section 3 (HP = 0,
turn = limit, empty inventory, corrupt save, EOF).

*Changed:* the AI suggested fuzzing with random bytes. The team replaced it
with four hand-written negative scripts because fuzzing output is not
reviewable.

## 4. Critical reflection: what we verified and what we did not

**What worked.** The AI was strongest at structure: proposing the module
boundaries, naming the functions, and producing the first draft of repetitive
code (the `key|value` parsing, the enemy subclasses). It also explained FNV-1a
and `mt19937` clearly enough that we could defend the design in the demo.

**What we had to catch.** In every session the AI assumed a newer standard
(`make_unique`, `std::optional`, `auto` return types) or a third-party library.
Each of those would have failed the `-std=c++11 -pedantic-errors` build. This
became our standard review question: *"does this compile on the server, with
these exact flags?"*

**A concrete bug we found by reading, not by running.** The first draft of
`Combat::run()` looped on `while (player_.alive() && enemy_->alive())` and
`continue`d on empty input. With a truncated script, `getline` returns an
empty string at EOF, so the loop would spin forever. We changed `Io::eof()` to
report `fail()` as well and added an explicit EOF branch inside combat. The AI
had not considered EOF at all; it only appears when you test with files.

**What we could not verify.** The development machine used for drafting had no
C++ compiler, so the code was reviewed statically (brace balance, every
declared member matched against its definition, scan for C++14/17 features)
before the first server build. Compilation itself, the `valgrind` run and the
transcripts in `tests/out/` are recorded on the CS Academy server by a team
member; those results, not the AI's claims, are what the testing report
reports.

**Division of labour.** Each team member owns at least one module end to end
(header, implementation, tests, documentation). AI drafts were never merged
without the owning member being able to explain every line.

## 5. Academic integrity statement

We understand that submitting code we cannot explain is a breach of the
University's academic honesty policy. The AI Usage Log exists so that the
teaching team can see exactly which parts were assisted and how they were
verified. All committed code has been read, compiled and tested by the team.
