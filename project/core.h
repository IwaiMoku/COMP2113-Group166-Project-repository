#ifndef NYX_CORE_H
#define NYX_CORE_H

#include <string>
#include <vector>
#include <iostream>
#include <random>
#include <cstddef>

namespace nyx {

// ---------------------------------------------------------------------
// Small string utilities used by every text parser in the game.
// ---------------------------------------------------------------------
std::string trim(const std::string& s);
std::string toLower(const std::string& s);

// Splits "a|b||c" into {"a","b","","c"}. Empty fields are preserved so
// that the data files can leave optional columns blank.
std::vector<std::string> split(const std::string& s, char delim);

std::string join(const std::vector<std::string>& parts, const std::string& sep);

bool isNumber(const std::string& s);

// Returns `fallback` instead of throwing on malformed input.
int toInt(const std::string& s, int fallback);

// ---------------------------------------------------------------------
// Deterministic random number generator.
//
// The whole game draws from ONE Random object. Passing --seed makes every
// run byte-for-byte reproducible, which is what the test cases rely on.
// ---------------------------------------------------------------------
class Random {
public:
    explicit Random(unsigned int seed);

    // Inclusive on both ends. Swaps the arguments if high < low.
    int range(int low, int high);

    // True with the given probability (0..100).
    bool chance(int percent);

private:
    std::mt19937 engine_;
};

// ---------------------------------------------------------------------
// Input abstraction: wraps std::cin or a script file.
//
// Script mode ("--script file.txt") is the deterministic command mode:
// commands are echoed so a transcript shows exactly what was typed.
// ---------------------------------------------------------------------
class Io {
public:
    explicit Io(std::istream& in);
    std::string readLine(const std::string& prompt);
    bool eof() const;
    void setEcho(bool enabled);

private:
    std::istream* in_;
    bool echo_;
};

// ---------------------------------------------------------------------
// Singly linked list of journal entries.
//
// Implemented by hand (raw new/delete) to demonstrate manual dynamic
// memory, ownership and a destructor that walks the chain.
// ---------------------------------------------------------------------
class LogList {
public:
    LogList();
    ~LogList();

    void add(const std::string& line);
    std::vector<std::string> recent(int count) const;
    int size() const;

private:
    struct Node {
        std::string text;
        Node* next;
        explicit Node(const std::string& t) : text(t), next(0) {}
    };

    Node* head_;
    Node* tail_;
    int count_;

    // A log is owned by exactly one Game object: copying it would
    // double-free the nodes, so copying is disabled.
    LogList(const LogList&);
    LogList& operator=(const LogList&);
};

// ---------------------------------------------------------------------
// DeckGrid: a heap-allocated 2D char array used to draw the station
// schematic. Owns `rows_` separate heap blocks; therefore it implements
// the rule of three (destructor, copy constructor, assignment).
// ---------------------------------------------------------------------
class DeckGrid {
public:
    DeckGrid(int rows, int cols, char fill);
    DeckGrid(const DeckGrid& other);
    DeckGrid& operator=(const DeckGrid& other);
    ~DeckGrid();

    void set(int row, int col, char value);
    char get(int row, int col) const;
    int rows() const;
    int cols() const;
    std::string render() const;

private:
    void allocate(int rows, int cols, char fill);
    void release();

    char** cells_;
    int rows_;
    int cols_;
};

} // namespace nyx

#endif // NYX_CORE_H
