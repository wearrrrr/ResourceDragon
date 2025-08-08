#pragma once

#include "ArchiveFormats/ArchiveFormat.h"
#include <map>
#include <memory>

typedef std::map<std::string, std::unique_ptr<ArchiveFormat>> FormatMap;
typedef std::vector<ArchiveFormat*> FormatList;

class ExtractorManager {
private:
  FormatMap m_formats;

public:
  void RegisterFormat(std::unique_ptr<ArchiveFormat> format) {
    m_formats.insert({format.get()->GetTag(), std::move(format)});
  }

  void UnregisterFormat(std::string tag) {
    m_formats.erase(tag);
  }

  const FormatMap& GetFormats() {
      return m_formats;
  }

  FormatList GetExtractorCandidates(u8 *buffer, u64 size, const std::string &ext) {
    auto format_list = FormatList();
    for (const auto &[name, format] : m_formats) {
      if (format->CanHandleFile(buffer, size, ext)) {
          format_list.push_back(format.get());
      }
    }
    return format_list;
  }
};
