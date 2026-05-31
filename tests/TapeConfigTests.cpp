#include "TapeConfig.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

std::filesystem::path makeTestPath(const std::string& name) {
    return std::filesystem::temp_directory_path() / name;
}

void writeTextFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::trunc);
    output << content;
}

class TapeConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::remove(testFile_);
    }

    void TearDown() override {
        std::filesystem::remove(testFile_);
    }

    std::filesystem::path testFile_ = makeTestPath("limited_memory_tape_config_test.txt");
};

} 

TEST_F(TapeConfigTest, LoadsAllSupportedFields) {
    writeTextFile(
        testFile_,
        "read_delay_ms=1\n"
        "write_delay_ms=2\n"
        "move_delay_ms=3\n"
        "rewind_delay_ms=4\n"
        "memory_limit_bytes=4096\n"
        "tmp_dir=my_tmp\n"
    );

    const TapeConfig config = TapeConfig::loadFromFile(testFile_);

    EXPECT_EQ(config.readDelay, std::chrono::milliseconds(1));
    EXPECT_EQ(config.writeDelay, std::chrono::milliseconds(2));
    EXPECT_EQ(config.moveDelay, std::chrono::milliseconds(3));
    EXPECT_EQ(config.rewindDelay, std::chrono::milliseconds(4));
    EXPECT_EQ(config.memoryLimitBytes, 4096);
    EXPECT_EQ(config.tmpDir, std::filesystem::path("my_tmp"));
}

TEST_F(TapeConfigTest, KeepsDefaultValuesForMissingFields) {
    writeTextFile(
        testFile_,
        "memory_limit_bytes=2048\n"
    );

    const TapeConfig config = TapeConfig::loadFromFile(testFile_);

    EXPECT_EQ(config.readDelay, std::chrono::milliseconds(0));
    EXPECT_EQ(config.writeDelay, std::chrono::milliseconds(0));
    EXPECT_EQ(config.moveDelay, std::chrono::milliseconds(0));
    EXPECT_EQ(config.rewindDelay, std::chrono::milliseconds(0));
    EXPECT_EQ(config.memoryLimitBytes, 2048);
    EXPECT_EQ(config.tmpDir, std::filesystem::path("tmp"));
}

TEST_F(TapeConfigTest, IgnoresEmptyLinesAndComments) {
    writeTextFile(
        testFile_,
        "\n"
        "# tape config\n"
        "read_delay_ms=10\n"
        "\n"
        "# memory limit\n"
        "memory_limit_bytes=8192\n"
    );

    const TapeConfig config = TapeConfig::loadFromFile(testFile_);

    EXPECT_EQ(config.readDelay, std::chrono::milliseconds(10));
    EXPECT_EQ(config.memoryLimitBytes, 8192);
}

TEST_F(TapeConfigTest, UnknownKeyThrows) {
    writeTextFile(
        testFile_,
        "unknown_key=123\n"
    );

    EXPECT_THROW(
        TapeConfig::loadFromFile(testFile_),
        std::runtime_error
    );
}

TEST_F(TapeConfigTest, LineWithoutSeparatorThrows) {
    writeTextFile(
        testFile_,
        "read_delay_ms 10\n"
    );

    EXPECT_THROW(
        TapeConfig::loadFromFile(testFile_),
        std::runtime_error
    );
}

TEST_F(TapeConfigTest, MissingConfigFileThrows) {
    EXPECT_THROW(
        TapeConfig::loadFromFile(testFile_),
        std::runtime_error
    );
}
