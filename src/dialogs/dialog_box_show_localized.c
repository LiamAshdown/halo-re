// dialog_box_show_localized  (Ghidra: dialog_box_show_localized, already named)
// address 0x57e1f0, size 165 bytes
// name confidence: 0.7   rewrite confidence: 0.9 (phase-4 review: matched instruction by instruction to objdump)
// evidence: out/phase2/results/dialogs_00.json "Calls FindResourceExA with the current language
//   id (DAT_0069ff20), then LoadResource/LockResource/DialogBoxIndirectParamA; on failure
//   retries with language 0x409 (English US) if different, and finally falls back to a plain
//   DialogBoxParamA(param_1) lookup by name/ID." Resource type constant &DAT_00000005 is
//   MAKEINTRESOURCE(5) = RT_DIALOG. types/dialogs.h k_dialog_resource_type_dialog /
//   k_dialog_language_english / k_dialog_box_failed match these three literals exactly.
// evidence for the return value: out/phase4/dialogs_types_notes.md "Ghidra's void (LPCSTR, HWND)
//   signature drops the return and both register arguments. The existing
//   src/shell/shell_display_fatal_error_dialog.c extern declaration is consistent with this" --
//   that caller (0x57ee2a) compares the result against 2 (IDCANCEL), so this rewrite keeps the
//   INT_PTR DialogBox result instead of Ghidra's `void`.
// register convention (objdump, out/phase4/dialogs_types_notes.md): module (HINSTANCE) in ESI,
//   the DLGPROC in EBX; the stack holds (template name or MAKEINTRESOURCE id, parent HWND), and
//   the caller pops them (add esp,8 at 0x57ee2f). Only call site: 0x57ee2a in
//   shell_display_fatal_error_dialog, with ESI = strings_module (0x00722bb8), EBX = 0x57e5a0
//   (the fatal error dialog proc), template id 0x66, parent = the window.
// blam-cc: dialog_proc in EBX, module in ESI; template_name and parent_window are the two
//   stack arguments, in their original order.

#include "tags.h"
#include "dialogs.h"

extern void *__stdcall FindResourceExA(void *module, const char *type, const char *name,
                                        uint16_t language); // import 0x63a30c
extern void *__stdcall LoadResource(void *module, void *resource_info); // import 0x63a18c
extern void *__stdcall LockResource(void *resource_data);               // import 0x63a190
extern int32_t __stdcall DialogBoxIndirectParamA(void *module, const void *dialog_template, void *parent_window,
                                                  dialog_window_proc_fn dialog_proc,
                                                  int32_t init_param); // import 0x63a388
extern int32_t __stdcall DialogBoxParamA(void *module, const char *template_name, void *parent_window,
                                          dialog_window_proc_fn dialog_proc,
                                          int32_t init_param); // import 0x63a38c

extern uint32_t shell_language_id; // 0x0069ff20, shell-owned; see types/dialogs.h

// Shows a modal dialog box built from `template_name` (a resource name, or a MAKEINTRESOURCE id
// such as the fatal-error dialog's 0x66). Tries the RT_DIALOG resource in the current UI
// language first, then in English (0x409) if that differs, and finally falls back to a plain
// DialogBoxParamA lookup of `template_name` by the module's default language. Returns the
// DialogBox result (an INT_PTR) of whichever attempt actually ran the dialog.
int32_t dialog_box_show_localized(dialog_window_proc_fn dialog_proc, void *module, const char *template_name,
                                   void *parent_window)
{
    void *resource_info;
    void *resource_data;
    const void *dialog_template;
    int32_t result;

    resource_info = FindResourceExA(module, (const char *)k_dialog_resource_type_dialog, template_name,
                                     (uint16_t)shell_language_id);
    if (resource_info != 0) {
        resource_data = LoadResource(module, resource_info);
        if (resource_data != 0) {
            dialog_template = LockResource(resource_data);
            if (dialog_template != 0) {
                result = DialogBoxIndirectParamA(module, dialog_template, parent_window, dialog_proc, 0);
                if (result != k_dialog_box_failed) {
                    return result;
                }
            }
        }
    }

    if (shell_language_id != k_dialog_language_english) {
        resource_info = FindResourceExA(module, (const char *)k_dialog_resource_type_dialog, template_name,
                                         k_dialog_language_english);
        if (resource_info != 0) {
            resource_data = LoadResource(module, resource_info);
            if (resource_data != 0) {
                dialog_template = LockResource(resource_data);
                if (dialog_template != 0) {
                    result = DialogBoxIndirectParamA(module, dialog_template, parent_window, dialog_proc, 0);
                    if (result != k_dialog_box_failed) {
                        return result;
                    }
                }
            }
        }
    }

    result = DialogBoxParamA(module, template_name, parent_window, dialog_proc, 0);
    return result;
}

#if 0
Original Ghidra decompilation (0x57e1f0):

void dialog_box_show_localized(LPCSTR param_1,HWND param_2)

{
  HRSRC pHVar1;
  HGLOBAL pvVar2;
  LPCDLGTEMPLATEA pDVar3;
  INT_PTR IVar4;
  DLGPROC unaff_EBX;
  HMODULE unaff_ESI;

  pHVar1 = FindResourceExA(unaff_ESI,&DAT_00000005,param_1,(WORD)DAT_0069ff20);
  if ((((pHVar1 != (HRSRC)0x0) && (pvVar2 = LoadResource(unaff_ESI,pHVar1), pvVar2 != (HGLOBAL)0x0))
      && (pDVar3 = LockResource(pvVar2), pDVar3 != (LPCDLGTEMPLATEA)0x0)) &&
     (IVar4 = DialogBoxIndirectParamA(unaff_ESI,pDVar3,param_2,unaff_EBX,0), IVar4 != -1)) {
    return;
  }
  if (((DAT_0069ff20 != 0x409) &&
      (pHVar1 = FindResourceExA(unaff_ESI,&DAT_00000005,param_1,0x409), pHVar1 != (HRSRC)0x0)) &&
     ((pvVar2 = LoadResource(unaff_ESI,pHVar1), pvVar2 != (HGLOBAL)0x0 &&
      ((pDVar3 = LockResource(pvVar2), pDVar3 != (LPCDLGTEMPLATEA)0x0 &&
       (IVar4 = DialogBoxIndirectParamA(unaff_ESI,pDVar3,param_2,unaff_EBX,0), IVar4 != -1)))))) {
    return;
  }
  DialogBoxParamA(unaff_ESI,param_1,param_2,unaff_EBX,0);
  return;
}
#endif
