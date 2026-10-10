#pragma once

#include "prospero/common/types.h"

namespace prospero::elf {

// Standard ELF Identification
constexpr u8 ELFMAG0 = 0x7F;
constexpr u8 ELFMAG1 = 'E';
constexpr u8 ELFMAG2 = 'L';
constexpr u8 ELFMAG3 = 'F';

constexpr u8 ELFCLASS64   = 2;
constexpr u8 ELFDATA2LSB  = 1;
constexpr u8 EV_CURRENT   = 1;
constexpr u8 ELFOSABI_FREEBSD = 9;

// ELF File Types
constexpr u16 ET_NONE = 0;
constexpr u16 ET_REL  = 1;
constexpr u16 ET_EXEC = 2;
constexpr u16 ET_DYN  = 3;
constexpr u16 ET_CORE = 4;
constexpr u16 ET_SCE_EXEC = 0xFE00;
constexpr u16 ET_SCE_RELEXEC = 0xFE04;
constexpr u16 ET_SCE_STUBLIB = 0xFE0C;
constexpr u16 ET_SCE_DYNAMIC = 0xFE18;

// ELF Machine
constexpr u16 EM_X86_64 = 62;

// Program Header Types
constexpr u32 PT_NULL    = 0;
constexpr u32 PT_LOAD    = 1;
constexpr u32 PT_DYNAMIC = 2;
constexpr u32 PT_INTERP  = 3;
constexpr u32 PT_NOTE    = 4;
constexpr u32 PT_SHLIB   = 5;
constexpr u32 PT_PHDR    = 6;
constexpr u32 PT_TLS     = 7;

// Sony SCE Program Header Types
constexpr u32 PT_SCE_RELA        = 0x60000000;
constexpr u32 PT_SCE_DYNLIBDATA  = 0x61000000;
constexpr u32 PT_SCE_PROGLIB     = 0x61000001;
constexpr u32 PT_SCE_VERSION     = 0x61000002;
constexpr u32 PT_SCE_RELRO       = 0x6474E552;
constexpr u32 PT_SCE_COMMENT     = 0x6FFFFF00;
constexpr u32 PT_SCE_LIBVERSION  = 0x6FFFFF01;

// Segment Flags
constexpr u32 PF_X = 0x1;
constexpr u32 PF_W = 0x2;
constexpr u32 PF_R = 0x4;

// Sony Dynamic Tags
constexpr u64 DT_NULL                 = 0;
constexpr u64 DT_NEEDED               = 1;
constexpr u64 DT_PLTRELSZ             = 2;
constexpr u64 DT_PLTGOT               = 3;
constexpr u64 DT_HASH                 = 4;
constexpr u64 DT_STRTAB               = 5;
constexpr u64 DT_SYMTAB               = 6;
constexpr u64 DT_RELA                 = 7;
constexpr u64 DT_RELASZ               = 8;
constexpr u64 DT_RELAENT              = 9;
constexpr u64 DT_STRSZ                = 10;
constexpr u64 DT_SYMENT               = 11;
constexpr u64 DT_INIT                 = 12;
constexpr u64 DT_FINI                 = 13;
constexpr u64 DT_SONAME               = 14;
constexpr u64 DT_RPATH                = 15;
constexpr u64 DT_SYMBOLIC             = 16;
constexpr u64 DT_REL                  = 17;
constexpr u64 DT_RELSZ                = 18;
constexpr u64 DT_RELENT               = 19;
constexpr u64 DT_PLTREL               = 20;
constexpr u64 DT_DEBUG                = 21;
constexpr u64 DT_TEXTREL              = 22;
constexpr u64 DT_JMPREL               = 23;
constexpr u64 DT_INIT_ARRAY           = 25;
constexpr u64 DT_FINI_ARRAY           = 26;
constexpr u64 DT_INIT_ARRAYSZ         = 27;
constexpr u64 DT_FINI_ARRAYSZ         = 28;

// Sony SCE Specific Dynamic Tags
constexpr u64 DT_SCE_FINGERPRINT      = 0x61000007;
constexpr u64 DT_SCE_ORIGINAL_FILENAME= 0x61000009;
constexpr u64 DT_SCE_MODULE_INFO      = 0x6100000D;
constexpr u64 DT_SCE_NEEDED_MODULE    = 0x6100000F;
constexpr u64 DT_SCE_MODULE_ATTR      = 0x61000011;
constexpr u64 DT_SCE_EXPORT_LIB       = 0x61000013;
constexpr u64 DT_SCE_IMPORT_LIB       = 0x61000015;
constexpr u64 DT_SCE_EXPORT_LIB_ATTR  = 0x61000017;
constexpr u64 DT_SCE_IMPORT_LIB_ATTR  = 0x61000019;
constexpr u64 DT_SCE_RELA             = 0x6100002F;
constexpr u64 DT_SCE_RELASZ           = 0x61000031;
constexpr u64 DT_SCE_RELAENT          = 0x61000033;
constexpr u64 DT_SCE_STRTAB           = 0x61000035;
constexpr u64 DT_SCE_STRSZ            = 0x61000037;
constexpr u64 DT_SCE_SYMTAB           = 0x61000039;
constexpr u64 DT_SCE_SYMENT           = 0x6100003B;
constexpr u64 DT_SCE_HASHSZ           = 0x6100003D;
constexpr u64 DT_SCE_SYMTABSZ         = 0x6100003F;

// Relocations (x86-64)
constexpr u32 R_X86_64_NONE      = 0;
constexpr u32 R_X86_64_64        = 1;
constexpr u32 R_X86_64_PC32      = 2;
constexpr u32 R_X86_64_GOT32     = 3;
constexpr u32 R_X86_64_PLT32     = 4;
constexpr u32 R_X86_64_COPY      = 5;
constexpr u32 R_X86_64_GLOB_DAT  = 6;
constexpr u32 R_X86_64_JUMP_SLOT = 7;
constexpr u32 R_X86_64_RELATIVE  = 8;
constexpr u32 R_X86_64_DTPMOD64  = 16;
constexpr u32 R_X86_64_DTPOFF64  = 17;
constexpr u32 R_X86_64_TPOFF64   = 18;

#pragma pack(push, 1)

struct Elf64_Ehdr {
    u8  e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
};

struct Elf64_Phdr {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
};

struct Elf64_Shdr {
    u32 sh_name;
    u32 sh_type;
    u64 sh_flags;
    u64 sh_addr;
    u64 sh_offset;
    u64 sh_size;
    u32 sh_link;
    u32 sh_info;
    u64 sh_addralign;
    u64 sh_entsize;
};

struct Elf64_Dyn {
    s64 d_tag;
    union {
        u64 d_val;
        u64 d_ptr;
    } d_un;
};

struct Elf64_Sym {
    u32 st_name;
    u8  st_info;
    u8  st_other;
    u16 st_shndx;
    u64 st_value;
    u64 st_size;
};

struct Elf64_Rela {
    u64 r_offset;
    u64 r_info;
    s64 r_addend;
};

#pragma pack(pop)

inline u32 ELF64_R_SYM(u64 info) { return static_cast<u32>(info >> 32); }
inline u32 ELF64_R_TYPE(u64 info) { return static_cast<u32>(info & 0xFFFFFFFF); }

} // namespace prospero::elf
