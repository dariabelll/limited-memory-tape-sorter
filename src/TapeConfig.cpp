#include "TapeConfig.hpp"

#include <chrono>
#include <fstream>
#include <stdexcept>
#include <string>

TapeConfig TapeConfig::loadFromFile(const std::filesystem::path& path) {
    std::ifstream input_file(path);

    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path.string());
    }

    TapeConfig config;
    std::string file_line;
    std::size_t line_number = 0;

    while (std::getline(input_file, file_line)) {
        ++line_number;

        if (file_line.empty() || file_line[0] == '#') {
            continue;
        }

        const std::size_t sep = file_line.find('=');

        if (sep == std::string::npos) {
            throw std::runtime_error(
                "Invalid config line " + std::to_string(line_number) +
                ": expected key=value"
            );
        }

        const std::string key = file_line.substr(0, sep);
        const std::string value = file_line.substr(sep + 1);

        if (key == "read_delay_ms") {
            config.readDelay = std::chrono::milliseconds(std::stoll(value));
        } else if (key == "write_delay_ms") {
            config.writeDelay = std::chrono::milliseconds(std::stoll(value));
        } else if (key == "move_delay_ms") {
            config.moveDelay = std::chrono::milliseconds(std::stoll(value));
        } else if (key == "rewind_delay_ms") {
            config.rewindDelay = std::chrono::milliseconds(std::stoll(value));
        } else if (key == "memory_limit_bytes") {
            config.memoryLimitBytes = static_cast<std::size_t>(std::stoull(value));
        } else if (key == "tmp_dir") {
            config.tmpDir = value;
        } else {
            throw std::runtime_error(
                "Unknown config key on line " + std::to_string(line_number) +
                ": " + key
            );
        }
    }

    return config;
}
