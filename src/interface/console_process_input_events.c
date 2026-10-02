// console_process_input_events  (Ghidra: console_process_input_events, already named)
// address 0x496c80, size 178 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: straight win32 console API usage (GetNumberOfConsoleInputEvents then a
// ReadConsoleInputA loop), matching the given name exactly; disassembly at 0x496cdb..0x496d23
// (not visible in Ghidra's pseudo-C, which drops both calls' arguments) shows the two calls to
// input_record_windows_key_message are EAX=wVirtualKeyCode/ECX=0x100 (WM_KEYDOWN) and
// EAX=uChar/ECX=0x102 (WM_CHAR), reconstructed by tracking the stack slots Ghidra names
// local_20/local_1c/local_18 through to the two call sites.
// register convention: no register-passed arguments.
// win32 INPUT_RECORD/KEY_EVENT_RECORD come from types/interface.h (win32_input_record).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t console_win32_attached; // 0x006b2f18
extern void *console_input_handle;     // 0x006b2dcc, win32 console input handle (not otherwise named)

extern void input_record_windows_key_message(int32_t key_or_char, int32_t message); // 0x490d10, input module
// blam-cc: EAX -> key_or_char, ECX -> message (0x100 WM_KEYDOWN, 0x102 WM_CHAR, 0x104
// WM_SYSKEYDOWN, 0x106 WM_SYSCHAR; only the first two are ever produced here)

// Drains the attached win32 console's input queue and, for every key-down event, forwards it
// into the input system as a synthetic WM_KEYDOWN followed by a synthetic WM_CHAR.
void console_process_input_events(void)
{
    uint32_t event_count;
    uint32_t events_read;
    uint32_t i;
    win32_input_record record;

    if (console_win32_attached == 0) {
        return;
    }
    if (GetNumberOfConsoleInputEvents(console_input_handle, (LPDWORD)&event_count) == 0) {
        return;
    }
    if (event_count == 0) {
        return;
    }
    for (i = 0; i < event_count; i++) {
        if (ReadConsoleInputA(console_input_handle, (PINPUT_RECORD)&record, 1, (LPDWORD)&events_read) != 0 &&
            record.EventType == 1) {
            if (record.KeyEvent.bKeyDown != 0) {
                input_record_windows_key_message(record.KeyEvent.wVirtualKeyCode, 0x100);
                input_record_windows_key_message(record.KeyEvent.uChar, 0x102);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x496c80):

void console_process_input_events(void)

{
  BOOL BVar1;
  uint uVar2;
  uint local_2c;
  DWORD local_28 [2];
  DWORD local_20;
  DWORD local_1c;
  DWORD local_18;
  _INPUT_RECORD local_14;

  if ((((DAT_006b2f18 != '\0') &&
       (BVar1 = GetNumberOfConsoleInputEvents(DAT_006b2dcc,&local_2c), BVar1 != 0)) &&
      (local_2c != 0)) && (uVar2 = 0, local_2c != 0)) {
    do {
      BVar1 = ReadConsoleInputA(DAT_006b2dcc,&local_14,1,local_28);
      if ((BVar1 != 0) && (local_14.EventType == 1)) {
        local_20 = local_14.Event.MouseEvent.dwButtonState;
        local_1c = local_14.Event.MouseEvent.dwControlKeyState;
        local_18 = local_14.Event.KeyEvent.dwControlKeyState;
        if (local_14.Event.KeyEvent.bKeyDown != 0) {
          input_record_windows_key_message();
          input_record_windows_key_message();
        }
      }
      uVar2 = uVar2 + 1;
    } while (uVar2 < local_2c);
  }
  return;
}

Disassembly of the two undocumented calls (0x496cdb..0x496d23), reconstructing their register
arguments since Ghidra's decompile above drops them entirely:

  496cdb: movzx  eax,WORD PTR [esp+0x20]      ; record.EventType
  496ce0: dec    eax
  496ce1: jne    0x496d23                     ; skip unless EventType == 1
  496ce3: mov    edx,DWORD PTR [esp+0x28]     ; record+8  (wRepeatCount | wVirtualKeyCode<<16)
  496ce7: mov    eax,DWORD PTR [esp+0x24]     ; record+4  (bKeyDown)
  496ceb: test   eax,eax
  496ced: mov    ecx,DWORD PTR [esp+0x2c]     ; record+0xc (wVirtualScanCode | uChar<<16)
  496cf1: mov    DWORD PTR [esp+0x14],edx
  496cf5: mov    edx,DWORD PTR [esp+0x30]     ; record+0x10 (dwControlKeyState)
  496cf9: mov    DWORD PTR [esp+0x18],ecx
  496cfd: mov    DWORD PTR [esp+0x1c],edx
  496d01: je     0x496d23                     ; skip unless bKeyDown != 0
  496d03: mov    eax,DWORD PTR [esp+0x14]
  496d07: shr    eax,0x10                     ; eax = wVirtualKeyCode
  496d0a: mov    ecx,0x100                    ; WM_KEYDOWN
  496d0f: call   0x490d10                     ; input_record_windows_key_message(wVirtualKeyCode, 0x100)
  496d14: movzx  eax,WORD PTR [esp+0x1a]      ; eax = uChar
  496d19: mov    ecx,0x102                    ; WM_CHAR
  496d1e: call   0x490d10                     ; input_record_windows_key_message(uChar, 0x102)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
