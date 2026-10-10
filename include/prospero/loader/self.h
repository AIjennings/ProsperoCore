#pragma once

#include "prospero/common/types.h"
#include <vector>

namespace prospero::self {

constexpr u32 SELF_MAGIC_SCE = 0x45435300; // "SCE\0"
constexpr u32 SELF_MAGIC_FSELF = 0x1D534F4F; // Prospero SELF magic

#pragma pack(push, 1)

struct SelfHeader {
    u32 magic;       // 0x1D534F4F or SCE\0
    u32 version;     // Format version
    u16 mode;        // Mode (e.g. 1 = standard)
    u16 endian;      // Endianness (1 = Little Endian)
    u32 attr;        // Attributes
    u32 key_type;    // Encryption key index
    u32 header_size; // Header size
    u32 meta_size;   // Metadata size
    u64 file_size;   // Total file size
    u16 num_entries; // Number of segment entries
    u16 flags;       // Flags
    u32 pad;
};

struct SelfEntry {
    u32 props;       // Entry properties
    u32 pad;
    u64 offset;      // Offset in SELF file
    u64 file_size;   // Compressed/encrypted size
    u64 memory_size; // Target uncompressed memory size
};

#pragma pack(pop)

} // namespace prospero::self
