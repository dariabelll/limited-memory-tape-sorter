#include "SortReport.hpp"
#include "TapeConfig.hpp"
#include "TapeSorter.hpp"
#include "TapeVerifier.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void print_report(
    const SortReport& report,
    bool verification_enabled,
    bool verification_passed
) {
    std::cout << "Sorting completed\n\n";

    std::cout << "Tape sorting summary\n";
    std::cout << "--------------------\n";

    std::cout << "Input tape:\n";
    std::cout << "  values: " << report.input_value_count << '\n';

    std::cout << "\nMemory:\n";
    std::cout << "  limit: " << report.memory_limit_bytes << " bytes\n";
    std::cout << "  max loaded values: " << report.max_values_in_memory << '\n';

    std::cout << "\nTemporary tapes:\n";
    std::cout << "  initial sorted tapes: "
              << report.initial_sorted_tape_count << '\n';
    std::cout << "  merged tapes: " << report.merged_tape_count << '\n';
    std::cout << "  total created: "
              << report.total_temporary_tape_count << '\n';

    std::cout << "\nMerge phase:\n";
    std::cout << "  max tapes per merge: "
              << report.max_tapes_per_merge << '\n';
    std::cout << "  merge rounds: " << report.merge_round_count << '\n';
    std::cout << "  pairwise merge would need: "
              << report.pairwise_merge_round_estimate
              << " rounds\n";

    std::cout << "\nOutput tape:\n";
    std::cout << "  values: " << report.output_value_count << '\n';

    if (verification_enabled) {
        std::cout << "  verification: "
                  << (verification_passed ? "OK" : "FAILED")
                  << '\n';
    }
}

} 

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
        const SortReport report = sorter.sort(input_path, output_path);

        bool verification_passed = false;

        if (verify) {
            TapeVerifier verifier(config);
            verification_passed = verifier.is_sorted(output_path);

            if (!verification_passed) {
                print_report(report, true, false);
                return 1;
            }
        }

        print_report(report, verify, verification_passed);

        return 0;

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
