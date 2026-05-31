#include "FileTape.hpp"
#include "TapeConfig.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

std::filesystem::path makeTestPath(const std::string& name) {
    return std::filesystem::temp_directory_path() / name;
}

void writeRawValues(
    const std::filesystem::path& path,
    const std::vector<std::int32_t>& values
) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    for (std::int32_t value : values) {
        output.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
}

class FileTapeTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::filesystem::remove(testFile_);
    }

    void TearDown() override {
        std::filesystem::remove(testFile_);
    }

    std::filesystem::path testFile_ = makeTestPath("limited_memory_file_tape_test.bin");
    TapeConfig config_;
};

} 

TEST_F(FileTapeTest, NewOverwrittenTapeIsEmpty) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    EXPECT_EQ(tape.valueCount(), 0);
}

TEST_F(FileTapeTest, OpensExistingTapeAndCountsValues) {
    writeRawValues(testFile_, {10, 20, 30, 40});

    FileTape tape(testFile_, config_, FileTapeMode::open_existing);

    EXPECT_EQ(tape.valueCount(), 4);
}

TEST_F(FileTapeTest, ReadsValuesSequentiallyWhenMovedRight) {
    writeRawValues(testFile_, {10, 20, 30});

    FileTape tape(testFile_, config_, FileTapeMode::open_existing);

    EXPECT_EQ(tape.read(), 10);

    tape.moveRight();
    EXPECT_EQ(tape.read(), 20);

    tape.moveRight();
    EXPECT_EQ(tape.read(), 30);
}

TEST_F(FileTapeTest, ReadDoesNotMoveHead) {
    writeRawValues(testFile_, {10, 20});

    FileTape tape(testFile_, config_, FileTapeMode::open_existing);

    EXPECT_EQ(tape.read(), 10);
    EXPECT_EQ(tape.read(), 10);

    tape.moveRight();

    EXPECT_EQ(tape.read(), 20);
    EXPECT_EQ(tape.read(), 20);
}

TEST_F(FileTapeTest, WritesValuesAndReadsThemBack) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    tape.write(10);
    tape.moveRight();

    tape.write(20);
    tape.moveRight();

    tape.write(30);

    EXPECT_EQ(tape.valueCount(), 3);

    tape.rewind();

    EXPECT_EQ(tape.read(), 10);
    tape.moveRight();

    EXPECT_EQ(tape.read(), 20);
    tape.moveRight();

    EXPECT_EQ(tape.read(), 30);
}

TEST_F(FileTapeTest, WriteDoesNotMoveHead) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    tape.write(10);
    tape.write(20);

    EXPECT_EQ(tape.valueCount(), 1);

    tape.rewind();

    EXPECT_EQ(tape.read(), 20);
}

TEST_F(FileTapeTest, CanOverwriteExistingValue) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    tape.write(10);
    tape.moveRight();
    tape.write(20);

    tape.rewind();
    tape.moveRight();

    tape.write(99);

    tape.rewind();

    EXPECT_EQ(tape.read(), 10);
    tape.moveRight();

    EXPECT_EQ(tape.read(), 99);
    EXPECT_EQ(tape.valueCount(), 2);
}

TEST_F(FileTapeTest, MoveLeftMovesToPreviousCell) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    tape.write(10);
    tape.moveRight();
    tape.write(20);

    EXPECT_EQ(tape.read(), 20);

    tape.moveLeft();

    EXPECT_EQ(tape.read(), 10);
}

TEST_F(FileTapeTest, RewindMovesToFirstCell) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    tape.write(10);
    tape.moveRight();
    tape.write(20);
    tape.moveRight();
    tape.write(30);

    EXPECT_EQ(tape.read(), 30);

    tape.rewind();

    EXPECT_EQ(tape.read(), 10);
}

TEST_F(FileTapeTest, MoveLeftFromBeginningThrows) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    EXPECT_THROW(tape.moveLeft(), std::runtime_error);
}

TEST_F(FileTapeTest, ReadFromEmptyTapeThrows) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    EXPECT_THROW(tape.read(), std::runtime_error);
}

TEST_F(FileTapeTest, ReadPastEndThrows) {
    FileTape tape(testFile_, config_, FileTapeMode::create_or_overwrite);

    tape.write(10);
    tape.moveRight();

    EXPECT_THROW(tape.read(), std::runtime_error);
}

TEST_F(FileTapeTest, OpeningMissingExistingTapeThrows) {
    EXPECT_THROW(
        FileTape tape(testFile_, config_, FileTapeMode::open_existing),
        std::runtime_error
    );
}

TEST_F(FileTapeTest, InvalidBinaryFileSizeThrows) {
    {
        std::ofstream output(testFile_, std::ios::binary | std::ios::trunc);
        const char bytes[3] = {'a', 'b', 'c'};
        output.write(bytes, 3);
    }

    EXPECT_THROW(
        FileTape tape(testFile_, config_, FileTapeMode::open_existing),
        std::runtime_error
    );
}
