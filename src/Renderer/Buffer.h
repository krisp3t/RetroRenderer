#pragma once
#include <algorithm>
#include <cassert>
#include <cstring>

namespace RetroRenderer {
template <typename T> struct Buffer {
    size_t width = 0;
    size_t height = 0;
    size_t pitch = 0; // Bytes per row.
    T* data = nullptr;

    Buffer() = default;

    Buffer(size_t w, size_t h) : width(w), height(h), pitch(w * sizeof(T)), data(AllocateElements(w * h)) {
    }

    Buffer(size_t w, size_t h, size_t p) : width(w), height(h), pitch(p), data(AllocateElements(w * h)) {
    }

    ~Buffer() {
        delete[] data;
    }

    Buffer(const Buffer& other)
        : width(other.width), height(other.height), pitch(other.pitch), data(AllocateElements(other.GetCount())) {
        if (other.GetCount() > 0) {
            std::copy_n(other.data, other.GetCount(), data);
        }
    }

    Buffer& operator=(const Buffer& other) {
        if (this == &other) {
            return *this;
        }

        if (GetCount() != other.GetCount()) {
            delete[] data;
            data = AllocateElements(other.GetCount());
        }

        width = other.width;
        height = other.height;
        pitch = other.pitch;
        if (other.GetCount() > 0) {
            std::copy_n(other.data, other.GetCount(), data);
        }
        return *this;
    }

    Buffer(Buffer&& other) noexcept : width(other.width), height(other.height), pitch(other.pitch), data(other.data) {
        other.width = 0;
        other.height = 0;
        other.pitch = 0;
        other.data = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        delete[] data;

        width = other.width;
        height = other.height;
        pitch = other.pitch;
        data = other.data;

        other.width = 0;
        other.height = 0;
        other.pitch = 0;
        other.data = nullptr;
        return *this;
    }

    T& operator()(size_t x, size_t y) {
        assert(x < width && y < height && "Out of bounds");
        return data[y * width + x];
    }

    const T& operator()(size_t x, size_t y) const {
        assert(x < width && y < height && "Out of bounds");
        return data[y * width + x];
    }

    void Clear() {
        if (GetCount() == 0) {
            return;
        }
        memset(data, 0, width * height * sizeof(T));
    }

    void Clear(T value) {
        if (GetCount() == 0) {
            return;
        }
        std::fill_n(data, width * height, value);
    }

    void Set(size_t x, size_t y, const T value) {
        assert(x < width && y < height && "Out of bounds");
        data[y * width + x] = value;
    }

    size_t GetCount() const {
        return width * height;
    }

    size_t GetSize() const {
        return width * height * sizeof(T);
    }

  private:
    static T* AllocateElements(size_t count) {
        return count == 0 ? nullptr : new T[count];
    }
};
} // namespace RetroRenderer
