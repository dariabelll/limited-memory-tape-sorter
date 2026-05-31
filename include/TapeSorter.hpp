#pragma once

#include "SortedTapePart.hpp"
#include "TapeConfig.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

class TapeSorter {
public:
    explicit TapeSorter(TapeConfig config);

    void sort(
        const std::filesystem::path& input_path,
        const std::filesystem::path& output_path
    );

private:
    std::vector<SortedTapePart> split_input_into_sorted_parts(
        const std::filesystem::path& input_path
    );

    SortedTapePart merge_until_single_part(
        std::vector<SortedTapePart> parts
    );

    SortedTapePart merge_sorted_part_group(
        const std::vector<SortedTapePart>& parts,
        std::size_t merge_index
    );

    void copy_to_output_tape(
        const SortedTapePart& sorted_part,
        const std::filesystem::path& output_path
    );

    std::filesystem::path make_temp_tape_path(
        const std::string& prefix,
        std::size_t index
    ) const;

private:
    TapeConfig config_;
};
