#include "TapeConfig.hpp"
#include "TapeSorter.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        
        if (argc != 3 && argc != 4) {
            std::cerr
                << "Usage: limited-memory-tape-sorter "
                << "<input_file> <output_file> [config_file]\n";

            return 1;
        }

        const std::filesystem::path input_path = argv[1];
        const std::filesystem::path output_path = argv[2];

        const std::filesystem::path config_path = argc == 4 ? argv[3] : "config.txt";

        const TapeConfig config = TapeConfig::loadFromFile(config_path);

        TapeSorter sorter(config);
        sorter.sort(input_path, output_path);

        std::cout << "Sorting completed\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
