#include "prospero/gpu/command_processor.h"
#include "prospero/common/logging.h"
#include <iostream>
#include <iomanip>

namespace prospero::gpu {

CommandProcessor::CommandProcessor() = default;

void CommandProcessor::resetStats() {
    m_stats = GpuPipelineStats{};
}

void CommandProcessor::processCommandBuffer(const u32* dwords, size_t total_dwords) {
    size_t index = 0;

    while (index < total_dwords) {
        u32 header = dwords[index++];
        u32 header_type = getPm4HeaderType(header);

        if (header_type != 3) {
            // Non-type 3 packet or padding
            continue;
        }

        u32 opcode = getPm4Opcode(header);
        u32 count = getPm4PayloadDwords(header);

        if (index + count > total_dwords) {
            PROSPERO_LOG_WARN("GPU", "PM4 packet exceeds buffer boundaries (Opcode: 0x" 
                              << std::hex << opcode << " Count: " << count << std::dec << ")");
            break;
        }

        const u32* payload = &dwords[index];
        index += count;
        m_stats.packets_processed++;

        switch (opcode) {
            case IT_NOP:
                // Debug string or marker
                break;
            case IT_SET_CONTEXT_REG:
                handleSetContextReg(payload, count);
                break;
            case IT_SET_SH_REG:
                handleSetShReg(payload, count);
                break;
            case IT_DRAW_INDEX_AUTO:
                handleDrawIndexAuto(payload, count);
                break;
            case IT_DISPATCH_DIRECT:
                handleDispatchDirect(payload, count);
                break;
            default:
                PROSPERO_LOG_GPU("PM4", "Packet Opcode: 0x" << std::hex << opcode 
                                 << " (Count: " << std::dec << count << ")");
                break;
        }
    }
}

void CommandProcessor::handleSetContextReg(const u32* packet, u32 count) {
    if (count < 2) return;
    u32 reg_offset = packet[0] & 0xFFFF;
    for (u32 i = 1; i < count; ++i) {
        m_context_regs[reg_offset + i - 1] = packet[i];
        m_stats.registers_written++;
    }
}

void CommandProcessor::handleSetShReg(const u32* packet, u32 count) {
    if (count < 2) return;
    u32 reg_offset = packet[0] & 0xFFFF;
    for (u32 i = 1; i < count; ++i) {
        m_sh_regs[reg_offset + i - 1] = packet[i];
        m_stats.registers_written++;
    }
}

void CommandProcessor::handleDrawIndexAuto(const u32* packet, u32 count) {
    if (count < 2) return;
    u32 index_count = packet[0];
    u32 instance_count = packet[1];

    m_stats.draw_calls++;
    PROSPERO_LOG_GPU("DRAW", "DrawIndexAuto: Vertices=" << index_count << " Instances=" << instance_count);
}

void CommandProcessor::handleDispatchDirect(const u32* packet, u32 count) {
    if (count < 3) return;
    u32 dim_x = packet[0];
    u32 dim_y = packet[1];
    u32 dim_z = packet[2];

    m_stats.compute_dispatches++;
    PROSPERO_LOG_GPU("COMPUTE", "DispatchDirect: Grid=(" << dim_x << ", " << dim_y << ", " << dim_z << ")");
}

void CommandProcessor::dumpState() const {
    std::cout << "\n=======================================================\n";
    std::cout << "             PROSPERO GPU PIPELINE STATE               \n";
    std::cout << "=======================================================\n";
    std::cout << "  Packets Processed:    " << m_stats.packets_processed << "\n";
    std::cout << "  Draw Calls:           " << m_stats.draw_calls << "\n";
    std::cout << "  Compute Dispatches:   " << m_stats.compute_dispatches << "\n";
    std::cout << "  Registers Written:    " << m_stats.registers_written << "\n";
    std::cout << "  Active Context Regs:  " << m_context_regs.size() << "\n";
    std::cout << "  Active Shader Regs:   " << m_sh_regs.size() << "\n";
    std::cout << "=======================================================\n\n";
}

} // namespace prospero::gpu
