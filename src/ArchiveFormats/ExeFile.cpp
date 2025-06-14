#include "ExeFile.h"

uint32_t GetPEOffset(uint8_t *buffer) {
	return *based_pointer<uint32_t>(buffer, 0x3C);
}

PeHeader ExeFile::GetPEHeader() {
    uint32_t pe_offset = GetPEOffset(buffer);

    return *based_pointer<PeHeader>(buffer, pe_offset);
}

Pe32OptionalHeader ExeFile::GetPEOptionalHeader() {
	uint32_t pe_offset = GetPEOffset(buffer);
	uint32_t pe_optional_offset = pe_offset + sizeof(PeHeader);

	return *based_pointer<Pe32OptionalHeader>(buffer, pe_optional_offset);
}

bool ExeFile::SigCheck(uint8_t *buffer)
{
    uint16_t mz_signature = *based_pointer<uint16_t>(buffer, 0x0);
	uint32_t pe_offset = GetPEOffset(buffer);
	uint32_t pe_signature = *based_pointer<uint32_t>(buffer, pe_offset);

    return mz_signature == PackUInt16('M', 'Z') && pe_signature == PackUInt16('P', 'E');
}

std::map<std::string, Pe32SectionHeader> ExeFile::ParseSectionHeaders() {
	std::map<std::string, Pe32SectionHeader> info;
    uint32_t pe_offset = GetPEOffset(buffer);

    PeHeader *pe_header = based_pointer<PeHeader>(buffer, pe_offset);
    uint32_t section_headers_offset = pe_offset + sizeof(PeHeader) + pe_header->mSizeOfOptionalHeader;

	for (uint16_t i = 0; i < pe_header->mNumberOfSections; i++) {
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
