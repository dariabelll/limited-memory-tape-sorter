#pragma once

#include <cstddef>
#include <filesystem>

struct SortedTapePart {
    std::filesystem::path path;
    std::size_t value_count = 0;
};
