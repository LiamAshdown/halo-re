// virtual_keyboard_backspace  (Ghidra: virtual_keyboard_backspace, already named)
// address 0x4a96f0, size 88 bytes
// name confidence: 0.6 (existing Ghidra name)   rewrite confidence: 0.85
// evidence: types/interface.h virtual_keyboard_globals (destination, destination_end,
// maximum_length all match the DAT_ offsets exactly).
// Watch pointer arithmetic: destination/destination_end are typed uint16_t* in the header, but
// Ghidra's arithmetic here is byte-precise, so this rewrite works through uint8_t* locals and
// casts back only where the header's field type is assigned.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

// Deletes the wide character immediately before the caret in the virtual keyboard's edit
// buffer (shifting the remaining tail left by 2 bytes and shrinking destination_end), then
// always plays the UI sound effect.
void virtual_keyboard_backspace(void)
{
    uint8_t *destination = (uint8_t *)virtual_keyboard.destination;
    uint8_t *destination_end = (uint8_t *)virtual_keyboard.destination_end;

    if (destination < destination_end) {
        int32_t size = (int32_t)(uint16_t)virtual_keyboard.maximum_length -
                        (int32_t)destination_end + (int32_t)destination;
        if (size >= 0) {
            memmove(destination_end - 2, destination_end, size);
            *(int16_t *)(destination + ((virtual_keyboard.maximum_length & ~1) - 2)) = 0;
            virtual_keyboard.destination_end = (uint16_t *)(destination_end - 2);
        }
    }
    widget_play_sound_effect(1); // tail jump with EAX 1
}

#if 0
Original Ghidra decompilation (0x4a96f0):

void __cdecl virtual_keyboard_backspace(void)

{
  size_t _Size;

  if ((DAT_007193c0 < DAT_007193c4) &&
     (_Size = ((uint)DAT_007193b4 - (int)DAT_007193c4) + (int)DAT_007193c0, -1 < (int)_Size)) {
    _memmove((void *)((int)DAT_007193c4 + -2),DAT_007193c4,_Size);
    *(undefined2 *)((int)DAT_007193c0 + ((DAT_007193b4 & 0xfffffffe) - 2)) = 0;
    DAT_007193c4 = (void *)((int)DAT_007193c4 + -2);
  }
  widget_play_sound_effect();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
