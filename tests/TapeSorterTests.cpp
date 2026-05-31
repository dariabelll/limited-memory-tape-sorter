#include "TapeConfig.hpp"
#include "TapeSorter.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void write_values(
    const std::filesystem::path& path,
    const std::vector<std::int32_t>& values
) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    for (std::int32_t value : values) {
        output.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
}

std::vector<std::int32_t> read_values(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);

    std::vector<std::int32_t> values;

    std::int32_t value = 0;
    while (input.read(reinterpret_cast<char*>(&value), sizeof(value))) {
        values.push_back(value);
    }

    return values;
}

class TapeSorterTests : public ::testing::Test {
protected:
    void SetUp() override {
        input_path_ = make_test_path("input.bin");
        output_path_ = make_test_path("output.bin");
        tmp_dir_ = make_test_path("tmp");

        cleanup();

        config_.memoryLimitBytes = 8;
        config_.tmpDir = tmp_dir_;
    }

    void TearDown() override {
        cleanup();
    }

    std::filesystem::path make_test_path(const std::string& file_name) const {
        return std::filesystem::temp_directory_path() /
            ("limited_memory_tape_sorter_" + file_name);
    }

    void cleanup() const {
        std::filesystem::remove(input_path_);
        std::filesystem::remove(output_path_);
        std::filesystem::remove_all(tmp_dir_);
    }

    void sort_and_check(
        const std::vector<std::int32_t>& input_values,
        const std::vector<std::int32_t>& expected_values
    ) {
        write_values(input_path_, input_values);

        TapeSorter sorter(config_);
        sorter.sort(input_path_, output_path_);

        EXPECT_EQ(read_values(output_path_), expected_values);
    }

    std::filesystem::path input_path_;
    std::filesystem::path output_path_;
    std::filesystem::path tmp_dir_;

    TapeConfig config_;
};

} // namespace

TEST_F(TapeSorterTests, SortsEmptyTape) {
    sort_and_check(
        {},
        {}
    );
}

TEST_F(TapeSorterTests, SortsSingleValue) {
    sort_and_check(
        {42},
        {42}
    );
}

TEST_F(TapeSorterTests, SortsAlreadySortedValues) {
    sort_and_check(
        {1, 2, 3, 4, 5},
        {1, 2, 3, 4, 5}
    );
}

TEST_F(TapeSorterTests, SortsReverseSortedValues) {
    sort_and_check(
        {5, 4, 3, 2, 1},
        {1, 2, 3, 4, 5}
    );
}

TEST_F(TapeSorterTests, SortsValuesSplitIntoManyParts) {
    sort_and_check(
        {7, 1, 5, 3, 9, 2, 8, 4, 6, 0},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
    );
}

TEST_F(TapeSorterTests, SortsNegativeValues) {
    sort_and_check(
        {-3, 10, 0, -1, 5, -10},
        {-10, -3, -1, 0, 5, 10}
    );
}

TEST_F(TapeSorterTests, SortsValuesWithDuplicates) {
    sort_and_check(
        {4, 1, 3, 1, 2, 4, 3, 2},
        {1, 1, 2, 2, 3, 3, 4, 4}
    );
}

TEST_F(TapeSorterTests, SortsWhenMemoryFitsOnlyTwoValues) {
    config_.memoryLimitBytes = 2 * sizeof(std::int32_t);

    sort_and_check(
        {100, -1, 50, 20, 20, 0, -10},
        {-10, -1, 0, 20, 20, 50, 100}
    );
}

TEST_F(TapeSorterTests, ThrowsWhenMemoryCannotStoreOneValue) {
    write_values(input_path_, {3, 2, 1});

    config_.memoryLimitBytes = sizeof(std::int32_t) - 1;

    TapeSorter sorter(config_);

    EXPECT_THROW(
        sorter.sort(input_path_, output_path_),
        std::runtime_error
    );
}
