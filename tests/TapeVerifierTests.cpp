#include "TapeConfig.hpp"
#include "TapeVerifier.hpp"
#include "FileTape.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::filesystem::path test_path(const std::string& name) {
    return std::filesystem::temp_directory_path() /
        ("limited_memory_tape_verifier_" + name);
}

void write_values(
    const std::filesystem::path& path,
    const std::vector<std::int32_t>& values
) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    for (std::int32_t value : values) {
        output.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
}

class TapeVerifierTests : public ::testing::Test {
protected:
    void SetUp() override {
        tape_path_ = test_path("tape.bin");
        std::filesystem::remove(tape_path_);
    }

    void TearDown() override {
        std::filesystem::remove(tape_path_);
    }

    std::filesystem::path tape_path_;
    TapeConfig config_;
};

} 

TEST_F(TapeVerifierTests, ReturnsTrueForEmptyTape) {
    write_values(tape_path_, {});

    TapeVerifier verifier(config_);

    EXPECT_TRUE(verifier.is_sorted(tape_path_));
}

TEST_F(TapeVerifierTests, ReturnsTrueForSingleValue) {
    write_values(tape_path_, {42});

    TapeVerifier verifier(config_);

    EXPECT_TRUE(verifier.is_sorted(tape_path_));
}

TEST_F(TapeVerifierTests, ReturnsTrueForSortedTape) {
    write_values(tape_path_, {-10, -1, 0, 3, 3, 5});

    TapeVerifier verifier(config_);

    EXPECT_TRUE(verifier.is_sorted(tape_path_));
}

TEST_F(TapeVerifierTests, ReturnsFalseForUnsortedTape) {
    write_values(tape_path_, {1, 2, 5, 4, 6});

    TapeVerifier verifier(config_);

    EXPECT_FALSE(verifier.is_sorted(tape_path_));
}
