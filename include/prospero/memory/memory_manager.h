#pragma once

#include "prospero/common/types.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace prospero::memory {

struct MemoryRegion {
    vaddr_t guest_vaddr{0};
    size_t size{0};
    u8* host_ptr{nullptr};
    u32 protection{PROSPERO_PROT_NONE};
    bool is_direct{false};
    paddr_t direct_paddr{0};
    std::string tag;
};

struct DirectMemoryBlock {
    paddr_t paddr{0};
    size_t size{0};
    u8* host_ptr{nullptr};
    bool is_allocated{false};
};

class MemoryManager {
public:
    static MemoryManager& instance() {
        static MemoryManager s_instance;
        return s_instance;
    }

    bool init();
    void shutdown();

    // Virtual memory mapping
    vaddr_t allocateVirtual(vaddr_t preferred_addr, size_t size, u32 prot, const std::string& tag = "");
    bool freeVirtual(vaddr_t addr, size_t size);
    bool protectVirtual(vaddr_t addr, size_t size, u32 new_prot);

    // Direct Memory management (SCE Physical GDDR6 pools)
    s32 allocateDirectMemory(off_t search_start, off_t search_end, size_t length, size_t alignment, s32 type, off_t* out_paddr);
    s32 releaseDirectMemory(off_t paddr, size_t length);
    s32 mapDirectMemory(void** out_addr, size_t length, s32 prot, s32 flags, off_t paddr, size_t alignment);

    // Address translation and memory read/write
    template <typename T = void>
    T* getPointer(vaddr_t guest_addr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& reg : m_regions) {
            if (guest_addr >= reg.guest_vaddr && guest_addr < reg.guest_vaddr + reg.size) {
                size_t offset = guest_addr - reg.guest_vaddr;
                return reinterpret_cast<T*>(reg.host_ptr + offset);
            }
        }
        return nullptr;
    }

    bool read(vaddr_t guest_addr, void* dest, size_t size);
    bool write(vaddr_t guest_addr, const void* src, size_t size);

    // Stack and TLS helpers
    vaddr_t setupUserStack(size_t stack_size = 8 * 1024 * 1024); // 8 MiB stack

    void dumpMemoryMap() const;

private:
    MemoryManager() = default;
    ~MemoryManager();

    std::mutex m_mutex;
    std::vector<MemoryRegion> m_regions;
    std::unordered_map<paddr_t, DirectMemoryBlock> m_direct_blocks;
    paddr_t m_next_direct_paddr{PROSPERO_DIRECT_MEM_START};
    vaddr_t m_next_vaddr{0x000010000000ULL};
};

} // namespace prospero::memory
