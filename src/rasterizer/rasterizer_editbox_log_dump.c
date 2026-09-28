// rasterizer_editbox_log_dump  (Ghidra: rasterizer_editbox_log_dump, already named)
// address 0x5196b0, size 484 bytes
// name confidence: 0.6   rewrite confidence: 0.35
// evidence: lazily creates the KSML UI engine, builds "content/<height>editbox.ksml" and
//   "content/<height>log.ksml" wide-string paths (height from
//   rasterizer_round_up_resolution_height applied to the cached present parameters), and loads
//   them into the engine, releasing whatever document the editbox path previously resolved to
//   first.
// register convention: none -- no parameters.
// UNSURE, substantially: the "content/" prefix is reconstructed from four raw dword stack
//   stores Ghidra mistyped as pointers (0x006f0063/0x0074006e/0x006e0065/0x2f0074, each two
//   packed UTF-16 code units) -- decoded here as the literal wide string "content/" rather than
//   reproduced as raw pointer writes, since the actual mechanism (packed immediate stores into a
//   stack buffer) has no behavioural difference from a string literal once compiled. The four
//   engine callback pointers (0x00721ea0/eb4/eb8/edc/ec8) are not documented in
//   types/rasterizer.h; their signatures are best-effort guesses from this call site only.

// reconciled: the document key is passed by value (the global's contents), not its address; with the address the
//   edit box document was never found again, so the step that hides it never ran (the 'Prompt' box in game).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <wchar.h>
#include <stdlib.h>

extern void *chat_gui_root_handle;                                  // 0x00721ea4 KSML UI engine instance (interface module name)
extern void *(*unknown_00721ea0)(void *hwnd, void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e); // 0x00721ea0 UNSURE: engine factory
extern uint32_t keystone_current_directory; // 0x00721ea8 UNSURE: engine factory argument
extern void *rasterizer_device; // 0x0071d174
extern void *shell_window;   // 0x007461c4
extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern void *chat_gui_find_object_arg; // 0x0069c698, see rasterizer_ksml_ui_shutdown.c
extern void *chat_listbox_gui_find_object_arg; // 0x0069c69c, see rasterizer_ksml_ui_shutdown.c
extern int32_t (*unknown_00721eb4)(void *engine, void *path, void *key, uint32_t flags, void *rect,
                                    uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f); // 0x00721eb4 UNSURE: "load document"
extern int32_t (*unknown_00721eb8)(void *engine, void *key); // 0x00721eb8 UNSURE: "find document", see rasterizer_ksml_ui_shutdown.c
extern void (*unknown_00721edc)(int32_t document, uint32_t a); // 0x00721edc UNSURE
extern void (*unknown_00721ec8)(int32_t document); // 0x00721ec8 UNSURE: "release document"

extern int32_t rasterizer_round_up_resolution_height(int32_t height); // 0x5195f0 (this session)

// Lazily creates the debug KSML UI engine, then (re)loads the editbox and log KSML layout files
// from a resolution-specific "content/" subfolder.
void rasterizer_editbox_log_dump(void)
{
    wchar_t editbox_path[64];
    wchar_t log_path[64];
    wchar_t height_text[32];
    void *rect_zero[4]; // uStack_d0/cc/c8/c4: {0, 0, back_buffer_width, back_buffer_height}
    int32_t document;

    if (chat_gui_root_handle == (void *)0) {
        if (unknown_00721ea0 == (void *)0) {
            return;
        }
        chat_gui_root_handle = unknown_00721ea0(shell_window, rasterizer_device, keystone_current_directory, 0, 0, 0, 0);
        if (chat_gui_root_handle == (void *)0) {
            return;
        }
    }

    rect_zero[0] = (void *)0;
    rect_zero[1] = (void *)0;
    rect_zero[2] = (void *)(uintptr_t)rasterizer_present_parameters.back_buffer_width;
    rect_zero[3] = (void *)(uintptr_t)rasterizer_present_parameters.back_buffer_height;

    wcscpy(log_path, L"content/");
    wcscpy(editbox_path, L"content/");

    _itow(rasterizer_round_up_resolution_height(rasterizer_present_parameters.back_buffer_height),
          height_text, 10);

    wcscat(editbox_path, height_text);
    wcscat(editbox_path, L"editbox.ksml");
    wcscat(log_path, height_text);
    wcscat(log_path, L"log.ksml");

    unknown_00721eb4(chat_gui_root_handle, editbox_path, chat_gui_find_object_arg, 0x10000000, /* 0x519802: mov eax,[0x69c698]; push eax */ rect_zero, 0, 0, 0, 0, 0, 0);
    document = unknown_00721eb8(chat_gui_root_handle, chat_gui_find_object_arg);
    if (document != 0) {
        unknown_00721edc(document, 0);
        unknown_00721ec8(document);
    }
    unknown_00721eb4(chat_gui_root_handle, log_path, chat_listbox_gui_find_object_arg, 0x10000000, /* 0x51985b: mov eax,[0x69c69c] */ rect_zero, 0, 0, 0, 0, 0, 0);
}

