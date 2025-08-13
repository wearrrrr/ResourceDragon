#pragma once

#ifdef _WIN32
#include <shlwapi.h>
#include <util/int.h>
#include <windows.h>

#if _WIN32 || _WIN64
#if _WIN64
#define X64
#else
#define X32
#endif
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define X64
#else
#define X32
#endif
#endif

#define UNREACHABLE __builtin_unreachable()
#define TH_FALLTHROUGH [[fallthrough]]

namespace Stacktrace {
static HMODULE GetModuleContaining(void *addr) {
  HMODULE ret = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                              GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          (LPCWSTR)addr, &ret)) {
    // Just to be sure...
    return nullptr;
  }
  return ret;
}

static BOOL VirtualCheckRegion(const void *ptr, const size_t len) {
  MEMORY_BASIC_INFORMATION mbi;
  if (VirtualQuery(ptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0) {
    return FALSE;
  }

  auto ptr_end = (size_t)ptr + len;
  auto page_end = (size_t)mbi.BaseAddress + mbi.RegionSize;
  if (page_end < (ptr_end)) {
    return FALSE;
  }

  if (mbi.Protect == 0 || (mbi.Protect & PAGE_NOACCESS)) {
    return FALSE;
  }

  return TRUE;
}

static size_t StringToUTF8(char *str_utf8, const wchar_t *str_w, size_t str_utf8_len)
{
	int ret = WideCharToMultiByte(
		CP_UTF8, 0, str_w, -1, str_utf8, str_utf8_len, NULL, NULL
	);
	return str_w ? ret - 1 : ret;
}

static UINT fallback_codepage = CP_ACP;

static LPCSTR WINAPI CharNextU(
	LPCSTR lpsz
)
{
	LPCSTR ret;
	extern UINT fallback_codepage;

	if(lpsz == NULL || *lpsz == '\0') {
		ret = lpsz;
	}
	if ((lpsz[0] & 0xe0) == 0xc0 && (lpsz[1] & 0xc0) == 0x80) {
		ret = lpsz + 2;
	}
	else if ((lpsz[0] & 0xf0) == 0xe0 && (lpsz[1] & 0xc0) == 0x80 && (lpsz[2] & 0xc0) == 0x80) {
		ret = lpsz + 3;
	}
	else if ((lpsz[0] & 0xf8) == 0xf0 && (lpsz[1] & 0xc0) == 0x80 && (lpsz[2] & 0xc0) == 0x80 && (lpsz[3] & 0xc0) == 0x80) {
		ret = lpsz + 4;
	}
	else if(IsDBCSLeadByteEx(fallback_codepage, lpsz[0])) {
		size_t lpsz_len = strlen(lpsz);
		if(lpsz_len < 2) {
			ret = lpsz + 1;
		} else {
			ret = lpsz + 2;
		}
	}
	else {
		ret = lpsz + 1;
	}
	return ret;
}

static LPSTR STDAPICALLTYPE PathFindFileNameU(
	LPCSTR pszPath
)
{
	LPCSTR ret = pszPath;
	while(pszPath && pszPath[0]) {
		char c0 = pszPath[0];
		char c1 = pszPath[1];
		if(
			(c0 == '\\' || c0 == '/' || c0 == ':')
			&& (c1 && c1 != '\\' && c1 != '/')
		) {
			ret = pszPath;
		}
		pszPath = CharNextU(pszPath);
	}
	return (LPSTR)ret;
}

static DWORD WINAPI GetModuleFileNameU(
	HMODULE hModule,
	LPSTR lpFilename,
	DWORD nSize
)
{
	/**
	  * And here we are, the most stupid Win32 API function I've seen so far.
	  *
	  * This wrapper adds the "GetCurrentDirectory functionality" the original
	  * function unfortunately lacks. Pass NULL for [lpFilename] or [nSize] to
	  * get the size required for a buffer to hold the module name in UTF-8.
	  *
	  * ... and unless there is any alternative function I don't know of, the
	  * only way to actually calculate this size is to repeatedly increase a
	  * buffer and to check whether that has been enough.
	  *
	  * In practice though, this length should never exceed MAX_PATH. I failed to
	  * create any test case where the path would be larger. But just in case it
	  * is or this becomes more frequent some day, the code is here.
	  */

	// Only real VLAs can safely change size in a loop. Doing this with _malloca
	// or _alloca just continually expands the stack and never deallocates.

	DWORD wide_len = nSize ? nSize : MAX_PATH;
#if !VLA_SUPPORT
	wchar_t* lpFilename_w = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
#endif
	for (;;) {
#if VLA_SUPPORT
		wchar_t lpFilename_w[wide_len];
#endif

		DWORD ret = GetModuleFileNameW(hModule, lpFilename_w, wide_len);
		if (ret) {
			if (ret == wide_len) {
				wide_len += MAX_PATH;
#if !VLA_SUPPORT
				lpFilename_w = (wchar_t*)realloc(lpFilename_w, wide_len * sizeof(wchar_t));
#endif
				continue;
			}
			// The last error and return value will be set correctly
			// by the WideCharToMultiByte call inside StringToUTF8
			ret = StringToUTF8(lpFilename, lpFilename_w, nSize);
#if !VLA_SUPPORT
			free(lpFilename_w);
#endif
		}
		return ret;
	}
}

static void log_print_rva_and_module(HMODULE mod, void *addr) {
  DWORD crash_fn_len = GetModuleFileNameU(mod, NULL, 0) + 1;
  char *crash_fn = (char *)malloc(crash_fn_len * sizeof(char));
  if (GetModuleFileNameU(mod, crash_fn, crash_fn_len)) {
    printf(" (Rx%zX) (%s)", (uintptr_t)addr - (uintptr_t)mod,
           PathFindFileNameU(crash_fn));
  }
  free(crash_fn);
}

static void log_print_error_source(HMODULE crash_mod, void *address) {
  if (crash_mod) {
    log_print_rva_and_module(crash_mod, address);
  }
}

static void win_stack_walk(uintptr_t current_esp) {
#ifdef X64
  NT_TIB *tib = (NT_TIB *)__readgsqword(offsetof(NT_TIB, Self));
#else
  NT_TIB *tib = (NT_TIB *)__readfsdword(offsetof(NT_TIB, Self));
#endif

#ifdef X64
#define StkPtrReg "RSP"
#define InsPtrReg "RIP"
#else
#define StkPtrReg "ESP"
#define InsPtrReg "EIP"
#endif

  uintptr_t *stack_top = (uintptr_t *)tib->StackBase;
  uintptr_t *stack_bottom = (uintptr_t *)tib->StackLimit;

  if (!(current_esp < (uintptr_t)stack_top &&
        current_esp >= (uintptr_t)stack_bottom)) {
    printf(
        "\n"
        "Stack walk ERROR: " StkPtrReg
        " is not within the stack bounds specified by the TEB! (0x%p->0x%p)\n",
        stack_bottom, stack_top);
    return;
  }

  printf("\n"
         "Stack walk: (0x%p->0x%p)\n",
         current_esp, stack_top);

  if (((uintptr_t)stack_top - current_esp) % sizeof(uintptr_t)) {
    puts("WARNING: Stack is not aligned, data may be unreliable.");
  }
  MEMORY_BASIC_INFORMATION mbi;
  enum PtrState_TypeVals : uint8_t {
    RawValue = 0,
    PossiblePointer = 1,
    ReturnAddrIndirect2 = 2,
    ReturnAddrIndirect3 = 3,
    ReturnAddrIndirect4 = 4,
    ReturnAddr = 5,
    ReturnAddrIndirect6 = 6,
    ReturnAddrIndirect7 = 7,
    FarReturnAddrIndirect2 = 10,
    FarReturnAddrIndirect3 = 11,
    FarReturnAddrIndirect4 = 12,
    FarReturnAddr = 13, // Not valid in x64
    FarReturnAddrIndirect6 = 14,
    FarReturnAddrIndirect7 = 15
  };
  enum PtrState_AccessVals : uint8_t {
    None = 0,
    ReadOnly = 1,
    WriteCopy = 2,
    ReadWrite = 3,
    Execute = 4,
    ExecuteRead = 5,
    ExecuteWriteCopy = 6,
    ExecuteReadWrite = 7,
  };
  static constexpr const char *AccessStrings[] = {
      "(No Access)",          "(Read Only)",         "(Write-Copy)",
      "(Read Write)",         "(Execute Only)",      "(Execute Read)",
      "(Execute Write-Copy)", "(Execute Read Write)"};
  uint8_t stack_offset_length =
      snprintf(NULL, 0, "%zX", (uintptr_t)(stack_top - 1) - current_esp);
  for (uintptr_t *stack_addr = (uintptr_t *)current_esp;
       stack_addr < stack_top - 1; // These don't necessarily align cleanly, so
                                   // array indexing would be tricky to use here
       ++stack_addr) {
    uintptr_t stack_value = *stack_addr;

    uint8_t ptr_value_type = RawValue;
    uint8_t ptr_access_type = None;
    bool ptr_on_stack = false;
    bool has_segment_override = false;
#ifdef X64
    bool has_rex_byte = false;
#endif
    if (stack_value <
        65536) { // Thin out values that can't ever be a valid pointer
      if (!(true)) {
        // Memory protection hasn't been modified, so just continue the loop
        continue;
      }
      ptr_value_type = RawValue;
    } else {
      // Is this a pointer to other stack data?
      if (stack_value < (uintptr_t)stack_top &&
          stack_value >= (uintptr_t)current_esp) {
        ptr_on_stack = true;
      }
      // Is this a pointer to meaningful memory?
      if (VirtualQuery((void *)stack_value, &mbi,
                       sizeof(MEMORY_BASIC_INFORMATION))) {
        switch (mbi.Protect & 0xFF) {
        default:
        case PAGE_NOACCESS: // ???
          ptr_access_type = None;
          break;
        case PAGE_READONLY: // Probably a pointer to a constant
          ptr_access_type = ReadOnly;
          break;
        case PAGE_WRITECOPY:
          if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READONLY,
                             &mbi.Protect)) {
            ptr_access_type = WriteCopy;
          } else {
            ptr_access_type = None;
          }
          break;
        case PAGE_READWRITE: // Probably a pointer to data
          ptr_access_type = ReadWrite;
          break;
        case PAGE_EXECUTE:
          if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READONLY,
                             &mbi.Protect)) {
            ptr_access_type = Execute;
          } else {
            ptr_access_type = None;
          }
          break;
        case PAGE_EXECUTE_READ: // Probably a return address
          ptr_access_type = ExecuteRead;
          break;
        case PAGE_EXECUTE_WRITECOPY: // Probably a return address, but we can't
                                     // read it
          if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READONLY,
                             &mbi.Protect)) {
            ptr_access_type = ExecuteWriteCopy;
          } else {
            ptr_access_type = None;
          }
          break;
        case PAGE_EXECUTE_READWRITE: // Probably a codecave
          ptr_access_type = ExecuteReadWrite;
          break;
        }
      }
      switch (ptr_access_type) {
      default:
        UNREACHABLE;
      // I draw the line at repeated prefixes, 16-bit calls, and 16-bit
      // addressing. Screw that.
      case Execute:
      case ExecuteRead:
      case ExecuteWriteCopy:
      case ExecuteReadWrite:
        switch (stack_value - (uintptr_t)mbi.BaseAddress) {
        default:
#ifdef X64
          switch (*(uint8_t *)(stack_value - 9)) {
          case 0x26:
          case 0x2E:
          case 0x36:
          case 0x3E:
          case 0x64:
          case 0x65: // ES, CS, SS, DS, FS, GS
            has_segment_override = true;
            break;
          }
          TH_FALLTHROUGH;
        case 8:
#endif
          switch (*(uint8_t *)(stack_value - 8)) {
#ifdef X64
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
          case 0x48:
          case 0x49:
          case 0x4A:
          case 0x4B:
          case 0x4C:
          case 0x4D:
          case 0x4E:
          case 0x4F:
            has_rex_byte = true;
            break;
#endif
          case 0x26:
          case 0x2E:
          case 0x36:
          case 0x3E:
          case 0x64:
          case 0x65: // ES, CS, SS, DS, FS, GS
            has_segment_override = true;
            break;
          }
          TH_FALLTHROUGH;
        case 7:
          switch (*(uint8_t *)(stack_value - 7)) {
          case 0xFF: { // CALL Absolute Indirect
            uint8_t mod_byte = *(uint8_t *)(stack_value - 6);
            bool ptr_is_far = mod_byte != (mod_byte & 0xF7);
            mod_byte &= 0xF7;
            // Only ModRM 14 and 94 can be encoded as 7 bytes long
            switch (mod_byte) {
            case 0x14: // [disp32] (SIB)
              // ModRM 14 can only be 7 bytes long with SIB base == 5
              if ((*(uint8_t *)(stack_value - 5) & 0x7) != 5)
                break;
              TH_FALLTHROUGH;
            case 0x94: // [reg+disp32] (SIB)
              ptr_value_type =
                  !ptr_is_far ? ReturnAddrIndirect7 : FarReturnAddrIndirect7;
              goto IdentifiedValueType;
            }
          }
            TH_FALLTHROUGH;
          default:
            has_segment_override = false;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
#ifdef X64
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
          case 0x48:
          case 0x49:
          case 0x4A:
          case 0x4B:
          case 0x4C:
          case 0x4D:
          case 0x4E:
          case 0x4F:
            has_rex_byte = true;
            break;
#else
          case 0x9A: // FAR CALL Absolute Direct
            ptr_value_type = FarReturnAddr;
            goto IdentifiedValueType;
#endif
          case 0x26:
          case 0x2E:
          case 0x36:
          case 0x3E:
          case 0x64:
          case 0x65: // ES, CS, SS, DS, FS, GS
            has_segment_override = true;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
          }
          TH_FALLTHROUGH;
        case 6:
          switch (*(uint8_t *)(stack_value - 6)) { // CALL Absolute Indirect
          case 0xFF: {
            uint8_t mod_byte = *(uint8_t *)(stack_value - 5);
            bool ptr_is_far = mod_byte != (mod_byte & 0xF7);
            mod_byte &= 0xF7;
            // Only 15, 90-93, and 95-97 can be encoded as 6 bytes long
            if (mod_byte == 0x15 || // [disp32] (No SIB)
                (mod_byte & 0xF8) == 0x90 &&
                    (mod_byte & 0x7) != 4 // [reg+disp32] (No SIB)
            ) {
              ptr_value_type =
                  !ptr_is_far ? ReturnAddrIndirect6 : FarReturnAddrIndirect6;
              goto IdentifiedValueType;
            }
          }
            TH_FALLTHROUGH;
          default:
            has_segment_override = false;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
#ifdef X64
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
          case 0x48:
          case 0x49:
          case 0x4A:
          case 0x4B:
          case 0x4C:
          case 0x4D:
          case 0x4E:
          case 0x4F:
            has_rex_byte = true;
            break;
#endif
          case 0x26:
          case 0x2E:
          case 0x36:
          case 0x3E:
          case 0x64:
          case 0x65: // ES, CS, SS, DS, FS, GS
            has_segment_override = true;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
          }
          TH_FALLTHROUGH;
        case 5:
          switch (*(uint8_t *)(stack_value - 5)) { // CALL Relative Displacement
          case 0xE8:
            ptr_value_type = ReturnAddr;
            goto IdentifiedValueType;
          default:
            has_segment_override = false;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
#ifdef X64
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
          case 0x48:
          case 0x49:
          case 0x4A:
          case 0x4B:
          case 0x4C:
          case 0x4D:
          case 0x4E:
          case 0x4F:
            has_rex_byte = true;
            break;
#endif
          case 0x26:
          case 0x2E:
          case 0x36:
          case 0x3E:
          case 0x64:
          case 0x65: // ES, CS, SS, DS, FS, GS
            has_segment_override = true;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
          }
          TH_FALLTHROUGH;
        case 4:
          switch (*(uint8_t *)(stack_value - 4)) { // CALL Absolute Indirect
          case 0xFF: {
            uint8_t mod_byte = *(uint8_t *)(stack_value - 3);
            bool ptr_is_far = mod_byte != (mod_byte & 0xF7);
            mod_byte &= 0xF7;
            // Only 54 can be encoded as 4 bytes long
            if (mod_byte == 0x54) { // [ESP+disp8] (SIB)
              ptr_value_type =
                  !ptr_is_far ? ReturnAddrIndirect4 : FarReturnAddrIndirect4;
              goto IdentifiedValueType;
            }
          }
            TH_FALLTHROUGH;
          default:
            has_segment_override = false;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
#ifdef X64
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
          case 0x48:
          case 0x49:
          case 0x4A:
          case 0x4B:
          case 0x4C:
          case 0x4D:
          case 0x4E:
          case 0x4F:
            has_rex_byte = true;
            break;
#endif
          case 0x26:
          case 0x2E:
          case 0x36:
          case 0x3E:
          case 0x64:
          case 0x65: // ES, CS, SS, DS, FS, GS
            has_segment_override = true;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
          }
          TH_FALLTHROUGH;
        case 3:
          switch (*(uint8_t *)(stack_value - 3)) { // CALL Absolute Indirect
          case 0xFF: {
            uint8_t mod_byte = *(uint8_t *)(stack_value - 2);
            bool ptr_is_far = mod_byte != (mod_byte & 0xF7);
            mod_byte &= 0xF7;
            // Only 14, 50-53, and 55-57 can be encoded as 3 bytes long
            switch (mod_byte) {
            case 0x14: // [reg] (SIB)
              // ModRM 14 can only be 3 bytes long with SIB base != 5
              if ((*(uint8_t *)(stack_value - 1) & 0x7) == 5)
                break;
              TH_FALLTHROUGH;
            case 0x50:
            case 0x51:
            case 0x52:
            case 0x53:
            case 0x55:
            case 0x56:
            case 0x57: // [reg+disp8] (No SIB)
              ptr_value_type =
                  !ptr_is_far ? ReturnAddrIndirect3 : FarReturnAddrIndirect3;
              goto IdentifiedValueType;
            }
          }
            TH_FALLTHROUGH;
          default:
            has_segment_override = false;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
#ifdef X64
          case 0x40:
          case 0x41:
          case 0x42:
          case 0x43:
          case 0x44:
          case 0x45:
          case 0x46:
          case 0x47:
          case 0x48:
          case 0x49:
          case 0x4A:
          case 0x4B:
          case 0x4C:
          case 0x4D:
          case 0x4E:
          case 0x4F:
            has_rex_byte = true;
            break;
#endif
          case 0x26:
          case 0x2E:
          case 0x36:
          case 0x3E:
          case 0x64:
          case 0x65: // ES, CS, SS, DS, FS, GS
            has_segment_override = true;
#ifdef X64
            has_rex_byte = false;
#endif
            break;
          }
          TH_FALLTHROUGH;
        case 2:
          if (*(uint8_t *)(stack_value - 2) == 0xFF) { // CALL Absolute Indirect
            uint8_t mod_byte = *(uint8_t *)(stack_value - 1);
            bool ptr_is_far = mod_byte != (mod_byte & 0xF7);
            mod_byte &= 0xF7;
            // Only 10-13, 16, 17, and D0-D7 can be encoded as 2 bytes long
            switch (mod_byte & 0xF8) {
            case 0x10: // [reg] (No SIB)
              if ((mod_byte & 0x6) == 0x4)
                break; // Filter out [reg] (SIB) and [disp32]
              TH_FALLTHROUGH;
            case 0xD0: // reg
              ptr_value_type =
                  !ptr_is_far ? ReturnAddrIndirect2 : FarReturnAddrIndirect2;
              goto IdentifiedValueType;
            }
          }
          TH_FALLTHROUGH;
        case 1:
        case 0:
          break;
        }
        TH_FALLTHROUGH;
      case ReadOnly:
      case WriteCopy:
      case ReadWrite:
        if (VirtualCheckRegion((void *)stack_value, 16)) {
          ptr_value_type = PossiblePointer;
        } else {
          TH_FALLTHROUGH;
        case None:
          ptr_value_type = RawValue;
        }
        if (!(true)) {
          // Memory protection might have been modified, so jump to the end
          // where it's set back
          goto SkipPrint;
        }
        break;
      }
    }
  IdentifiedValueType:
    switch (ptr_value_type) {
    default:
      UNREACHABLE;
    case RawValue:
      printf("[" StkPtrReg "+%*X]\t%p", stack_offset_length,
             (uintptr_t)stack_addr - current_esp, stack_value);
      break;
    case PossiblePointer: {
      printf("[" StkPtrReg "+%*X]\t%p\t", stack_offset_length,
             (uintptr_t)stack_addr - current_esp, stack_value);
      if (false) {
        printf("DUMP %08X %08X %08X %08X ",
               _byteswap_ulong(((uint32_t *)stack_value)[0]),
               _byteswap_ulong(((uint32_t *)stack_value)[1]),
               _byteswap_ulong(((uint32_t *)stack_value)[2]),
               _byteswap_ulong(((uint32_t *)stack_value)[3]));
      }
      printf("%s", ptr_on_stack ? "(Stack)" : AccessStrings[ptr_access_type]);
      if (!ptr_on_stack) {
        log_print_error_source(GetModuleContaining((void *)stack_value),
                               (void *)stack_value);
      }
      break;
    }
    case ReturnAddr: {
      uintptr_t call_dest_addr =
          (intptr_t)stack_value + *(int32_t *)(stack_value - 4);
      if (stack_value != call_dest_addr) {
        printf("[" StkPtrReg "+%*X]\t%p\tRETURN to %p", stack_offset_length,
               (uintptr_t)stack_addr - current_esp, stack_value, stack_value);
        log_print_error_source(GetModuleContaining((void *)stack_value),
                               (void *)stack_value);
        printf(" from func %p", call_dest_addr);
        log_print_error_source(GetModuleContaining((void *)call_dest_addr),
                               (void *)call_dest_addr);
      } else {
#ifdef X64
        uintptr_t call_addr =
            stack_value - (5 + has_segment_override + has_rex_byte);
#else
        uintptr_t call_addr = stack_value - (5 + has_segment_override);
#endif
        printf("[" StkPtrReg "+%*X]\t%p\t" InsPtrReg " pushed by CALL 0 at %p",
               stack_offset_length, (uintptr_t)stack_addr - current_esp,
               stack_value, call_addr);
        log_print_error_source(GetModuleContaining((void *)call_addr),
                               (void *)call_addr);
      }
      break;
    }
    case ReturnAddrIndirect2:
    case ReturnAddrIndirect3:
    case ReturnAddrIndirect4:
    case ReturnAddrIndirect6:
    case ReturnAddrIndirect7: {
      printf("[" StkPtrReg "+%*X]\t%p\tRETURN to %p", stack_offset_length,
             (uintptr_t)stack_addr - current_esp, stack_value, stack_value);
      log_print_error_source(GetModuleContaining((void *)stack_value),
                             (void *)stack_value);
#ifdef X64
      uintptr_t call_addr =
          stack_value - (ptr_value_type + has_segment_override + has_rex_byte);
#else
      uintptr_t call_addr =
          stack_value - (ptr_value_type + has_segment_override);
#endif
      printf(" (Indirect call from %p", call_addr);
      log_print_error_source(GetModuleContaining((void *)call_addr),
                             (void *)call_addr);
      puts(")");
      break;
    }
#ifndef X64
    case FarReturnAddr: {
      printf("[" StkPtrReg "+%*X]\t%p\tFAR RETURN to %04hX:%p",
             stack_offset_length, (uintptr_t)stack_addr - current_esp,
             stack_value, *(uint16_t *)(stack_addr + 1), stack_value);
      log_print_error_source(GetModuleContaining((void *)stack_value),
                             (void *)stack_value);
      uintptr_t call_dest_addr = *(uintptr_t *)(stack_value - 6);
      printf(" from func %04hX:%p", *(uint16_t *)(stack_value - 2),
             call_dest_addr);
      log_print_error_source(GetModuleContaining((void *)call_dest_addr),
                             (void *)call_dest_addr);
      break;
    }
#endif
    case FarReturnAddrIndirect2:
    case FarReturnAddrIndirect3:
    case FarReturnAddrIndirect4:
    case FarReturnAddrIndirect6:
    case FarReturnAddrIndirect7: {
      printf("[" StkPtrReg "+%*X]\t%p\tFAR RETURN to %04hX:%p",
             stack_offset_length, (uintptr_t)stack_addr - current_esp,
             stack_value, *(uint16_t *)(stack_addr + 1), stack_value);
      log_print_error_source(GetModuleContaining((void *)stack_value),
                             (void *)stack_value);
#ifdef X64
      uintptr_t call_addr = stack_value - ((ptr_value_type - 8) +
                                           has_segment_override + has_rex_byte);
#else
      uintptr_t call_addr =
          stack_value - ((ptr_value_type - 8) + has_segment_override);
#endif
      printf(" (Indirect call from %p", call_addr);
      log_print_error_source(GetModuleContaining((void *)call_addr),
                             (void *)call_addr);
      puts(")");
    }
    }
    puts("");
  SkipPrint:
    switch (ptr_access_type) {
    case WriteCopy:
    case Execute:
    case ExecuteWriteCopy:
      VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect,
                     &mbi.Protect);
    }
  }
}
} // namespace Stacktrace
#endif
