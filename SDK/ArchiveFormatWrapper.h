#pragma once

#include "ArchiveFormat.h"
#include "sdk.h"

class ArchiveFormatWrapper : public ArchiveFormat {
public:
    ArchiveFormatWrapper(const ArchiveFormatVTable* vtbl, sdk_ctx* ctx)
        : vtbl(vtbl), ctx(ctx), instance(vtbl->New(ctx)) {}

    ~ArchiveFormatWrapper() override {
        if (vtbl->Delete) vtbl->Delete(instance);
    }

    bool CanHandleFile(u8* buffer, u64 size, const std::string& ext) const override {
        return vtbl->CanHandleFile(instance, buffer, size, ext.c_str());
    }

    ArchiveBase* TryOpen(u8* buffer, u64 size, std::string file_name) override {
        return static_cast<ArchiveBase*>(
            vtbl->TryOpen(instance, buffer, size, file_name.c_str())
        );
    }

    std::string GetTag() const override {
        return vtbl->GetTag(instance);
    }

    std::string GetDescription() const override {
        return vtbl->GetDescription(instance);
    }

private:
    const ArchiveFormatVTable* vtbl;
    sdk_ctx* ctx;
    void* instance;
};

ArchiveFormatWrapper *AddArchiveFormat(struct sdk_ctx* ctx, const ArchiveFormatVTable* vtable);
