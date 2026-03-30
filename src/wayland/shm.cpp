#include "shm.hpp"
#include "../debug/log.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace wl {

    static int create_shm_file(off_t size) {
        int ret, fd;

        // use memfd_create if available
        fd = syscall(SYS_memfd_create, "hyprwat-shm-buffer", MFD_CLOEXEC | MFD_ALLOW_SEALING);

        if (fd < 0) {
            debug::log(ERR, "Failed to create shm memfd");
            return -1;
        }

        do {
            ret = ftruncate(fd, size);
        } while (ret < 0 && errno == EINTR);

        if (ret < 0) {
            close(fd);
            debug::log(ERR, "Failed to ftruncate shm memfd");
            return -1;
        }

        return fd;
    }

    ShmBuffer::ShmBuffer(wl_shm* shm, int width, int height, int stride, uint32_t format)
        : width(width), height(height), stride(stride) {
        size = stride * height;

        fd = create_shm_file(size);
        if (fd < 0) {
            throw std::runtime_error("Failed to create shm file");
        }

        data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (data == MAP_FAILED) {
            close(fd);
            fd = -1;
            throw std::runtime_error("Failed to mmap shm file");
        }

        wl_shm_pool* pool = wl_shm_create_pool(shm, fd, size);
        if (!pool) {
            munmap(data, size);
            close(fd);
            fd = -1;
            throw std::runtime_error("Failed to create shm pool");
        }

        buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
        wl_shm_pool_destroy(pool);
    }

    ShmBuffer::~ShmBuffer() {
        if (buffer) {
            wl_buffer_destroy(buffer);
        }
        if (data != MAP_FAILED && data != nullptr) {
            munmap(data, size);
        }
        if (fd >= 0) {
            close(fd);
        }
    }

} // namespace wl
