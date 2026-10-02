// shell_pump_windows_messages  (Ghidra: shell_pump_windows_messages, already named)
// address 0x541a20, size 146 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: PeekMessageA/TranslateMessage/DispatchMessageA loop; keystone_translate_accelerator
// (0x00721eb0) matches its typedef in shell.h exactly (root, hwnd, unknown, message).
// register convention: __cdecl, no arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *chat_gui_root_handle;                    // 0x00721ea4
extern void *keystone_module;                  // 0x00721e9c
extern void *shell_window;                     // 0x007461c4
extern keystone_translate_accelerator_fn keystone_translate_accelerator; // 0x00721eb0


// Drains the Win32 message queue each frame, routing messages through the Keystone UI
// accelerator translator when the UI library is loaded and otherwise through the normal
// Translate/DispatchMessage path.
void shell_pump_windows_messages(void)
{
    uint8_t message[28]; // tagMSG
    int32_t has_message;
    int32_t handled;

    has_message = PeekMessageA((LPMSG)message, 0, 0, 0, 1);
    while (has_message != 0) {
        if (chat_gui_root_handle == 0 || keystone_module == 0) {
            TranslateMessage((const MSG *)message);
            DispatchMessageA((const MSG *)message);
        } else {
            handled = (int32_t)keystone_translate_accelerator(chat_gui_root_handle, shell_window, 0, message);
            if (handled == 0) {
                TranslateMessage((const MSG *)message);
                DispatchMessageA((const MSG *)message);
            }
        }
        has_message = PeekMessageA((LPMSG)message, 0, 0, 0, 1);
    }
}

#if 0
Original Ghidra decompilation (0x541a20):


void __cdecl shell_pump_windows_messages(void)

{
  int iVar1;
  tagMSG local_1c;
  
  iVar1 = PeekMessageA(&local_1c,(HWND)0x0,0,0,1);
  do {
    if (iVar1 == 0) {
      return;
    }
    if ((DAT_00721ea4 == 0) || (DAT_00721e9c == 0)) {
      TranslateMessage(&local_1c);
LAB_00541a99:
      DispatchMessageA(&local_1c);
    }
    else {
      iVar1 = (*DAT_00721eb0)(DAT_00721ea4,DAT_007461c4,0,&local_1c);
      if (iVar1 == 0) {
        TranslateMessage(&local_1c);
        goto LAB_00541a99;
      }
    }
    iVar1 = PeekMessageA(&local_1c,(HWND)0x0,0,0,1);
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
