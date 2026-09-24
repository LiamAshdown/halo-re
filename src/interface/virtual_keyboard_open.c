// virtual_keyboard_open  (Ghidra: virtual_keyboard_open, already named)
// address 0x4a89a0, size 362 bytes
// name confidence: 0.55 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.75
// evidence: types/interface.h virtual_keyboard_globals -- every DAT_ offset here lines up with
// a named field (active, strings_tag_data, caret, unknown_0a, destination, destination_end,
// maximum_length, selection_start, open_time, text (DAT_0071940e is text[31], its last wide
// char slot), validation_mode, large_ui_tag, small_ui_tag) plus field_kind/committed/opened
// packed at +0x14 (DAT_007193bc as a 4 byte span the Ghidra locals slice into two bytes and
// two shorts).
// UNSURE: the 0x100 byte buffer at 0x00712ccc that this function zeroes first is not part of
// virtual_keyboard_globals and was not identified in this pass; kept as an opaque byte array.
// UNSURE: the DirectInput-shaped vtable call (`object->vtable[0x28/4](object, 0x14, 0, &-1, 0)`)
// and the two 0x1b-dword-plus-one-byte buffer clears that follow it are transcribed as raw
// COM-style calls; the interface method and the two buffers were not identified.
// register convention: maximum_length and field_kind as the two recognized parameters
// (Ghidra's own param_1/param_2), destination buffer in ESI (unaff_ESI, unresolved register).
//   // blam-cc: maximum_length/field_kind -> recognized parameters, destination -> ESI

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>
#include <string.h>

extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE (per src/interface/widget_close_all.c)
extern uint8_t unknown_00712ccc[0x100];           // 0x00712ccc, UNSURE: not part of virtual_keyboard_globals
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern int32_t time_query_performance_counter_ms(void);                // 0x449210, UNSURE: appears to be a millisecond clock (see interface.h progress_screen_fade_end_time note)
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern int32_t FUN_00625b7a(const uint16_t *s);   // 0x625b7a, UNSURE: appears to be wcslen

extern void **directinput_keyboard_device; // 0x006b1800, UNSURE: DirectInput device COM pointer
extern uint8_t directinput_unknown_buffer_1[0x6d]; // 0x006b1620 (0x1b dwords + 1 byte cleared)
extern uint8_t directinput_unknown_buffer_2[0x6d]; // 0x006b168d (0x1b dwords + 1 byte cleared)


// blam-cc: maximum_length/field_kind -> recognized parameters, destination -> ESI
// Opens the on-screen virtual keyboard for a caller-supplied wide-string buffer, clamping
// maximum_length to 0x40 bytes, choosing the large or small UI prompt tag by length, and
// resetting the DirectInput keyboard device's buffered input if one is active. Returns 0
// without doing anything if the keyboard is already open or its strings tag failed to load.
uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind)
{
    if (virtual_keyboard.active != 0 || virtual_keyboard.strings_tag_data == 0) {
        return 0;
    }

    memset(unknown_00712ccc, 0, sizeof(unknown_00712ccc));

    virtual_keyboard.caret = 0;
    virtual_keyboard.unknown_0a = 0;
    virtual_keyboard.active = 1;
    virtual_keyboard.destination = destination;
    virtual_keyboard.destination_end = destination + FUN_00625b7a(destination);
    virtual_keyboard.maximum_length = (maximum_length > 0x3f) ? 0x40 : (int16_t)maximum_length;
    virtual_keyboard.selection_start = -1;
    virtual_keyboard.open_time = time_query_performance_counter_ms();
    virtual_keyboard.field_kind = field_kind;
    virtual_keyboard.unknown_01 = 0;
    virtual_keyboard.unknown_02 = 0;
    virtual_keyboard.unknown_03 = 0;
    virtual_keyboard.opened = 1;
    virtual_keyboard.validation_mode = 1; // dword store
    wcsncpy((wchar_t *)virtual_keyboard.text, (const wchar_t *)destination, 0x20);
    virtual_keyboard.text[31] = 0; // DAT_0071940e, null-terminate the last text slot
    virtual_keyboard.committed = 0;
    virtual_keyboard.large_ui_tag = tag_lookup(0x666f6e74 /* font */, "ui\\large_ui");
    virtual_keyboard.small_ui_tag = tag_lookup(0x666f6e74 /* font */, (maximum_length < 0x33) ? "ui\\large_ui" : "ui\\small_ui");
    widget_play_sound_effect(2);

    // UNSURE: unrelated flag bit raised alongside opening the keyboard; not part of this module.
    controls_input_capture_flags |= 4;

    if (directinput_keyboard_device != 0) {
        int32_t minus_one = -1;
        void **vtable = *(void ***)directinput_keyboard_device;
        ((directinput_set_property_fn)vtable[0x28 / 4])(directinput_keyboard_device, 0x14, 0, &minus_one, 0);
        memset(directinput_unknown_buffer_2, 0, sizeof(directinput_unknown_buffer_2));
        memset(directinput_unknown_buffer_1, 0, sizeof(directinput_unknown_buffer_1));
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a89a0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 virtual_keyboard_open(ushort param_1,undefined2 param_2)

{
  int iVar1;
  wchar_t *unaff_ESI;
  undefined4 *puVar2;
  char *pcVar3;
  undefined4 local_4;

  if ((DAT_007193a8 == '\0') && (DAT_007193ac != 0)) {
    puVar2 = &DAT_00712ccc;
    for (iVar1 = 0x40; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    _DAT_007193b0 = 0;
    _DAT_007193b2 = 0;
    DAT_007193a8 = 1;
    DAT_007193c0 = unaff_ESI;
    iVar1 = FUN_00625b7a();
    DAT_007193c4 = unaff_ESI + iVar1;
    DAT_007193b4 = param_1;
    if (0x3f < param_1) {
      DAT_007193b4 = 0x40;
    }
    _DAT_007193b6 = 0xffff;
    _DAT_007193c8 = FUN_00449210();
    DAT_007193bc._0_2_ = param_2;
    DAT_007193a9 = 0;
    DAT_007193aa = 0;
    DAT_007193ab = 0;
    DAT_007193bc._3_1_ = 1;
    DAT_00719410 = 1;
    _wcsncpy(&DAT_007193d0,unaff_ESI,0x20);
    _DAT_0071940e = 0;
    DAT_007193bc._2_1_ = 0;
    DAT_00719414 = tag_lookup("ui\\large_ui");
    if (param_1 < 0x33) {
      pcVar3 = "ui\\large_ui";
    }
    else {
      pcVar3 = "ui\\small_ui";
    }
    DAT_00719418 = tag_lookup(pcVar3);
    widget_play_sound_effect();
    DAT_00712542 = DAT_00712542 | 4;
    if (DAT_006b1800 != (int *)0x0) {
      local_4 = 0xffffffff;
      (**(code **)(*DAT_006b1800 + 0x28))(DAT_006b1800,0x14,0,&local_4,0);
      puVar2 = &DAT_006b168d;
      for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar2 = 0;
        puVar2 = puVar2 + 1;
      }
      *(undefined1 *)puVar2 = 0;
      puVar2 = (undefined4 *)&DAT_006b1620;
      for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar2 = 0;
        puVar2 = puVar2 + 1;
      }
      *(undefined1 *)puVar2 = 0;
    }
    return 1;
  }
  return 0;
}
#endif
