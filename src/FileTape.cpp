#include "FileTape.hpp"

#include <stdexcept>

FileTape::FileTape(
    const std::filesystem::path& path,
    const TapeConfig& config,
    bool overwrite
)
    : path_(path),
      config_(config)
{
    std::ios::openmode mode = std::ios::binary | std::ios::in | std::ios::out;

    if (overwrite) {
        mode |= std::ios::trunc;
    }

    file_.open(path_, mode);

    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open tape file: " + path_.string());
    }

    file_.seekg(0, std::ios::end);

    const std::streamoff file_size = file_.tellg();

    if (file_size < 0) {
        throw std::runtime_error("Failed to get tape file size: " + path_.string());
    }

    if (file_size % static_cast<std::streamoff>(sizeof(std::int32_t)) != 0) {
        throw std::runtime_error(
            "Invalid tape file size, expected multiple of int32_t size: " + path_.string()
        );
    }

    valueCount_ = static_cast<std::size_t>(file_size) / sizeof(std::int32_t);

    file_.seekg(0, std::ios::beg);
    file_.seekp(0, std::ios::beg);

    currentPosition_ = 0;
}

std::size_t FileTape::valueCount() const {
    return valueCount_;
}

std::int32_t FileTape::read() {
    throw std::runtime_error("FileTape::read is not implemented yet");
}

void FileTape::write(std::int32_t value) {
    (void)value;
    throw std::runtime_error("FileTape::write is not implemented yet");
}

void FileTape::moveLeft() {
    throw std::runtime_error("FileTape::moveLeft is not implemented yet");
}

void FileTape::moveRight() {
    throw std::runtime_error("FileTape::moveRight is not implemented yet");
}

void FileTape::rewind() {
    throw std::runtime_error("FileTape::rewind is not implemented yet");
}
