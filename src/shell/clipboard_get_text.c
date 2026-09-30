// clipboard_get_text  (Ghidra: clipboard_get_text, already named)
// address 0x541ac0, size 109 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: IsClipboardFormatAvailable/OpenClipboard/GetClipboardData/GlobalLock text retrieval.
// register convention: __cdecl (buffer, capacity) -- Ghidra already recovered both parameters.
// UNSURE: OpenClipboard is only called when IsClipboardFormatAvailable succeeds, but
// GetClipboardData is then called unconditionally regardless of whether the clipboard was
// actually opened; preserved exactly as decompiled (likely a genuine bug in the original).

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


extern void *shell_window; // 0x007461c4

// Copies the current CF_TEXT clipboard contents into the caller-supplied buffer (capacity
// bytes), returning 1 on success or 0 if no text is available.
uint32_t clipboard_get_text(char *buffer, uint32_t capacity)
{
    void *clipboard_handle;
    char *locked_text;

    if (IsClipboardFormatAvailable(1 /* CF_TEXT */) != 0) {
        OpenClipboard(shell_window);
    }

    clipboard_handle = GetClipboardData(1 /* CF_TEXT */);
    if (clipboard_handle == 0) {
        GetLastError();
    } else {
        locked_text = (char *)GlobalLock(clipboard_handle);
        if (locked_text != 0) {
            strncpy(buffer, locked_text, capacity);
            GlobalUnlock(clipboard_handle);
            CloseClipboard();
            return 1;
        }
    }

    CloseClipboard();
    return 0;
}

#if 0
Original Ghidra decompilation (0x541ac0):


undefined4 clipboard_get_text(char *param_1,size_t param_2)

{
  BOOL BVar1;
  HANDLE hMem;
  char *_Source;
  
  BVar1 = IsClipboardFormatAvailable(1);
  if (BVar1 != 0) {
    OpenClipboard(DAT_007461c4);
  }
  hMem = GetClipboardData(1);
  if (hMem == (HANDLE)0x0) {
    GetLastError();
  }
  else {
    _Source = GlobalLock(hMem);
    if (_Source != (char *)0x0) {
      _strncpy(param_1,_Source,param_2);
      GlobalUnlock(hMem);
      CloseClipboard();
      return 1;
    }
  }
  CloseClipboard();
  return 0;
}
#endif
