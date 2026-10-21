#include <sys/mman.h>
#include <cerrno>
#include <cstdio>

// Standalone, no Qt: exercise the same successful-MCL_FUTURE/late-mmap
// failure pattern without opening any device or writing a disc.
int main() {
    constexpr unsigned long size = 16UL * 1024 * 1024;
    const bool locked = mlockall(MCL_CURRENT | MCL_FUTURE) == 0;
    void* memory = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    const bool allocated = memory != MAP_FAILED;
    const int error = allocated ? 0 : errno;
    munlockall();
    if (allocated) munmap(memory, size);
    std::printf("locked=%s allocated=%s errno=%d\n",
                locked ? "yes" : "no", allocated ? "yes" : "no", error);
    return allocated ? 0 : 2;
}