#if 0
Original Ghidra decompilation (0x5196b0):

void rasterizer_editbox_log_dump(void)

{
  int iVar1;
  undefined4 *puVar2;
  wchar_t *_Dest;
  int _Radix;
  undefined4 uStack_d0;
  undefined4 uStack_cc;
  undefined4 uStack_c8;
  undefined4 uStack_c4;
  undefined *puStack_c0;
  undefined *puStack_bc;
  undefined *puStack_b8;
  undefined4 uStack_b4;
  undefined2 uStack_b0;
  undefined4 auStack_ae [11];
  undefined *puStack_80;
  undefined *puStack_7c;
  undefined *puStack_78;
  undefined4 uStack_74;
  undefined2 uStack_70;
  undefined4 auStack_6e [11];
  wchar_t local_40;
  undefined4 local_3e [15];

  if (DAT_00721ea4 == 0) {
    if (DAT_00721ea0 == (code *)0x0) {
      return;
    }
    DAT_00721ea4 = (*DAT_00721ea0)(DAT_007461c4,DAT_0071d174,DAT_00721ea8,0,0,0,0);
    if (DAT_00721ea4 == 0) {
      return;
    }
  }
  local_40 = L'\0';
  puVar2 = local_3e;
  for (iVar1 = 0xf; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  uStack_70 = 0;
  puStack_80 = &DAT_006f0063;
  puStack_7c = &DAT_0074006e;
  puStack_78 = &DAT_006e0065;
  uStack_74 = 0x2f0074;
  puVar2 = auStack_6e;
  for (iVar1 = 0xb; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  uStack_b0 = 0;
  puStack_c0 = &DAT_006f0063;
  puStack_bc = &DAT_0074006e;
  puStack_b8 = &DAT_006e0065;
  uStack_b4 = 0x2f0074;
  puVar2 = auStack_ae;
  for (iVar1 = 0xb; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  _Radix = 10;
  _Dest = &local_40;
  uStack_cc = 0;
  uStack_d0 = 0;
  uStack_c4 = DAT_007c04a4;
  uStack_c8 = DAT_007c04a0;
  iVar1 = rasterizer_round_up_resolution_height();
  __itow(iVar1,_Dest,_Radix);
  _wcscat((wchar_t *)&puStack_c0,&local_40);
  _wcscat((wchar_t *)&puStack_c0,L"editbox.ksml");
  _wcscat((wchar_t *)&puStack_80,&local_40);
  _wcscat((wchar_t *)&puStack_80,L"log.ksml");
  (*DAT_00721eb4)(DAT_00721ea4,&puStack_c0,PTR_PTR_0069c698,0x10000000,&uStack_d0,0,0,0,0,0,0);
  iVar1 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c698);
  if (iVar1 != 0) {
    (*DAT_00721edc)(iVar1,0);
    (*DAT_00721ec8)(iVar1);
  }
  (*DAT_00721eb4)(DAT_00721ea4,&puStack_80,PTR_PTR_0069c69c,0x10000000,&uStack_d0,0,0,0,0,0,0);
  return;
}
#endif
