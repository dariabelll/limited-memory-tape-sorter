#include "TapeConfig.hpp"
#include "TapeSorter.hpp"
#include "TapeVerifier.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        if (argc < 3 || argc > 5) {
            std::cerr
                << "Usage: limited-memory-tape-sorter "
                << "<input_file> <output_file> [config_file] [--verify]\n";

            return 1;
        }

        const std::filesystem::path input_path = argv[1];
        const std::filesystem::path output_path = argv[2];

        std::filesystem::path config_path = "config.txt";
        bool verify = false;

        if (argc >= 4) {
            const std::string third_argument = argv[3];

            if (third_argument == "--verify") {
                verify = true;
            } else {
                config_path = third_argument;
            }
        }

        if (argc == 5) {
            const std::string fourth_argument = argv[4];

            if (fourth_argument != "--verify") {
                std::cerr << "Unknown argument: " << fourth_argument << '\n';
                return 1;
            }

            verify = true;
        }

        const TapeConfig config = TapeConfig::loadFromFile(config_path);

        TapeSorter sorter(config);
        sorter.sort(input_path, output_path);

        std::cout << "Sorting completed\n";

        if (verify) {
            TapeVerifier verifier(config);

            if (!verifier.is_sorted(output_path)) {
                std::cerr << "Verification failed: output tape is not sorted\n";
                return 1;
            }

            std::cout << "Verification completed\n";
        }

        return 0;

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
