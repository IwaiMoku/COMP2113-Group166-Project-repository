#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "game.h"

namespace {

void printUsage(const char* program) {
    std::cout << "STARFALL: The Nyx-9 Incident\n\n";
    std::cout << "Usage: " << program << " [options]\n\n";
    std::cout << "  --seed <n>       fixed RNG seed (reproducible runs)\n";
    std::cout << "  --script <file>  read commands from a file (deterministic mode)\n";
    std::cout << "  --load <file>    load a save file at startup\n";
    std::cout << "  --debug          enable debug commands\n";
    std::cout << "  --help           show this message\n";
}

} // namespace

int main(int argc, char** argv) {
    nyx::Options options;
    std::string scriptPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--seed" && i + 1 < argc) {
            options.seed = static_cast<unsigned int>(std::atoi(argv[++i]));
            options.useSeed = true;
        } else if (arg == "--script" && i + 1 < argc) {
            scriptPath = argv[++i];
            options.scriptPath = scriptPath;
        } else if (arg == "--load" && i + 1 < argc) {
            options.loadPath = argv[++i];
        } else if (arg == "--debug") {
            options.debug = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cout << "Unknown option: " << arg << "\n\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    std::ifstream script;
    std::istream* input = &std::cin;
    if (!scriptPath.empty()) {
        script.open(scriptPath.c_str());
        if (!script) {
            std::cerr << "Could not open script file: " << scriptPath << "\n";
            return 1;
        }
        input = &script;
    }

    nyx::Game game(options, *input);
    return game.run();
}
