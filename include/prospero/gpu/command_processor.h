#pragma once

#include "prospero/common/types.h"
#include "prospero/gpu/pm4_packet.h"
#include <vector>
#include <unordered_map>

namespace prospero::gpu {

struct GpuPipelineStats {
    u64 draw_calls{0};
    u64 compute_dispatches{0};
    u64 packets_processed{0};
    u64 registers_written{0};
};

class CommandProcessor {
public:
    CommandProcessor();
    ~CommandProcessor() = default;

    void processCommandBuffer(const u32* dwords, size_t count);
    const GpuPipelineStats& getStats() const { return m_stats; }
    void resetStats();

    void dumpState() const;

private:
    void handleSetContextReg(const u32* packet, u32 count);
    void handleSetShReg(const u32* packet, u32 count);
    void handleDrawIndexAuto(const u32* packet, u32 count);
    void handleDispatchDirect(const u32* packet, u32 count);

    std::unordered_map<u32, u32> m_context_regs;
    std::unordered_map<u32, u32> m_sh_regs;
    GpuPipelineStats m_stats;
};

} // namespace prospero::gpu
