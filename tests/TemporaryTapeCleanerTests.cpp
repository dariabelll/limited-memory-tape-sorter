#include "TemporaryTapeCleaner.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path test_path(const std::string& name) {
    return std::filesystem::temp_directory_path() /
        ("limited_memory_tape_cleaner_" + name);
}

void create_file(const std::filesystem::path& path) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << "temporary";
}

} 

TEST(TemporaryTapeCleanerTests, RemovesTrackedFilesOnDestruction) {
    const std::filesystem::path path = test_path("tracked_file.bin");

    std::filesystem::remove(path);
    create_file(path);

    ASSERT_TRUE(std::filesystem::exists(path));

    {
        TemporaryTapeCleaner cleaner;
        cleaner.track(path);
    }

    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(TemporaryTapeCleanerTests, RemoveDeletesFileImmediately) {
    const std::filesystem::path path = test_path("removed_file.bin");

    std::filesystem::remove(path);
    create_file(path);

    ASSERT_TRUE(std::filesystem::exists(path));

    TemporaryTapeCleaner cleaner;
    cleaner.track(path);
    cleaner.remove(path);

    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(TemporaryTapeCleanerTests, DismissKeepsTrackedFiles) {
    const std::filesystem::path path = test_path("dismissed_file.bin");

    std::filesystem::remove(path);
    create_file(path);

    ASSERT_TRUE(std::filesystem::exists(path));

    {
        TemporaryTapeCleaner cleaner;
        cleaner.track(path);
        cleaner.dismiss();
    }

    EXPECT_TRUE(std::filesystem::exists(path));

    std::filesystem::remove(path);
}
