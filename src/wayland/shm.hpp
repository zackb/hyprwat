#pragma once

#include <cstddef>
#include <cstdint>
#include <wayland-client.h>

namespace wl {

    class ShmBuffer {
    public:
        ShmBuffer(wl_shm* shm, int width, int height, int stride, uint32_t format);
        ~ShmBuffer();

        wl_buffer* getBuffer() const { return buffer; }
        void* getData() const { return data; }
        int getWidth() const { return width; }
        int getHeight() const { return height; }
        int getStride() const { return stride; }

    private:
        wl_buffer* buffer = nullptr;
        void* data = nullptr;
        int width = 0;
        int height = 0;
        int stride = 0;
        size_t size = 0;
        int fd = -1;
    };

} // namespace wl
