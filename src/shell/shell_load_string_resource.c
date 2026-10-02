// shell_load_string_resource  (Ghidra: shell_load_string_resource, already named)
// address 0x57e110, size 130 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Loads a specific
//   localized string from a Win32 string-table resource by manually walking the resource block,
//   used as a language-aware alternative to LoadStringA." Standard Win32 string-table layout: a
//   FindResourceExA(RT_STRING=6) block holds 16 length-prefixed UTF-16 strings, block number
//   (id >> 4) + 1, string index id & 0xf.
// register convention: out/phase4/shell_types_notes.md: "shell_load_string_resource 0x57e110: id
//   in EAX, language in CX, buffer size in EBX, module in EDI, buffer on the stack." Confirmed by
//   objdump (every one of those registers is read before being written, and the buffer is the
//   only value loaded from the stack).
// blam-cc: id in EAX, language in CX (low word of ECX), buffer_capacity in EBX, module in EDI,
//   buffer on the stack.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"


// Loads Win32 string-table resource entry `id` (block (id>>4)+1, index id&0xf) from `module` for
// the given language, converting it to ANSI into `buffer` (capacity `buffer_capacity`).
// Returns the converted character count (excluding the terminator), or 0 on any failure.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t shell_load_string_resource(uint32_t id, uint16_t language, uint32_t buffer_capacity, void *module, char *buffer)
{
    void *resource_info;
    void *resource_data;
    const uint16_t *entry;
    uint32_t index;
    uint16_t entry_length;
    int32_t converted;
    int32_t terminator_index;

    resource_info = FindResourceExA((HMODULE)module, (const char *)6 /* RT_STRING */,
                                     (const char *)(uint32_t)((id >> 4) + 1), language);
    if (resource_info == 0) {
        return 0;
    }
    resource_data = LoadResource((HMODULE)module, (HRSRC)resource_info);
    if (resource_data == 0) {
        return 0;
    }
    entry = (const uint16_t *)LockResource(resource_data);
    if (entry == 0) {
        return 0;
    }

    index = 0;
    do {
        entry_length = *entry;
        entry++;
        if (entry_length != 0 && index == (id & 0xf)) {
            converted = WideCharToMultiByte(0, 0, (LPCWCH)entry, entry_length, buffer, buffer_capacity, 0, 0);
            if (converted == 0) {
                return 0;
            }
            terminator_index = (int32_t)buffer_capacity - 1;
            if (converted < terminator_index) {
                terminator_index = converted;
            }
            buffer[terminator_index] = 0;
            return converted;
        }
        index++;
        entry += entry_length;
    } while (index < 0x10);

    return 0;
}

#if 0
Original Ghidra decompilation (0x57e110):

int shell_load_string_resource(LPSTR param_1)

{
  uint in_EAX;
  HRSRC hResInfo;
  HGLOBAL hResData;
  LPCWSTR pWVar1;
  int iVar2;
  WORD in_CX;
  uint cchWideChar;
  int iVar3;
  uint uVar4;
  int unaff_EBX;
  HMODULE unaff_EDI;

  hResInfo = FindResourceExA(unaff_EDI,&DAT_00000006,(LPCSTR)((in_EAX >> 4) + 1 & 0xffff),in_CX);
  if (((hResInfo != (HRSRC)0x0) &&
      (hResData = LoadResource(unaff_EDI,hResInfo), hResData != (HGLOBAL)0x0)) &&
     (pWVar1 = LockResource(hResData), pWVar1 != (LPCWSTR)0x0)) {
    uVar4 = 0;
    do {
      cchWideChar = (uint)(ushort)*pWVar1;
      if ((cchWideChar != 0) && (uVar4 == (in_EAX & 0xf))) {
        iVar2 = WideCharToMultiByte(0,0,pWVar1 + 1,cchWideChar,param_1,unaff_EBX,(LPCSTR)0x0,
                                    (LPBOOL)0x0);
        if (iVar2 == 0) {
          return 0;
        }
        iVar3 = unaff_EBX + -1;
        if (iVar2 < unaff_EBX + -1) {
          iVar3 = iVar2;
        }
        param_1[iVar3] = '\0';
        return iVar2;
      }
      uVar4 = uVar4 + 1;
      pWVar1 = pWVar1 + 1 + cchWideChar;
    } while (uVar4 < 0x10);
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
