#pragma once

#include "TapeConfig.hpp"

#include <filesystem>

class TapeVerifier {
public:
    explicit TapeVerifier(TapeConfig config);

    bool is_sorted(const std::filesystem::path& tape_path) const;

private:
    TapeConfig config_;
};
