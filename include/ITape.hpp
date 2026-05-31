#pragma once

#include <cstddef>
#include <cstdint>

class ITape {
public:
    virtual ~ITape() = default;

    virtual std::int32_t read() = 0;
    virtual void write(std::int32_t value) = 0;

    virtual void moveLeft() = 0;
    virtual void moveRight() = 0;
    virtual void rewind() = 0;

    virtual std::size_t valueCount() const = 0;
};
