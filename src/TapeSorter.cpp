#include "TapeSorter.hpp"

#include "FileTape.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct MergeItem {
    std::int32_t value = 0;
    std::size_t tape_index = 0;
};

struct MergeItemCompare {
    bool operator()(const MergeItem& x, const MergeItem& y) const {
        if (x.value != y.value) {
            return x.value > y.value;
        }

        return x.tape_index > y.tape_index;
    }
};


} 

TapeSorter::TapeSorter(TapeConfig config)
    : config_(std::move(config)) {
}

void TapeSorter::sort(
    const std::filesystem::path& input_path,
    const std::filesystem::path& output_path
) {
    std::filesystem::create_directories(config_.tmpDir);

    std::vector<SortedTapePart> parts =
        split_input_into_sorted_parts(input_path);

    if (parts.empty()) {
        FileTape output_tape(output_path, config_, true);
        return;
    }

    SortedTapePart final_part = merge_until_single_part(std::move(parts));

    copy_to_output_tape(final_part, output_path);

    std::filesystem::remove(final_part.path);
}

std::vector<SortedTapePart> TapeSorter::split_input_into_sorted_parts(
    const std::filesystem::path& input_path
) {
    FileTape input_tape(input_path, config_, false);

    const std::size_t values_per_part =
        config_.memoryLimitBytes / sizeof(std::int32_t);

    if (values_per_part == 0) {
        throw std::runtime_error("memory_limit_bytes is too small");
    }

    std::vector<SortedTapePart> parts;
    std::vector<std::int32_t> buffer;

    buffer.reserve(values_per_part);

    std::size_t values_read = 0;
    std::size_t part_index = 0;

    while (values_read < input_tape.valueCount()) {
        buffer.clear();

        while (
            values_read < input_tape.valueCount() &&
            buffer.size() < values_per_part
        ) {
            buffer.push_back(input_tape.read());
            ++values_read;

            if (values_read < input_tape.valueCount()) {
                input_tape.moveRight();
            }
        }

        std::sort(buffer.begin(), buffer.end());

        const std::filesystem::path part_path =
            make_temp_tape_path("sorted_part", part_index);

        FileTape part_tape(part_path, config_, true);

        for (std::size_t i = 0; i < buffer.size(); ++i) {
            part_tape.write(buffer[i]);

            if (i + 1 < buffer.size()) {
                part_tape.moveRight();
            }
        }

        parts.push_back(SortedTapePart{
            part_path,
            buffer.size()
        });

        ++part_index;
    }

    return parts;
}

SortedTapePart TapeSorter::merge_until_single_part(
    std::vector<SortedTapePart> parts
) {
    if (parts.empty()) {
        throw std::runtime_error("cannot merge empty sorted tape part list");
    }

    if (parts.size() == 1) {
        return parts.front();
    }

    if (config_.memoryLimitBytes < 2 * sizeof(std::int32_t)) {
        throw std::runtime_error("memory_limit_bytes is too small for merge phase");
    }

    const std::size_t bytes_per_active_tape =
        sizeof(MergeItem) + sizeof(std::size_t);

    std::size_t merge_group_size =
        config_.memoryLimitBytes / bytes_per_active_tape;

    if (merge_group_size < 2) {
        merge_group_size = 2;
    }

    const std::size_t max_merge_group_size = 128;

    if (merge_group_size > max_merge_group_size) {
        merge_group_size = max_merge_group_size;
    }

    std::size_t merge_index = 0;

    while (parts.size() > 1) {
        std::vector<SortedTapePart> next_parts;

        for (std::size_t begin = 0; begin < parts.size(); begin += merge_group_size) {
            const std::size_t end = std::min(begin + merge_group_size, parts.size());

            std::vector<SortedTapePart> group(
                parts.begin() + static_cast<std::ptrdiff_t>(begin),
                parts.begin() + static_cast<std::ptrdiff_t>(end)
            );

            if (group.size() == 1) {
                next_parts.push_back(group.front());
                continue;
            }

            SortedTapePart merged_part =
                merge_sorted_part_group(group, merge_index);

            next_parts.push_back(merged_part);
            ++merge_index;

            for (const SortedTapePart& part : group) {
                std::filesystem::remove(part.path);
            }
        }

        parts = std::move(next_parts);
    }

    return parts.front();
}

SortedTapePart TapeSorter::merge_sorted_part_group(
    const std::vector<SortedTapePart>& parts,
    std::size_t merge_index
) {
    if (parts.empty()) {
        throw std::runtime_error("cannot merge empty sorted tape part group");
    }

    if (parts.size() == 1) {
        return parts.front();
    }

    const std::filesystem::path output_path =
        make_temp_tape_path("merged_part", merge_index);

    FileTape output_tape(output_path, config_, true);

    std::vector<std::unique_ptr<FileTape>> input_tapes;
    std::vector<std::size_t> remaining_values;

    input_tapes.reserve(parts.size());
    remaining_values.reserve(parts.size());

    std::size_t total_value_count = 0;

    for (const SortedTapePart& part : parts) {
        input_tapes.push_back(
            std::make_unique<FileTape>(part.path, config_, false)
        );

        remaining_values.push_back(part.value_count);
        total_value_count += part.value_count;
    }

    std::priority_queue<
        MergeItem,
        std::vector<MergeItem>,
        MergeItemCompare
    > heap;

    for (std::size_t i = 0; i < input_tapes.size(); ++i) {
        if (remaining_values[i] == 0) {
            continue;
        }

        heap.push(MergeItem{
            input_tapes[i]->read(),
            i
        });

        --remaining_values[i];

        if (remaining_values[i] > 0) {
            input_tapes[i]->moveRight();
        }
    }

    std::size_t values_written = 0;

    while (!heap.empty()) {
        const MergeItem current = heap.top();
        heap.pop();

        output_tape.write(current.value);
        ++values_written;

        if (values_written < total_value_count) {
            output_tape.moveRight();
        }

        const std::size_t tape_index = current.tape_index;

        if (remaining_values[tape_index] == 0) {
            continue;
        }

        heap.push(MergeItem{
            input_tapes[tape_index]->read(),
            tape_index
        });

        --remaining_values[tape_index];

        if (remaining_values[tape_index] > 0) {
            input_tapes[tape_index]->moveRight();
        }
    }

    return SortedTapePart{
        output_path,
        total_value_count
    };
}

void TapeSorter::copy_to_output_tape(
    const SortedTapePart& sorted_part,
    const std::filesystem::path& output_path
) {
    FileTape input_tape(sorted_part.path, config_, false);
    FileTape output_tape(output_path, config_, true);

    for (std::size_t i = 0; i < sorted_part.value_count; ++i) {
        output_tape.write(input_tape.read());

        if (i + 1 < sorted_part.value_count) {
            input_tape.moveRight();
            output_tape.moveRight();
        }
    }
}

std::filesystem::path TapeSorter::make_temp_tape_path(
    const std::string& prefix,
    std::size_t index
) const {
    return config_.tmpDir / (prefix + "_" + std::to_string(index) + ".bin");
}
