#pragma once

#include "ITape.hpp"
#include "TapeConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>

enum class FileTapeMode {
    open_existing,
    create_or_overwrite
};

class FileTape final : public ITape {
public:
    FileTape(
        const std::filesystem::path& path,
        const TapeConfig& config,
        FileTapeMode mode
    );

    std::int32_t read() override;
    void write(std::int32_t value) override;

    void moveLeft() override;
    void moveRight() override;
    void rewind() override;

    std::size_t valueCount() const override;

private:
    std::filesystem::path path_;
    TapeConfig config_;
    std::fstream file_;

    std::size_t currentPosition_ = 0;
    std::size_t valueCount_ = 0;
};
