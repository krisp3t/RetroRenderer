#pragma once
#include <memory>
#include <algorithm>

namespace RetroRenderer
{
    template<typename T>
    struct Buffer
    {
        const size_t width, height, pitch;
        std::unique_ptr<T[]> data;

        Buffer() : width(0), height(0), pitch(0) {}

        Buffer(size_t w, size_t h) : width(w), height(h), pitch(w * sizeof(T))
        {
            data = std::make_unique<T[]>(w * h);
        }

        Buffer(size_t w, size_t h, size_t p) : width(w), height(h), pitch(p)
        {
            data = std::make_unique<T[]>(p * h);
        }

        T& operator()(size_t x, size_t y)
        {
            return data[y * pitch + x];
        }

        const T& operator()(size_t x, size_t y) const
        {
            return data[y * pitch + x];
        }

        void Clear()
        {
            std::fill(data.get(), data.get() + width * height, T());
        }

        void Clear(T value)
        {
            std::fill(data.get(), data.get() + width * height, value);
        }

        void Set(size_t x, size_t y, const T value)
        {
            assert(x < width && y < height && "Out of bounds");
            data[y * width + x] = value;
        }

        size_t GetCount() const
        {
            return width * height;
        }

        size_t GetSize() const
        {
            return width * height * sizeof(T);
        }
    };
}