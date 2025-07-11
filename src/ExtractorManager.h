#pragma once

#include "ArchiveFormats/ArchiveFormat.h"
#include <map>
#include <memory>

typedef std::map<std::string, std::unique_ptr<ArchiveFormat>> FormatMap;

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

  FormatMap& GetFormats() {
      return m_formats;
  }

  ArchiveFormat *getExtractorFor(u8 *buffer, u64 size, const std::string &ext) {
    for (const auto &format : m_formats) {
      /* TODO:
        this is bad! we should not be returning the first one that happens to
        meet the criteria we should instead return a vector or something and
        try to extract each one or, we can make absolutely certain that
        ArchiveFormat::CanHandleFile ONLY returns the one we need
      */
      if (format.second->CanHandleFile(buffer, size, ext)) {
        return format.second.get();
      }
    }
    return nullptr;
  }
};
