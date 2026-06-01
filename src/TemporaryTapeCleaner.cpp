#include "TemporaryTapeCleaner.hpp"

#include <algorithm>
#include <system_error>

TemporaryTapeCleaner::~TemporaryTapeCleaner() {
    for (const std::filesystem::path& path : paths_) {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
}

void TemporaryTapeCleaner::track(const std::filesystem::path& path) {
    paths_.push_back(path);
}

void TemporaryTapeCleaner::remove(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::remove(path, error);

    forget(path);
}

void TemporaryTapeCleaner::dismiss() {
    paths_.clear();
}

void TemporaryTapeCleaner::forget(const std::filesystem::path& path) {
    const auto new_end = std::remove(paths_.begin(), paths_.end(), path);
    paths_.erase(new_end, paths_.end());
}
