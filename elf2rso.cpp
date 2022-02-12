#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <tuple>

#include "FileWriter.h"
#include "elfio/elfio.hpp"
#include "optparser.h"

enum RelocationType
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
    R_PPC_COUNT
};

namespace fs = std::filesystem;

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

struct Relocation
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

void writeModuleHeader(FileWriter& writer, RSOHeader& header)
{
    writer.writeBE(header.next_module_link);
    writer.writeBE(header.prev_module_link);
    writer.writeBE(header.section_count);
    writer.writeBE(header.section_info_offset);
    writer.writeBE(header.module_name_offset);
    writer.writeBE(header.module_name_size);
    writer.writeBE(header.module_version);
    writer.writeBE(header.bss_size);
    writer.writeBE(header.prolog_section_index);
    writer.writeBE(header.epilog_section_index);
    writer.writeBE(header.unresolved_section_index);
    writer.writeBE(header.bss_section_index);
    writer.writeBE(header.prolog_function_offset);
    writer.writeBE(header.epilog_function_offset);
    writer.writeBE(header.unresolved_function_offset);
    writer.writeBE(header.internal_relocation_table_offset);
    writer.writeBE(header.internal_relocation_table_size);
    writer.writeBE(header.external_relocation_table_offset);
    writer.writeBE(header.external_relocation_table_size);
    writer.writeBE(header.export_symbol_table_offset);
    writer.writeBE(header.export_symbol_table_size);
    writer.writeBE(header.export_symbol_names_offset);
    writer.writeBE(header.import_symbol_table_offset);
    writer.writeBE(header.import_symbol_table_size);
    writer.writeBE(header.import_symbol_names_offset);
}

void writeSectionInfo(FileWriter& writer, u32 offset, u32 size)
{
    writer.writeBE(offset);
    writer.writeBE(size);
}

void writeExportSymbol(FileWriter& writer, u32 nameOffset, u32 offset, u32 sectionIndex, u32 hash)
{
    writer.writeBE(nameOffset);
    writer.writeBE(offset);
    writer.writeBE(sectionIndex);
    writer.write(hash);
}

void writeImportSymbol(FileWriter& writer, u32 nameOffset, u32 relOffset)
{
    writer.writeBE(nameOffset);
    writer.write(0);
    writer.writeBE(relOffset);
}

void writeRelocation(FileWriter& writer, u32 offset, u32 symbol_id, u8 relocation_type, u32 addend)
{
    writer.writeBE(offset);

    writer.writeBE(symbol_id << 8);
    writer.seek(writer.position() - 1);
    writer.writeBE(relocation_type);

    writer.writeBE(addend);
}

// @Source: PistonMiner's elf2rel
const std::vector<std::string> cRelSectionMask = {".init",   ".text", ".ctors", ".dtors",
                                                  ".rodata", ".data", ".bss"};

u32 getHash(std::string& symbol)
{
    u32 hash = 0;
    for (const auto& chr : symbol)
    {
        u32 mod = (hash << 4) + chr;
        u32 negate = mod & 0xF0000000;
        if (negate != 0)
        {
            mod ^= negate >> 24;
        }
        hash = mod & ~negate;
    }
    return hash;
}

