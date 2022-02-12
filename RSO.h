#pragma once

struct RSOHeader
{
    u32 next_module_link;
    u32 prev_module_link;
    u32 section_count;
    u32 section_info_offset;
    u32 module_name_offset;
    u32 module_name_size;
    u32 module_version;
    u32 bss_size;
    u8 prolog_section_index;
    u8 epilog_section_index;
    u8 unresolved_section_index;
    u8 bss_section_index;

    u32 prolog_function_offset;
    u32 epilog_function_offset;
    u32 unresolved_function_offset;

    u32 internal_relocation_table_offset;
    u32 internal_relocation_table_size;

    u32 external_relocation_table_offset;
    u32 external_relocation_table_size;

    u32 export_symbol_table_offset;
    u32 export_symbol_table_size;
    u32 export_symbol_names_offset;

    u32 import_symbol_table_offset;
    u32 import_symbol_table_size;
    u32 import_symbol_names_offset;
};

enum RSORelocationType
{
    R_PPC_NONE = 0,
    R_PPC_ADDR32,
    R_PPC_ADDR24,
    R_PPC_ADDR16,
    R_PPC_ADDR16_LO,
    R_PPC_ADDR16_HI,
    R_PPC_ADDR16_HA,
    R_PPC_ADDR14,
    R_PPC_ADDR14_BRTAKEN,
    R_PPC_ADDR14_BRNKTAKEN,
    R_PPC_REL24,
    R_PPC_REL14,
};

struct RSORelocation
{
    u32 symbolHash;
    u32 symbolIndex;
    uint32_t section;
    uint32_t offset;
    uint8_t targetSection;  // target symbol
    uint32_t addend;
    uint8_t type;
};

struct RSOSectionInfo
{
    u32 offset;
    u32 size;
};

struct RSOSymbol
{
    u32 hash;
    std::string symbol;
    u32 sectionIndex;
    u32 sectionRelativeOffset;  // For exported symbol this mean the symbol offset, for imported
                                // symbol this mean the relocation offset
};