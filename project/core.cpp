#include "core.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace nyx {

std::string trim(const std::string& s) {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }
    return s.substr(begin, end - begin);
}

std::string toLower(const std::string& s) {
    std::string out = s;
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::string current;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) {
            parts.push_back(current);
            current.clear();
        } else {
            current += s[i];
        }
    }
    parts.push_back(current);
    return parts;
}

std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out += sep;
        }
        out += parts[i];
    }
    return out;
}

bool isNumber(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    std::size_t i = 0;
    if (s[0] == '-' || s[0] == '+') {
        i = 1;
    }
    if (i >= s.size()) {
        return false;
    }
    for (; i < s.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(s[i])) == 0) {
            return false;
        }
    }
    return true;
}

int toInt(const std::string& s, int fallback) {
    if (!isNumber(s)) {
        return fallback;
    }
    return std::atoi(s.c_str());
}

// ---------------------------------------------------------------- Random

Random::Random(unsigned int seed) : engine_(seed) {}

int Random::range(int low, int high) {
    if (high < low) {
        int tmp = low;
        low = high;
        high = tmp;
    }
    std::uniform_int_distribution<int> dist(low, high);
    return dist(engine_);
}

bool Random::chance(int percent) {
    if (percent <= 0) {
        return false;
    }
    if (percent >= 100) {
        return true;
    }
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(engine_) <= percent;
}

// ---------------------------------------------------------------- Io

Io::Io(std::istream& in) : in_(&in), echo_(false) {}

std::string Io::readLine(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();

    std::string line;
    if (!std::getline(*in_, line)) {
        return std::string();
    }
    if (echo_) {
        std::string shown = trim(line);
        if (!shown.empty()) {
            std::cout << shown << "\n";
        }
    }
    return line;
}

bool Io::eof() const {
    // A stream that failed for any reason is treated as exhausted, so the
    // game loops can never spin forever on a dead input.
    return in_->eof() || in_->fail();
}

void Io::setEcho(bool enabled) {
    echo_ = enabled;
}

// ---------------------------------------------------------------- LogList

LogList::LogList() : head_(0), tail_(0), count_(0) {}

LogList::~LogList() {
    Node* current = head_;
    while (current != 0) {
        Node* next = current->next;
        delete current;
        current = next;
    }
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

void LogList::add(const std::string& line) {
    Node* node = new Node(line);
    if (tail_ == 0) {
        head_ = node;
        tail_ = node;
    } else {
        tail_->next = node;
        tail_ = node;
    }
    ++count_;
}

std::vector<std::string> LogList::recent(int count) const {
    std::vector<std::string> out;
    if (count <= 0) {
        return out;
    }
    int skip = count_ - count;
    if (skip < 0) {
        skip = 0;
    }
    const Node* current = head_;
    int index = 0;
    while (current != 0) {
        if (index >= skip) {
            out.push_back(current->text);
        }
        current = current->next;
        ++index;
    }
    return out;
}

int LogList::size() const {
    return count_;
}

// ---------------------------------------------------------------- DeckGrid

DeckGrid::DeckGrid(int rows, int cols, char fill) : cells_(0), rows_(0), cols_(0) {
    allocate(rows, cols, fill);
}

DeckGrid::DeckGrid(const DeckGrid& other) : cells_(0), rows_(0), cols_(0) {
    allocate(other.rows_, other.cols_, ' ');
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            cells_[r][c] = other.cells_[r][c];
        }
    }
}

DeckGrid& DeckGrid::operator=(const DeckGrid& other) {
    if (this != &other) {
        release();
        allocate(other.rows_, other.cols_, ' ');
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                cells_[r][c] = other.cells_[r][c];
            }
        }
    }
    return *this;
}

DeckGrid::~DeckGrid() {
    release();
}

void DeckGrid::allocate(int rows, int cols, char fill) {
    if (rows < 1) {
        rows = 1;
    }
    if (cols < 1) {
        cols = 1;
    }
    rows_ = rows;
    cols_ = cols;
    cells_ = new char*[rows_];
    for (int r = 0; r < rows_; ++r) {
        cells_[r] = new char[cols_];
        for (int c = 0; c < cols_; ++c) {
            cells_[r][c] = fill;
        }
    }
}

void DeckGrid::release() {
    if (cells_ == 0) {
        return;
    }
    for (int r = 0; r < rows_; ++r) {
        delete[] cells_[r];
    }
    delete[] cells_;
    cells_ = 0;
    rows_ = 0;
    cols_ = 0;
}

void DeckGrid::set(int row, int col, char value) {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        return;
    }
    cells_[row][col] = value;
}

char DeckGrid::get(int row, int col) const {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        return ' ';
    }
    return cells_[row][col];
}

int DeckGrid::rows() const {
    return rows_;
}

int DeckGrid::cols() const {
    return cols_;
}

std::string DeckGrid::render() const {
    std::string out;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            out += cells_[r][c];
            if (c + 1 < cols_) {
                out += ' ';
            }
        }
        out += '\n';
    }
    return out;
}

} // namespace nyx
