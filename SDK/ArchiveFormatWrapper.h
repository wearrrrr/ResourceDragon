#pragma once

#include "../src/ArchiveFormats/ArchiveFormat.h"
#include "sdk.h"
#include <cstring>

class ArchiveBaseWrapper : public ArchiveBase {
public:
    ArchiveBaseWrapper(ArchiveBaseHandle handle) : handle(handle) {}

    ~ArchiveBaseWrapper() {
        if (handle.vtable && handle.vtable->Destroy)
            handle.vtable->Destroy(handle.inst);
    }

    EntryMapPtr GetEntries() override {
        EntryMapPtr entries;
        size_t count = handle.vtable->GetEntryCount(handle.inst);

        for (size_t i = 0; i < count; i++) {
            Entry* e = new Entry();
            e->name = handle.vtable->GetEntryName(handle.inst, i);
            e->size = handle.vtable->GetEntrySize(handle.inst, i);
            // data will be fetched later in OpenStream
            entries[e->name] = e;
        }

        return entries;
    }

    u8* OpenStream(const Entry *entry, u8 *buffer) override {
        if (!handle.vtable || !handle.vtable->OpenStream) return nullptr;

        size_t count = handle.vtable->GetEntryCount(handle.inst);
        for (size_t i = 0; i < count; i++) {
            const char* name = handle.vtable->GetEntryName(handle.inst, i);
            if (name && entry->name == name) {
                usize out_size = 0;
                u8* data = handle.vtable->OpenStream(handle.inst, i, &out_size);
                return data;
            }
        }
        return nullptr;
    }

private:
    ArchiveBaseHandle handle;
};


class ArchiveFormatWrapper : public ArchiveFormat {
public:
    ArchiveFormatWrapper(const ArchiveFormatVTable* vtbl, sdk_ctx* ctx, ArchiveHandle inst)
      : vtbl(vtbl), ctx(ctx), inst(inst) {}

    ~ArchiveFormatWrapper() override {
        if (vtbl && vtbl->Delete && inst) vtbl->Delete(inst);
    }

    virtual bool CanHandleFile(u8* buffer, u64 size, const std::string& ext) const override {
        if (!vtbl || !vtbl->CanHandleFile) return false;
        return vtbl->CanHandleFile(inst, buffer, size, ext.c_str()) != 0;
    }

    virtual ArchiveBase* TryOpen(u8* buffer, u64 size, std::string file_name) override {
        if (!vtbl || !vtbl->TryOpen) return nullptr;
        ArchiveBaseHandle h = vtbl->TryOpen(inst, buffer, size, file_name.c_str());
        if (h.vtable == nullptr) return nullptr;
         // adapter that converts ArchiveBaseHandle -> ArchiveBase*
        return new ArchiveBaseWrapper(h);
    }

    virtual std::string GetTag() const override {
        if (!vtbl || !vtbl->GetTag) return "??";
        const char* t = vtbl->GetTag(inst);
        return t ? std::string(t) : std::string("??");
    }

    virtual std::string GetDescription() const override {
        if (!vtbl || !vtbl->GetDescription) return "??";
        const char* d = vtbl->GetDescription(inst);
        return d ? std::string(d) : std::string("??");
    }
private:
    const ArchiveFormatVTable* vtbl;
    sdk_ctx* ctx;
    ArchiveHandle inst;
};


ArchiveFormatWrapper *AddArchiveFormat(struct sdk_ctx* ctx, const ArchiveFormatVTable* vtable);
