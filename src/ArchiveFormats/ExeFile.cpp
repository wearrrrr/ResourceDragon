#include "ExeFile.h"

uint32_t GetPEOffset(unsigned char *buffer) {
	return *based_pointer<uint32_t>(buffer, 0x3C);
}

PeHeader ExeFile::GetPEHeader(unsigned char *buffer)
{
	uint32_t pe_offset = GetPEOffset(buffer);
	uint32_t pe_signature = *based_pointer<uint32_t>(buffer, pe_offset);
	PeHeader header = *based_pointer<PeHeader>(buffer, pe_offset);

	printf("Section Count: %d\n", header.mNumberOfSections);
    return header;
}

Pe32OptionalHeader ExeFile::GetPEOptionalHeader(unsigned char *buffer) {
	uint32_t pe_offset = GetPEOffset(buffer);
	uint32_t pe_signature = *based_pointer<uint32_t>(buffer, pe_offset);
	uint32_t pe_optional_offset = pe_offset + sizeof(PeHeader);

	return *based_pointer<Pe32OptionalHeader>(buffer, pe_optional_offset);
}

bool ExeFile::SignatureCheck(unsigned char *buffer)
{
    uint16_t mz_signature = *based_pointer<uint16_t>(buffer, 0x0);
	uint32_t pe_offset = GetPEOffset(buffer);
	uint32_t pe_signature = *based_pointer<uint32_t>(buffer, pe_offset);

	// MZ and PE in hex (reversed because endianness yayy)
    return mz_signature == 0x5A4D && pe_signature == 0x4550;
}

std::vector<Pe32SectionHeader> ExeFile::ParseSectionHeaders(unsigned char *buffer) {
	std::vector<Pe32SectionHeader> info;
    uint32_t pe_offset = GetPEOffset(buffer);
	
    PeHeader *pe_header = based_pointer<PeHeader>(buffer, pe_offset);
    Pe32OptionalHeader *optional_header = based_pointer<Pe32OptionalHeader>(buffer, pe_offset + sizeof(PeHeader));
    uint32_t section_headers_offset = pe_offset + sizeof(PeHeader) + pe_header->mSizeOfOptionalHeader;
    
	for (uint16_t i = 0; i < pe_header->mNumberOfSections; i++) {
        Pe32SectionHeader *section = based_pointer<Pe32SectionHeader>(
            buffer, section_headers_offset + (i * sizeof(Pe32SectionHeader))
        );
		info.push_back(*section);
        // printf("Section %d: Name = %.8s, VirtualSize = 0x%x, RawSize = 0x%x, RawDataPtr = 0x%x\n",
        //        i,
        //        section->mName,
        //        section->mVirtualSize,
        //        section->mSizeOfRawData,
        //        section->mPointerToRawData);
    }
	return info;
}

Pe32SectionHeader *ExeFile::GetSectionHeader(unsigned char *buffer, std::string target_section)
{
    std::vector<Pe32SectionHeader> sections = ParseSectionHeaders(buffer);

    for (auto &section : sections) {
        if (!strncmp(section.mName, target_section.c_str(), 8)) {
            return &section;
        }
    }


    return nullptr;
}

unsigned char* ExeFile::DumpDataFromSection(unsigned char *buffer, std::string target_section) {
    printf("Attempting to dump section '%s'\n", target_section.c_str());

    std::vector<Pe32SectionHeader> sections = ParseSectionHeaders(buffer);

    for (const auto &section : sections) {
        if (!strncmp(section.mName, target_section.c_str(), 8)) {
            printf("Found section! Dumping...\n");

            uint32_t size = section.mSizeOfRawData;
            uint32_t section_offset = section.mPointerToRawData;

            printf("Section offset in file: 0x%x\n", section_offset);
            printf("Section size: 0x%x\n", size);
            unsigned char* extracted_data = new unsigned char[size];
            std::memcpy(extracted_data, buffer + section_offset, size);

			// FILE* file = fopen("dump.bin", "wb");

			// fwrite(extracted_data, 1, size, file);
			// fclose(file);

            return extracted_data;
        }
    }

    printf("Section '%s' not found!\n", target_section.c_str());
    return nullptr;
}

bool ExeFile::ContainsSection(unsigned char *buffer, std::string section_name)
{
    std::vector<Pe32SectionHeader> sections = ParseSectionHeaders(buffer);

    for (const auto &section : sections) {
        if (!strncmp(section.mName, section_name.c_str(), 8)) {
            return true;
        }
    }

    return false;
}
