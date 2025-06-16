#include "ExeFile.h"
#include "../zero_templates.h"

u32 GetPEOffset(u8 *buffer) {
	return *based_pointer<u32>(buffer, 0x3C);
}

PeHeader ExeFile::GetPEHeader() {
    u32 pe_offset = GetPEOffset(buffer);

    return *based_pointer<PeHeader>(buffer, pe_offset);
}

Pe32OptionalHeader ExeFile::GetPEOptionalHeader() {
	u32 pe_offset = GetPEOffset(buffer);
	u32 pe_optional_offset = pe_offset + sizeof(PeHeader);

	return *based_pointer<Pe32OptionalHeader>(buffer, pe_optional_offset);
}

bool ExeFile::SigCheck(u8 *buffer)
{
    u16 mz_signature = *based_pointer<u16>(buffer, 0x0);
	u32 pe_offset = GetPEOffset(buffer);
	u32 pe_signature = *based_pointer<u32>(buffer, pe_offset);

    return mz_signature == PackUInt16('M', 'Z') && pe_signature == PackUInt16('P', 'E');
}

std::map<std::string, Pe32SectionHeader> ExeFile::ParseSectionHeaders() {
	std::map<std::string, Pe32SectionHeader> info;
    u32 pe_offset = GetPEOffset(buffer);

    PeHeader *pe_header = based_pointer<PeHeader>(buffer, pe_offset);
    u32 section_headers_offset = pe_offset + sizeof(PeHeader) + pe_header->mSizeOfOptionalHeader;

	for (u16 i = 0; i < pe_header->mNumberOfSections; i++) {
        Pe32SectionHeader *section = based_pointer<Pe32SectionHeader>(
            buffer, section_headers_offset + (i * sizeof(Pe32SectionHeader))
        );
		info.insert({std::string(section->name), *section});
    }
	return info;
}

Pe32SectionHeader* ExeFile::GetSectionHeader(std::string target_section) {
    return &this->sections[target_section];
}

bool ExeFile::ContainsSection(std::string section_name) {
    return this->sections.count(section_name);
}
