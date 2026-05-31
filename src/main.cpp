#include "TapeConfig.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        if (argc != 3 && argc != 4) {
            std::cerr << "Usage: limited-memory-tape-sorter <input_file> <output_file> [config_file]\n";
            return 1;
        }

        const std::string inputPath = argv[1];
        const std::string outputPath = argv[2];
        const std::string configPath = argc == 4 ? argv[3] : "config.txt";

        const TapeConfig config = TapeConfig::loadFromFile(configPath);

        (void)inputPath;
        (void)outputPath;
        (void)config;

        std::cout << "Configuration loaded successfully\n";

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
