#include "prospero/loader/elf.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <iostream>

using namespace prospero;

int main(int argc, char* argv[]) {
    const char* out_filename = (argc > 1) ? argv[1] : "ps5_game.elf";

    std::cout << "Generating synthetic PS5 ELF binary: " << out_filename << "...\n";

    prospero::elf::Elf64_Ehdr ehdr{};
    ehdr.e_ident[0] = prospero::elf::ELFMAG0;
    ehdr.e_ident[1] = prospero::elf::ELFMAG1;
    ehdr.e_ident[2] = prospero::elf::ELFMAG2;
    ehdr.e_ident[3] = prospero::elf::ELFMAG3;
    ehdr.e_ident[4] = prospero::elf::ELFCLASS64;
    ehdr.e_ident[5] = prospero::elf::ELFDATA2LSB;
    ehdr.e_ident[6] = prospero::elf::EV_CURRENT;
    ehdr.e_ident[7] = prospero::elf::ELFOSABI_FREEBSD;

    ehdr.e_type = prospero::elf::ET_SCE_EXEC;
    ehdr.e_machine = prospero::elf::EM_X86_64;
    ehdr.e_version = 1;
    ehdr.e_entry = 0x400100;
    ehdr.e_phoff = sizeof(prospero::elf::Elf64_Ehdr);
    ehdr.e_ehsize = sizeof(prospero::elf::Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(prospero::elf::Elf64_Phdr);
    ehdr.e_phnum = 3; // PT_LOAD (code), PT_DYNAMIC, PT_SCE_DYNLIBDATA

    // String table for dynamic libraries
    std::string strtab = "\0libkernel.sprx\0libSceVideoOut.sprx\0libScePad.sprx\0libSceSysmodule.sprx\0";
    u64 libkernel_offset = 1;
    u64 libvideo_offset = 16;
    u64 libpad_offset = 33;
    u64 libsysmod_offset = 48;

    // Dynamic tags
    std::vector<prospero::elf::Elf64_Dyn> dyns = {
        { prospero::elf::DT_SCE_NEEDED_MODULE, { libkernel_offset } },
        { prospero::elf::DT_SCE_NEEDED_MODULE, { libvideo_offset } },
        { prospero::elf::DT_SCE_NEEDED_MODULE, { libpad_offset } },
        { prospero::elf::DT_SCE_NEEDED_MODULE, { libsysmod_offset } },
        { prospero::elf::DT_SCE_STRTAB, { 0 } },
        { prospero::elf::DT_SCE_STRSZ, { strtab.size() } },
        { prospero::elf::DT_NULL, { 0 } }
    };

    // Code payload (x86-64 instructions)
    // mov rax, 0x42 ; ret
    std::vector<prospero::u8> code = {
        0x48, 0xC7, 0xC0, 0x42, 0x00, 0x00, 0x00, // mov rax, 0x42
        0xC3                                      // ret
    };

    // Calculate layout
    u64 code_file_offset = 0x100;
    u64 dyn_file_offset = code_file_offset + code.size();
    u64 strtab_file_offset = dyn_file_offset + dyns.size() * sizeof(prospero::elf::Elf64_Dyn);

    // Update dynamic tag for strtab
    dyns[2].d_un.d_ptr = strtab_file_offset;

    // Program headers
    std::vector<prospero::elf::Elf64_Phdr> phdrs(3);

    // 1. PT_LOAD (Code + Data)
    phdrs[0].p_type = prospero::elf::PT_LOAD;
    phdrs[0].p_flags = prospero::elf::PF_R | prospero::elf::PF_X;
    phdrs[0].p_offset = code_file_offset;
    phdrs[0].p_vaddr = 0x400000;
    phdrs[0].p_paddr = 0x400000;
    phdrs[0].p_filesz = 0x1000;
    phdrs[0].p_memsz = 0x4000;
    phdrs[0].p_align = 0x4000;

    // 2. PT_DYNAMIC
    phdrs[1].p_type = prospero::elf::PT_DYNAMIC;
    phdrs[1].p_flags = prospero::elf::PF_R;
    phdrs[1].p_offset = dyn_file_offset;
    phdrs[1].p_vaddr = 0x400000 + dyn_file_offset;
    phdrs[1].p_paddr = phdrs[1].p_vaddr;
    phdrs[1].p_filesz = dyns.size() * sizeof(prospero::elf::Elf64_Dyn);
    phdrs[1].p_memsz = phdrs[1].p_filesz;
    phdrs[1].p_align = 8;

    // 3. PT_SCE_DYNLIBDATA
    phdrs[2].p_type = prospero::elf::PT_SCE_DYNLIBDATA;
    phdrs[2].p_flags = prospero::elf::PF_R;
    phdrs[2].p_offset = strtab_file_offset;
    phdrs[2].p_vaddr = 0x400000 + strtab_file_offset;
    phdrs[2].p_paddr = phdrs[2].p_vaddr;
    phdrs[2].p_filesz = strtab.size();
    phdrs[2].p_memsz = strtab.size();
    phdrs[2].p_align = 1;

    // Write file
    std::ofstream out(out_filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }

    out.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
    out.write(reinterpret_cast<const char*>(phdrs.data()), phdrs.size() * sizeof(prospero::elf::Elf64_Phdr));

    // Pad to code_file_offset
    std::streampos pos = out.tellp();
    if (pos < code_file_offset) {
        std::vector<char> pad(code_file_offset - pos, 0);
        out.write(pad.data(), pad.size());
    }

    out.write(reinterpret_cast<const char*>(code.data()), code.size());
    out.write(reinterpret_cast<const char*>(dyns.data()), dyns.size() * sizeof(prospero::elf::Elf64_Dyn));
    out.write(strtab.data(), strtab.size());

    out.close();
    std::cout << "Successfully generated " << out_filename << " (" << strtab_file_offset + strtab.size() << " bytes)\n";
    return 0;
}
