#include "ExeFile.h"

uint32_t GetPEOffset(unsigned char *buffer) {
	return *based_pointer<uint32_t>(buffer, 0x3C);
}

PeHeader ExeFile::GetPEHeader() {
    uint32_t pe_offset = GetPEOffset(raw_contents);

    return *based_pointer<PeHeader>(raw_contents, pe_offset);
}

Pe32OptionalHeader ExeFile::GetPEOptionalHeader() {
	uint32_t pe_offset = GetPEOffset(raw_contents);
	uint32_t pe_optional_offset = pe_offset + sizeof(PeHeader);

	return *based_pointer<Pe32OptionalHeader>(raw_contents, pe_optional_offset);
}

bool ExeFile::SignatureCheck(unsigned char *buffer)
{
    uint16_t mz_signature = *based_pointer<uint16_t>(buffer, 0x0);
	uint32_t pe_offset = GetPEOffset(buffer);
	uint32_t pe_signature = *based_pointer<uint32_t>(buffer, pe_offset);

    return mz_signature == PackUInt('M', 'Z') && pe_signature == PackUInt('P', 'E');
}

std::map<std::string, Pe32SectionHeader> ExeFile::ParseSectionHeaders() {
	std::map<std::string, Pe32SectionHeader> info;
    uint32_t pe_offset = GetPEOffset(raw_contents);
	
    PeHeader *pe_header = based_pointer<PeHeader>(raw_contents, pe_offset);
    Pe32OptionalHeader *optional_header = based_pointer<Pe32OptionalHeader>(raw_contents, pe_offset + sizeof(PeHeader));
    uint32_t section_headers_offset = pe_offset + sizeof(PeHeader) + pe_header->mSizeOfOptionalHeader;
    
	for (uint16_t i = 0; i < pe_header->mNumberOfSections; i++) {
        Pe32SectionHeader *section = based_pointer<Pe32SectionHeader>(
            raw_contents, section_headers_offset + (i * sizeof(Pe32SectionHeader))
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