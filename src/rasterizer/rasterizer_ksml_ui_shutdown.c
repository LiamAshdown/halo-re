// rasterizer_ksml_ui_shutdown  (Ghidra: rasterizer_ksml_ui_shutdown, already named)
// address 0x5198a0, size 213 bytes
// name confidence: 0.55  rewrite confidence: 0.9
// evidence: guarded by a live engine instance + two engine callback pointers, finds and
//   releases (twice) the editbox and log document handles, then destroys the engine and clears
//   it; matches its own name and the functions.md summary.
// register convention: none -- __cdecl, no parameters.
// The compiler-generated x86 SEH frame (fs:[0] registration, push 0x672b68 / 0x628dfc) is plumbing and is not modeled;
//   the observable calls and their order are exactly the original's.
// VERIFIED against disassembly 0x5198a0..0x519970 (2026-09-30): guard on the three pointers, find/release x2 per document
//   (keys read from 0x69c698 / 0x69c69c), destroy, clear 0x721ea4.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *chat_gui_root_handle;                                  // 0x00721ea4 KSML UI engine instance (interface module name)
extern int32_t (*unknown_00721eb8)(void *engine, void *key); // 0x00721eb8: "find document"
extern void (*unknown_00721ec8)(int32_t document); // 0x00721ec8: "release document"
extern void (*unknown_00721eac)(void *engine); // 0x00721eac: "destroy engine"
extern void *chat_gui_find_object_arg; // 0x0069c698: editbox document key
extern void *chat_listbox_gui_find_object_arg; // 0x0069c69c: log document key

// Tears down the debug KSML UI engine: releases the editbox and log documents (each released
// twice, matching the original) and destroys the engine instance, only while all three engine
// pointers are live.
void __cdecl rasterizer_ksml_ui_shutdown(void)
{
    int32_t document;

    if (chat_gui_root_handle == (void *)0 || unknown_00721eb8 == (void *)0 || unknown_00721ec8 == (void *)0) {
        return;
    }

    document = unknown_00721eb8(chat_gui_root_handle, chat_gui_find_object_arg);
    if (document != 0) {
        unknown_00721ec8(document);
        unknown_00721ec8(document);
    }

    document = unknown_00721eb8(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
    if (document != 0) {
        unknown_00721ec8(document);
        unknown_00721ec8(document);
    }

    unknown_00721eac(chat_gui_root_handle);
    chat_gui_root_handle = (void *)0;
}

#if 0
Original Ghidra decompilation (0x5198a0):

void __cdecl rasterizer_ksml_ui_shutdown(void)

{
  int iVar1;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;

  puStack_c = &DAT_00672b68;
  puStack_10 = &LAB_00628dfc;
  local_14 = ExceptionList;
  local_8 = 0;
  if (((DAT_00721ea4 != 0) && (DAT_00721eb8 != (code *)0x0)) && (DAT_00721ec8 != (code *)0x0)) {
    ExceptionList = &local_14;
    iVar1 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c698);
    if (iVar1 != 0) {
      (*DAT_00721ec8)(iVar1);
      (*DAT_00721ec8)(iVar1);
    }
    iVar1 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c69c);
    if (iVar1 != 0) {
      (*DAT_00721ec8)(iVar1);
      (*DAT_00721ec8)(iVar1);
    }
    (*DAT_00721eac)(DAT_00721ea4);
    DAT_00721ea4 = 0;
  }
  ExceptionList = local_14;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
