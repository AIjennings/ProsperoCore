#pragma once

#include "prospero/common/types.h"

namespace prospero::gpu {

// AMD RDNA2 / GFX10.3 PM4 Packet Definitions
constexpr u32 PM4_HEADER_TYPE_MASK  = 0xC0000000;
constexpr u32 PM4_HEADER_TYPE_3     = 0xC0000000;

// PM4 Type 3 Opcodes
constexpr u32 IT_NOP                = 0x10;
constexpr u32 IT_SET_BASE           = 0x11;
constexpr u32 IT_CLEAR_STATE        = 0x12;
constexpr u32 IT_INDEX_BUFFER_SIZE  = 0x13;
constexpr u32 IT_DISPATCH_DIRECT    = 0x15;
constexpr u32 IT_DISPATCH_INDIRECT  = 0x16;
constexpr u32 IT_OCCLUSION_QUERY    = 0x1F;
constexpr u32 IT_SET_PREDICATION    = 0x20;
constexpr u32 IT_DRAW_INDEX_2       = 0x27;
constexpr u32 IT_CONTEXT_CONTROL    = 0x28;
constexpr u32 IT_INDEX_TYPE         = 0x2A;
constexpr u32 IT_DRAW_INDEX_AUTO    = 0x2D;
constexpr u32 IT_DRAW_INDIRECT      = 0x24;
constexpr u32 IT_NUM_INSTANCES      = 0x2F;
constexpr u32 IT_STRMOUT_BUFFER_UPDATE = 0x34;
constexpr u32 IT_EVENT_WRITE        = 0x46;
constexpr u32 IT_EVENT_WRITE_EOP    = 0x47;
constexpr u32 IT_EVENT_WRITE_EOS    = 0x48;
constexpr u32 IT_RELEASE_MEM        = 0x49;
constexpr u32 IT_DMA_DATA           = 0x50;
constexpr u32 IT_ACQUIRE_MEM        = 0x58;
constexpr u32 IT_REWIND             = 0x59;
constexpr u32 IT_SET_CONFIG_REG     = 0x68;
constexpr u32 IT_SET_CONTEXT_REG    = 0x69;
constexpr u32 IT_SET_SH_REG         = 0x76;
constexpr u32 IT_SET_UCONFIG_REG    = 0x79;

inline u32 getPm4HeaderType(u32 header) { return (header >> 30) & 0x3; }
inline u32 getPm4Opcode(u32 header)     { return (header >> 8) & 0xFF; }
inline u32 getPm4Count(u32 header)      { return (header >> 16) & 0x3FFF; }
inline u32 getPm4PayloadDwords(u32 header) { return ((header >> 16) & 0x3FFF) + 1; }

} // namespace prospero::gpu
