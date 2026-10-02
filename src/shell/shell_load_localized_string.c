// shell_load_localized_string  (Ghidra: shell_load_localized_string, already named)
// address 0x57e1a0, size 75 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Loads a localized
//   UI string for resource id param_1, preferring FUN_0057e110's language-aware lookup and
//   falling back to LoadStringA." objdump confirms the fallback path retries
//   shell_load_string_resource once more in English (0x409) before finally calling LoadStringA.
// register convention: objdump: this in_EAX -> EBX (LoadStringA's cchBufferMax), in_ECX -> EDI
//   (LoadStringA's hInstance, also shell_load_string_resource's module), unaff_ESI is the buffer
//   pointer (live-in, unmodified), and the stack holds the resource id. The two
//   shell_load_string_resource calls pass shell_language_id (0x0069ff20) as the language the
//   first time and the literal 0x409 (English) the second time.
// blam-cc: buffer_capacity in EAX, module in ECX, buffer in ESI, resource_id on the stack.
// Review fix: returns EAX, the nonzero loader result or LoadStringA's length (0x57e1bc..0x57e1e7);
//   chat_dispatch_incoming (interface) tests it. The first rewrite returned void.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t shell_load_string_resource(uint32_t id, uint16_t language, uint32_t buffer_capacity, void *module,
                                           char *buffer); // 0x57e110
extern uint32_t shell_language_id; // 0x0069ff20

// Loads a localized UI string for resource id `id`: tries shell_load_string_resource in the
// current language, then (if that fails and the current language is not already English) again
// in English, and only falls back to the plain Win32 LoadStringA if both attempts fail. Returns
// the character count, 0 when nothing was found.
int32_t shell_load_localized_string(uint32_t buffer_capacity, void *module, char *buffer, uint32_t id)
{
    int32_t loaded;

    loaded = shell_load_string_resource(id, (uint16_t)shell_language_id, buffer_capacity, module, buffer);
    if (loaded != 0) {
        return loaded;
    }
    if (shell_language_id != k_shell_language_default) {
        loaded = shell_load_string_resource(id, (uint16_t)k_shell_language_default, buffer_capacity, module, buffer);
        if (loaded != 0) {
            return loaded;
        }
    }
    return LoadStringA((HINSTANCE)module, id, buffer, buffer_capacity);
}

#if 0
Original Ghidra decompilation (0x57e1a0):

void __cdecl shell_load_localized_string(UINT param_1)

{
  int in_EAX;
  int iVar1;
  HINSTANCE in_ECX;
  LPSTR unaff_ESI;

  iVar1 = shell_load_string_resource();
  if (iVar1 == 0) {
    if ((DAT_0069ff20 != 0x409) && (iVar1 = shell_load_string_resource(), iVar1 != 0)) {
      return;
    }
    LoadStringA(in_ECX,param_1,unaff_ESI,in_EAX);
  }
  return;
}
#endif
