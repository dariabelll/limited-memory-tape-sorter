#pragma once

#include <filesystem>
#include <vector>

class TemporaryTapeCleaner {
public:
    TemporaryTapeCleaner() = default;

    TemporaryTapeCleaner(const TemporaryTapeCleaner&) = delete;
    TemporaryTapeCleaner& operator=(const TemporaryTapeCleaner&) = delete;

    TemporaryTapeCleaner(TemporaryTapeCleaner&&) = delete;
    TemporaryTapeCleaner& operator=(TemporaryTapeCleaner&&) = delete;

    ~TemporaryTapeCleaner();

    void track(const std::filesystem::path& path);
    void remove(const std::filesystem::path& path);
    void dismiss();

private:
    void forget(const std::filesystem::path& path);

private:
    std::vector<std::filesystem::path> paths_;
};