int main(int argc, char** argv)
{
    optparse::OptionParser parser = optparse::OptionParser().description("Elf2RSO v1.0");

    parser.add_option("-i", "--input").dest("input").help("ELF File").metavar("FILE");
    parser.add_option("-o", "--output").dest("output").set_default("").help("Output file");
    parser.add_option("-a", "--fullpath")
        .dest("fullpath")
        .action("true")
        .set_default(false)
        .help("Use fullpath for module name");

    const optparse::Values options = parser.parse_args(argc, argv);

    if (!options.is_set("input"))
    {
        parser.print_help();
        return -1;
    }

    const auto elfFile = options["input"];
    fs::path rsoFile;
    if (!options.is_set_by_user("output"))
    {
        rsoFile = elfFile;
        rsoFile.replace_extension(".rso");
    }
    else
    {
        rsoFile = options.get("output");
    }

    // Load input file
    ELFIO::elfio inputElf;
    if (!inputElf.load(elfFile))
    {
        printf("Failed to load input file\n");
        return 1;
    }

    const bool useFullPath = options.get("fullpath");

    FileWriter fileWriter(rsoFile);

    // Find special sections
    ELFIO::section* symSection = nullptr;
    std::vector<ELFIO::section*> internalRelocationSections;
    for (const auto& section : inputElf.sections)
    {
        if (section->get_type() == SHT_SYMTAB)
        {
            symSection = section;
        }
        else if (section->get_type() == SHT_RELA)
        {
            internalRelocationSections.emplace_back(section);
        }
    }

    // Symbol accessor
    ELFIO::symbol_section_accessor symbols(inputElf, symSection);

    // Find prolog, epilog and unresolved
    // @Source: PistonMiner's elf2rel
    auto findSymbolSectionAndOffset = [&](const std::string& name, u8& sectionIndex, u32& offset) {
        ELFIO::Elf64_Addr addr;
        ELFIO::Elf_Xword size;
        unsigned char bind;
        unsigned char type;
        ELFIO::Elf_Half section_index;
        unsigned char other;
        std::string symbolName;
        for (int i = 0; i < symbols.get_symbols_num(); ++i)
        {
            if (symbols.get_symbol(static_cast<ELFIO::Elf_Xword>(i), symbolName, addr, size, bind,
                                   type, section_index, other))
            {
                if (symbolName == name)
                {
                    sectionIndex = static_cast<u8>(section_index);
                    offset = static_cast<u32>(addr);
                    break;
                }
            }
        }
    };

    RSOHeader header{};
    header.module_version = 1;

    findSymbolSectionAndOffset("_prolog", header.prolog_section_index,
                               header.prolog_function_offset);
    findSymbolSectionAndOffset("_epilog", header.epilog_section_index,
                               header.epilog_function_offset);
    findSymbolSectionAndOffset("_unresolved", header.unresolved_section_index,
                               header.unresolved_function_offset);

    writeModuleHeader(fileWriter, header);

    // Write Sections Info Table (Blank)
    header.section_info_offset = fileWriter.position();
    header.section_count = inputElf.sections.size();
    for (const auto& section : inputElf.sections)
    {
        writeSectionInfo(fileWriter, 0, 0);
    }

    std::vector<RSOSectionInfo> rsoSections;
    auto totalBssSize = 0u;
    for (const auto& section : inputElf.sections)
    {
        const auto sectionName = section->get_name();
        const auto fromValidSection = std::find_if(cRelSectionMask.begin(), cRelSectionMask.end(),
                                                   [&](const std::string& val) {
                                                       return sectionName == val;
                                                   }) != cRelSectionMask.end();

        const auto size = static_cast<u32>(section->get_size());
        if (!fromValidSection || size == 0)
        {
            rsoSections.emplace_back(RSOSectionInfo{0, 0});
            continue;
        }

        if (section->get_type() == SHT_NOBITS)
        {
            totalBssSize += size;
            rsoSections.emplace_back(RSOSectionInfo{0, size});
            continue;
        }

        fileWriter.padToAlignment(static_cast<size_t>(section->get_addr_align()));

        const auto offset = static_cast<u32>(fileWriter.position());

        fileWriter.write(section->get_data(), size);
        rsoSections.emplace_back(RSOSectionInfo{offset, size});
    }

    header.bss_size = totalBssSize;

    // Save the position
    fileWriter.padToAlignment(4);
    header.module_name_offset = static_cast<u32>(fileWriter.position());

    // Rewind and write the correct section infos
    fileWriter.seek(header.section_info_offset);

    for (const auto& section : rsoSections)
    {
        writeSectionInfo(fileWriter, section.offset, section.size);
    }

    // Go back
    fileWriter.seek(static_cast<size_t>(header.module_name_offset));

    // Write Module Name
    if (!useFullPath)
    {
        std::string name = rsoFile.filename().string();
        header.module_name_size = name.size();
        fileWriter.writeString(name);
    }
    else
    {
        std::string name = fs::absolute(rsoFile).string();
        header.module_name_size = name.size();
        fileWriter.writeString(name);
    }

    std::vector<RSOSymbol> internalSymbolTable;
    std::vector<RSOSymbol> externalSymbolTable;

    const auto tryInsertSymbol = [](std::vector<RSOSymbol>& symbolTable, std::string symbolName,
                                    u32 sectionIndex, u32 sectionRelativeOffset) {
        const auto hash = getHash(symbolName);
        const auto it = std::find_if(symbolTable.begin(), symbolTable.end(),
                                     [&hash](const RSOSymbol& p) { return p.hash == hash; });

        if (it != symbolTable.end())
        {
            return std::make_tuple(static_cast<u32>(it - symbolTable.begin()), hash);
        }

        symbolTable.emplace_back(
            RSOSymbol{hash, std::move(symbolName), sectionIndex, sectionRelativeOffset});
        return std::make_tuple(static_cast<u32>(symbolTable.size() - 1), hash);
    };

    std::vector<Relocation> internalRelocations;
    std::vector<Relocation> externalRelocations;

    // Acumulate Relocations
    for (const auto& section : inputElf.sections)
    {
        const auto sectionName = section->get_name();

        // Check if it's a relocation section
        if (sectionName.compare(0, 5, ".rela") != 0)
        {
            continue;
        }

        // Check if the relocation section is from a usable section
        const auto fromValidSection =
            std::find_if(cRelSectionMask.begin(), cRelSectionMask.end(),
                         [&](const std::string& val) {
                             return sectionName.compare(5, val.size(), val.c_str()) == 0;
                         }) != cRelSectionMask.end();

        if (!fromValidSection)
        {
            continue;
        }

        const auto relocationSectionIndex = section->get_info();

        ELFIO::relocation_section_accessor relocations(inputElf, section);

        for (auto i = 0u; i < relocations.get_entries_num(); ++i)
        {
            ELFIO::Elf64_Addr offset;
            ELFIO::Elf_Word symbol;
            ELFIO::Elf_Word type;
            ELFIO::Elf_Sxword addend;
            relocations.get_entry(i, offset, symbol, type, addend);

            if (type == R_PPC_NONE)
                continue;

            // TODO: If the `_unresolved` function is found and the relocation is of type
            //		 `R_PPC_REL24` we can immediately patch it and still write the relocation.
            //		 That way even if at the moment of execution the relocation hasn't being
            //		 applied it would still call someone.

            ELFIO::Elf_Xword size;
            unsigned char bind;
            unsigned char symbolType;
            ELFIO::Elf_Half sectionIndex;
            unsigned char other;
            std::string symbolName;
            ELFIO::Elf64_Addr symbolValue;
            if (!symbols.get_symbol(symbol, symbolName, symbolValue, size, bind, symbolType,
                                    sectionIndex, other))
            {
                printf("Unable to find symbol %u in symbol table!\n",
                       static_cast<uint32_t>(symbol));
                return 1;
            }

            Relocation rel;
            rel.section = relocationSectionIndex;
            rel.offset = static_cast<uint32_t>(offset);
            rel.type = type;
            rel.targetSection = static_cast<uint8_t>(sectionIndex);
            if (sectionIndex != 0)
            {
                // Internal Relocation
                auto [symbolIndex, hash] = tryInsertSymbol(
                    internalSymbolTable, std::move(symbolName), rel.section, symbolValue);
                rel.symbolHash = hash;
                rel.symbolIndex = symbolIndex;
                rel.addend = static_cast<uint32_t>(addend + symbolValue);
                internalRelocations.emplace_back(rel);
            }
            else
            {
                // External Relocation
                auto [symbolIndex, hash] = tryInsertSymbol(
                    externalSymbolTable, std::move(symbolName), rel.section, rel.offset);
                rel.symbolHash = hash;
                rel.symbolIndex = symbolIndex;
                rel.addend = 0;
                externalRelocations.emplace_back(rel);
            }
        }
    }

    // Sort External Relocation, by Imported Symbol Index
    std::sort(externalRelocations.begin(), externalRelocations.end(),
              [](const Relocation& left, const Relocation& right) {
                  return left.symbolIndex > right.symbolIndex;
              });

    // Sort Internal Symbol by Hash
    std::sort(internalSymbolTable.begin(), internalSymbolTable.end(),
              [](const RSOSymbol& left, const RSOSymbol& right) { return left.hash > right.hash; });

    // Write Exported Symbol Table

    // Calculate NameOffset
    std::vector<u32> symbolNameOffset;
    auto currentNameOffset = 0u;
    for (const auto& internalSymbol : internalSymbolTable)
    {
        symbolNameOffset.emplace_back(currentNameOffset);
        currentNameOffset += internalSymbol.symbol.size() + 1;  // Include `\0`
    }

    fileWriter.padToAlignment(4);
    header.export_symbol_table_offset = fileWriter.position();
    header.export_symbol_table_size = internalSymbolTable.size() * 16;
    for (auto idx = 0u; idx < internalSymbolTable.size(); ++idx)
    {
        const auto& internalSymbol = internalSymbolTable[idx];
        const auto nameOffset = symbolNameOffset[idx];
        writeExportSymbol(fileWriter, nameOffset, internalSymbol.sectionRelativeOffset,
                          internalSymbol.sectionIndex, internalSymbol.hash);
    }

    // Write Exported Symbol String Table
    fileWriter.padToAlignment(4);
    header.export_symbol_names_offset = fileWriter.position();
    for (const auto& internalSymbol : internalSymbolTable)
    {
        fileWriter.writeString(internalSymbol.symbol);
    }

    // Write External Relocation
    fileWriter.padToAlignment(4);
    header.external_relocation_table_offset = fileWriter.position();
    header.external_relocation_table_size = externalRelocations.size() * 12;
    for (const auto& relocation : externalRelocations)
    {
        const auto section = rsoSections[relocation.section];

        // Convert the relocation offset from being section relative to file relative
        const auto offset = section.offset + relocation.offset;

        // Get the symbol index inside the import symbol table
        const auto& symbolIt =
            std::find_if(externalSymbolTable.begin(), externalSymbolTable.end(),
                         [&](const RSOSymbol& p) { return p.hash == relocation.symbolHash; });

        if (symbolIt == externalSymbolTable.end())
        {
            // This should be imposible since the hash of the symbol come from a function that add
            // the symbol to the list
            printf("Errror! Unable to find symbol for relocation %ull\n", offset);
            return 1;
        }

        const auto symbolIndex = static_cast<u32>(symbolIt - externalSymbolTable.begin());
        writeRelocation(fileWriter, offset, symbolIndex, relocation.type, relocation.addend);
    }

    // Write Imported Symbol Table

    // Calculate name offset
    symbolNameOffset.clear();
    currentNameOffset = 0u;
    for (const auto& externalSymbol : externalSymbolTable)
    {
        symbolNameOffset.emplace_back(currentNameOffset);
        currentNameOffset += externalSymbol.symbol.size() + 1;  // Include `\0`
    }

    fileWriter.padToAlignment(4);
    header.import_symbol_table_offset = fileWriter.position();
    header.import_symbol_table_size = externalSymbolTable.size() * 12;

    for (auto idx = 0u; idx < externalSymbolTable.size(); ++idx)
    {
        const auto& externalSymbol = externalSymbolTable[idx];

        const auto nameOffset = symbolNameOffset[idx];

        // Find first relocation that uses this
        const auto& relIt = std::find_if(
            externalRelocations.begin(), externalRelocations.end(),
            [&](const Relocation& rel) { return rel.symbolHash == externalSymbol.hash; });

        if (relIt == externalRelocations.end())
        {
            printf("Error! Unable to find relocation for external symbol %s",
                   externalSymbol.symbol.c_str());
            return 1;
        }

        const auto relOffset = static_cast<u32>(relIt - externalRelocations.begin()) * 12;

        writeImportSymbol(fileWriter, nameOffset, relOffset);
    }

    // Write Imported Symbol String Table
    fileWriter.padToAlignment(4);
    header.import_symbol_names_offset = fileWriter.position();
    for (const auto& externalSymbol : externalSymbolTable)
    {
        fileWriter.writeString(externalSymbol.symbol);
    }

    // Write Internal Relocation Table
    fileWriter.padToAlignment(4);
    header.internal_relocation_table_offset = fileWriter.position();
    header.internal_relocation_table_size = internalRelocations.size() * 12;
    for (const auto& relocation : internalRelocations)
    {
        const auto section = rsoSections[relocation.section];

        // Convert the relocation offset from being section relative to file relative
        const auto offset = section.offset + relocation.offset;

        // Get the symbol index inside the import symbol table
        const auto& sectionIndex = relocation.section;

        writeRelocation(fileWriter, offset, sectionIndex, relocation.type, relocation.addend);
    }

    fileWriter.padToAlignment(32);
    fileWriter.seek(0);
    writeModuleHeader(fileWriter, header);

    return 0;
}