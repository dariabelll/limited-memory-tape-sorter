#include "FileTape.hpp"

#include <stdexcept>
#include <thread> 

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

    if (currentPosition_ >= valueCount_) {
        throw std::runtime_error("Cannot read past the end of tape: " + path_.string());
    }

    std::this_thread::sleep_for(config_.readDelay);

    const auto byte_offset = static_cast<std::streamoff>(
        currentPosition_ * sizeof(std::int32_t)
    );

    file_.clear();
    file_.seekg(byte_offset);

    std::int32_t value = 0;
    file_.read(reinterpret_cast<char*>(&value), sizeof(value));

    if (!file_) {
        throw std::runtime_error("Failed to read from tape: " + path_.string());
    }

    return value;
}

void FileTape::write(std::int32_t value) {
    if (currentPosition_ > valueCount_) {
        throw std::runtime_error("Cannot write past the end of tape: " + path_.string());
    }

    std::this_thread::sleep_for(config_.writeDelay);

    const auto byte_offset = static_cast<std::streamoff>(
        currentPosition_ * sizeof(std::int32_t)
    );

    file_.clear();
    file_.seekp(byte_offset, std::ios::beg);

    file_.write(reinterpret_cast<const char*>(&value), sizeof(value));
    file_.flush();

    if (!file_) {
        throw std::runtime_error("Failed to write to tape: " + path_.string());
    }

    if (currentPosition_ == valueCount_) {
        ++valueCount_;
    }
}

void FileTape::moveLeft() {
    if (currentPosition_ == 0) {
        throw std::runtime_error("Cannot move left from the beginning of tape: " + path_.string());
    }

    std::this_thread::sleep_for(config_.moveDelay);

    --currentPosition_;
}

void FileTape::moveRight() {
    if (currentPosition_ >= valueCount_) {
        throw std::runtime_error("Cannot move right past the end of tape: " + path_.string());
    }

    std::this_thread::sleep_for(config_.moveDelay);

    ++currentPosition_;
}

void FileTape::rewind() {
    std::this_thread::sleep_for(config_.rewindDelay);

    currentPosition_ = 0;
}
