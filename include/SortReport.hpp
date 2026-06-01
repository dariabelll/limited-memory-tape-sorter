#pragma once

#include <cstddef>

struct SortReport {
    std::size_t input_value_count = 0;
    std::size_t output_value_count = 0;

    std::size_t memory_limit_bytes = 0;
    std::size_t max_values_in_memory = 0;

    std::size_t initial_sorted_tape_count = 0;
    std::size_t merged_tape_count = 0;
    std::size_t total_temporary_tape_count = 0;

    std::size_t max_tapes_per_merge = 0;
    std::size_t merge_round_count = 0;
    std::size_t pairwise_merge_round_estimate = 0;
};
