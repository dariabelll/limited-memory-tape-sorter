#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>

struct TapeConfig {
    std::chrono::milliseconds readDelay{0};
    std::chrono::milliseconds writeDelay{0};
    std::chrono::milliseconds moveDelay{0};
    std::chrono::milliseconds rewindDelay{0};

    std::size_t memoryLimitBytes = 1024 * 1024;
    std::filesystem::path tmpDir = "tmp";

    static TapeConfig loadFromFile(const std::filesystem::path& path);
};
