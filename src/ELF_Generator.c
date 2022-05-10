#include "../include/Binary_Translator.h"

struct ELF_Header
{
    const char EI_MAG        [4];   // signature of an ELF file
    const char EI_CLASS      [1];   // 32-/64-bit version
    const char EI_DATA       [1];   // big/little endian
    const char EI_VERSION    [1];   // set to 1 for the original and current version
    const char EI_OSABI      [1];   // identifies the target operation system ABI (application binary interface)
    const char EI_ABIVERSION [1];   // version of ABI
    const char EI_PAD        [7];   // reserved padding bytes; currently unused

    const char e_type        [2];   // identifies object file type
    const char e_machine     [2];   // instruction set architecture
    const char e_verion      [4];   // set to 1 for the original version of ELF
    const char e_entry       [8];   // address of the entry point
    const char e_phoff       [8];   // points to the start of the program header table
    const char e_shoff       [8];   // points to the start of the section header table (0, if no header table)
    const char e_flags       [4];   // interpretation of this field depends on the target architecture
    const char e_ehsize      [2];   // size of the header (64 bytes for 64-bit version)
    const char e_phentsize   [2];   // contains the size of a program header table (56 for 64-bit version / 32 for 32-bit version)
    const char e_phnum       [2];   // contains the number of entries in the program header table
    const char e_shentsize   [2];   // contains the size of the section header table
    const char e_shstrndx    [2];   // contains index of the section header table entry that contains the section names
};

struct Program_Header
{
    const char p_type  [4]; // identifies the type of the segment (loadable)
    const char p_flags [4]; // segment flags (the segment is allowed to be read)
    const char p_offset[8]; // offset of the segment in the file image
    const char p_vaddr [8]; // virtual address of the segment in the memory
    const char p_paddr [8]; // physical address of the segment
    const char p_filesz[8]; // size in bytes of the segment in file image   
    const char p_memsz [8]; // size in bytes of the segment in memory
    const char p_align [8]; // no alignment
};

int Generate_ELF (const char *const file_name, const char *const data, const uint64_t entry_point)
{
    FILE *file = Open_File (file_name, "wb");

    char ELF_header[64] = {};

        static const struct ELF_Header File_Header =
    {
        {0x7F, 0x45, 0x4C, 0x46},   // Magic number: 0x7F, 'E', 'L', 'F'
        {0x02},                     // 64-bit version
        {0x01},                     // little endian
        {0x01},                     // default
        {0x00},                     // System V
        {0x00},                     // default
        {0x00},                     // default
        
        {0x02, 0x00},               // executable file
        {0x3E, 0x00},               // AMD x86-64
        {0x01, 0x00, 0x00, 0x00},   // default
        {},
        {0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // program head table is just after elf header
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // no section header table
        {0x00, 0x00, 0x00, 0x00},                           // interpretation of this field depends on the target architecture
        {0x40, 0x00},                                       // 64 bytes for 64-bit version
        {0x38, 0x00},                                       // 56 byte for 64-bit version
        {},                                                 // contains the number of entries in the program header table
        {0x00, 0x00},                                       // contains the size of the section header table
        {0x00, 0x00}                                        // contains index of the section header table entry that contains the section names
        
        // as far as executable file don't have the section header table, last 4 bytes are 0
    };

    struct Program_Header Pr_Header =
    {
        {0x01, 0x00, 0x00, 0x00},                           // identifies the type of the segment (loadable)
        {0x01, 0x00, 0x00, 0x00},                           // segment flags (the segment is allowed to be read)
        {},                                                 // offset of the segment in the file image
        {},                                                 // virtual address of the segment in the memory
        {},                                                 // physical address of the segment
        {},                                                 // size in bytes of the segment in file image   
        {},                                                 // size in bytes of the segment in memory
        {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // no alignment
    };

    Close_File (file, file_name);

    return NO_ERRORS;
}
