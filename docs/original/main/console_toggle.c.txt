// console_toggle  (Ghidra: console_toggle, already named)
// address 0x4c6530, size 64 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/main.h console_globals; types/interface.h terminal_console (console_open takes
// EDI -> &console_globals_data.terminal, 0x006b7024). Disassembly (objdump -d -M intel, bin/halo.exe)
// shows the call to input_keyboard_set_capture_mode (0x48b650, src/input/input_keyboard_set_capture_mode.c)
// is a tail jump with AL hard-coded to 1 just before it (`mov al,0x1` at 0x4c6567), not the
// console_open result as Ghidra's decompile might suggest by proximity -- capture mode is turned
// on unconditionally whenever this path is taken, independent of whether console_open succeeded.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern console_globals console_globals_data; // 0x006b7020
extern uint8_t virtual_keyboard; // 0x007193a8, UNSURE: only tested here; blocks opening
                                           // the console while set (movie playback? name is a guess)

extern void console_deactivate(void); // this module, 0x4c64b0
extern uint8_t console_open(terminal_console *console); // 0x496510, blam-cc: EDI -> console
extern void input_keyboard_set_capture_mode(uint8_t enable_capture); // 0x48b650, blam-cc: AL -> enable_capture

// Closes the console if it is open; otherwise opens it (when enabled and not blocked by
// virtual_keyboard), clearing the input line first, and always turns on keyboard
// capture mode afterward.
void console_toggle(void)
{
    if (console_globals_data.active != 0) {
        console_deactivate();
        return;
    }
    if (console_globals_data.enabled != 0 && virtual_keyboard == 0) {
        console_globals_data.terminal.input[0] = 0; // 0x006b70d8
        console_globals_data.active = console_open(&console_globals_data.terminal); // EDI -> &terminal
        input_keyboard_set_capture_mode(1); // AL = 1 unconditionally (tail call in the binary)
    }
}

#if 0
Original Ghidra decompilation (0x4c6530):

void __cdecl console_toggle(void)

{
  if (DAT_006b7020 != '\0') {
    console_deactivate();
    return;
  }
  if ((DAT_006b7021 != '\0') && (DAT_007193a8 == '\0')) {
    DAT_006b70d8 = 0;
    DAT_006b7020 = console_open();
    FUN_0048b650();
    return;
  }
  return;
}

Disassembly (0x4c6530..0x4c656f):

004c6530:  mov    al,ds:0x6b7020
004c6535:  test   al,al
004c6537:  je     0x4c653e
004c6539:  jmp    0x4c64b0              ; console_deactivate(); return;
004c653e:  mov    al,ds:0x6b7021
004c6543:  test   al,al
004c6545:  je     0x4c656f
004c6547:  mov    al,ds:0x7193a8
004c654c:  test   al,al
004c654e:  jne    0x4c656f
004c6550:  push   edi
004c6551:  mov    edi,0x6b7024         ; &console_globals_data.terminal
004c6556:  mov    BYTE PTR ds:0x6b70d8,0x0   ; console_globals_data.terminal.input[0] = 0
004c655d:  call   0x496510              ; console_open(EDI = &terminal)
004c6562:  mov    ds:0x6b7020,al        ; console_globals_data.active = result
004c6567:  mov    al,0x1
004c6569:  pop    edi
004c656a:  jmp    0x48b650              ; tail call: input_keyboard_set_capture_mode(AL = 1)
004c656f:  ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
