#include "TapeSorter.hpp"

#include "FileTape.hpp"
#include "TemporaryTapeCleaner.hpp"

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

struct CurrentTapeValue {
    std::int32_t value = 0;
    std::size_t tape_index = 0;
};

struct CurrentTapeValueCompare {
    bool operator()(const CurrentTapeValue& x, const CurrentTapeValue& y) const {
        if (x.value != y.value) {
            return x.value > y.value;
        }

        return x.tape_index > y.tape_index;
    }
};


} 

TapeSorter::TapeSorter(TapeConfig config)
    : config_(std::move(config))
{
}

SortReport TapeSorter::sort(
    const std::filesystem::path& input_path,
    const std::filesystem::path& output_path
) {
    std::filesystem::create_directories(config_.tmpDir);

    TemporaryTapeCleaner cleaner;

    SortReport report;
    report.memory_limit_bytes = config_.memoryLimitBytes;
    report.max_values_in_memory =
        config_.memoryLimitBytes / sizeof(std::int32_t);

    std::vector<SortedTapePart> parts =
        split_input_into_sorted_parts(input_path, cleaner);

    report.initial_sorted_tape_count = parts.size();
    report.total_temporary_tape_count = parts.size();

    for (const SortedTapePart& part : parts) {
        report.input_value_count += part.value_count;
    }

    if (parts.empty()) {
        FileTape output_tape(
            output_path,
            config_,
            FileTapeMode::create_or_overwrite
        );

        return report;
    }

    SortedTapePart final_part =
        merge_parts_to_single_tape(std::move(parts), report, cleaner);

    copy_to_output_tape(final_part, output_path);

    report.output_value_count = final_part.value_count;

    cleaner.remove(final_part.path);

    return report;
}

std::vector<SortedTapePart> TapeSorter::split_input_into_sorted_parts(
    const std::filesystem::path& input_path,
    TemporaryTapeCleaner& cleaner
) {
    FileTape input_tape(
        input_path,
        config_,
        FileTapeMode::open_existing
    );

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

        FileTape part_tape(
            part_path,
            config_,
            FileTapeMode::create_or_overwrite
        );

        cleaner.track(part_path);

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

SortedTapePart TapeSorter::merge_parts_to_single_tape(
    std::vector<SortedTapePart> parts,
    SortReport& report,
    TemporaryTapeCleaner& cleaner
) {
    if (parts.empty()) {
        throw std::runtime_error("cannot merge empty sorted tape part list");
    }

    if (parts.size() == 1) {
        report.max_tapes_per_merge = 1;
        report.pairwise_merge_round_estimate = 0;

        return parts.front();
    }

    if (config_.memoryLimitBytes < 2 * sizeof(std::int32_t)) {
        throw std::runtime_error("memory_limit_bytes is too small for merge phase");
    }

    const std::size_t bytes_per_active_tape =
        sizeof(CurrentTapeValue) + sizeof(std::size_t);

    std::size_t max_tapes_per_merge =
        config_.memoryLimitBytes / bytes_per_active_tape;

    if (max_tapes_per_merge < 2) {
        max_tapes_per_merge = 2;
    }

    const std::size_t hard_max_tapes_per_merge = 128;

    if (max_tapes_per_merge > hard_max_tapes_per_merge) {
        max_tapes_per_merge = hard_max_tapes_per_merge;
    }

    report.max_tapes_per_merge = max_tapes_per_merge;

    std::size_t pairwise_parts = parts.size();

    while (pairwise_parts > 1) {
        pairwise_parts = (pairwise_parts + 1) / 2;
        ++report.pairwise_merge_round_estimate;
    }

    std::size_t merge_index = 0;

    while (parts.size() > 1) {
        ++report.merge_round_count;

        std::vector<SortedTapePart> next_parts;

        for (
            std::size_t begin = 0;
            begin < parts.size();
            begin += max_tapes_per_merge
        ) {
            const std::size_t end =
                std::min(begin + max_tapes_per_merge, parts.size());

            std::vector<SortedTapePart> group(
                parts.begin() + static_cast<std::ptrdiff_t>(begin),
                parts.begin() + static_cast<std::ptrdiff_t>(end)
            );

            if (group.size() == 1) {
                next_parts.push_back(group.front());
                continue;
            }

            SortedTapePart merged_part =
                merge_part_group(group, merge_index, cleaner);

            next_parts.push_back(merged_part);

            ++merge_index;
            ++report.merged_tape_count;
            ++report.total_temporary_tape_count;

            for (const SortedTapePart& part : group) {
                cleaner.remove(part.path);
            }
        }

        parts = std::move(next_parts);
    }

    return parts.front();
}

SortedTapePart TapeSorter::merge_part_group(
    const std::vector<SortedTapePart>& parts,
    std::size_t merge_index,
    TemporaryTapeCleaner& cleaner
) {
    if (parts.empty()) {
        throw std::runtime_error("cannot merge empty sorted tape part group");
    }

    if (parts.size() == 1) {
        return parts.front();
    }

    const std::filesystem::path output_path =
        make_temp_tape_path("merged_part", merge_index);

    FileTape output_tape(
        output_path,
        config_,
        FileTapeMode::create_or_overwrite
    );

    cleaner.track(output_path);

    std::vector<std::unique_ptr<FileTape>> input_tapes;
    std::vector<std::size_t> remaining_values;

    input_tapes.reserve(parts.size());
    remaining_values.reserve(parts.size());

    std::size_t total_value_count = 0;

    for (const SortedTapePart& part : parts) {
        input_tapes.push_back(
            std::make_unique<FileTape>(
                part.path,
                config_,
                FileTapeMode::open_existing
            )
        );

        remaining_values.push_back(part.value_count);
        total_value_count += part.value_count;
    }

    std::priority_queue<
        CurrentTapeValue,
        std::vector<CurrentTapeValue>,
        CurrentTapeValueCompare
    > heap;

    for (std::size_t i = 0; i < input_tapes.size(); ++i) {
        if (remaining_values[i] == 0) {
            continue;
        }

        heap.push(CurrentTapeValue{
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
        const CurrentTapeValue current = heap.top();
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

        heap.push(CurrentTapeValue{
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
    FileTape input_tape(
        sorted_part.path,
        config_,
        FileTapeMode::open_existing
    );

    FileTape output_tape(
        output_path,
        config_,
        FileTapeMode::create_or_overwrite
    );

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
