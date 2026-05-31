#include "TapeVerifier.hpp"

#include "FileTape.hpp"

#include <cstdint>
#include <utility>

TapeVerifier::TapeVerifier(TapeConfig config)
    : config_(std::move(config)) {
}

bool TapeVerifier::is_sorted(const std::filesystem::path& tape_path) const {
    FileTape tape(tape_path, config_, FileTapeMode::open_existing);

    if (tape.valueCount() < 2) {
        return true;
    }

    std::int32_t previous = tape.read();

    for (std::size_t i = 1; i < tape.valueCount(); ++i) {
        tape.moveRight();

        const std::int32_t current = tape.read();

        if (current < previous) {
            return false;
        }

        previous = current;
    }

    return true;
}
