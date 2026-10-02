// input_mouse_state_process  (Ghidra: FUN_00491bc0)
// address 0x491bc0, size 134 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: types/input.h names this exactly: "input_mouse_state_process 0x491bc0 builds it from
// a di_mouse_state2 in ECX into the argument, the poll passes 0x006b180c" -- confirming dest is
// the one stack parameter and raw is in ECX. objdump of 0x491bc0..0x491c45 was used to resolve a
// discrepancy the Ghidra pseudo-C hid: mouse_state::button_pressed's own doc comment says "1 on
// the frame the button went down", but the actual instructions (`shr cl,7` then `test cl,cl;
// jne <clear-pressed>`, falling through to `cmp cl,[frames]; je <clear-pressed>` only when NOT
// currently pressed) set button_pressed to 1 only on a RELEASE transition (held last frame, not
// held now) -- never on a press. This is a phase-4 correction to that struct comment, not
// something this file can fix; kept exactly as the binary does it.
// register convention: dest on the stack; raw (di_mouse_state2*) in ECX (in_ECX)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t mouse_wheel_granularity;              // 0x006b1808
extern int16_t mouse_button_map[k_input_mouse_button_count]; // 0x0068e534

// blam-cc: dest on the stack, raw in ECX
// Converts one raw DirectInput mouse sample into the engine's mouse_state: x copied, y negated,
// wheel divided by mouse_wheel_granularity and negated (left unchanged if the granularity is
// still 0), and each physical button's hold-frame count and (release-transition) pressed flag
// updated through the left/right swap map.
void input_mouse_state_process(mouse_state *dest, di_mouse_state2 *raw)
{
    int32_t i;
    int32_t mapped_slot;
    uint8_t pressed_now;
    uint8_t old_frames;

    dest->x = raw->x;
    dest->y = -raw->y;
    if (mouse_wheel_granularity != 0) {
        dest->wheel = -(raw->z / mouse_wheel_granularity);
    }

    for (i = 0; i < k_input_mouse_button_count; i++) {
        mapped_slot = mouse_button_map[i];
        pressed_now = (raw->buttons[i] & 0x80) != 0;
        old_frames = dest->button_frames[mapped_slot];

        dest->button_pressed[mapped_slot] = (pressed_now == 0 && old_frames != 0) ? 1 : 0;

        if (pressed_now) {
            dest->button_frames[mapped_slot] = (old_frames < 0xff) ? (uint8_t)(old_frames + 1) : 0xff;
        } else {
            dest->button_frames[mapped_slot] = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x491bc0):

void FUN_00491bc0(undefined4 *param_1)

{
  char cVar1;
  int iVar2;
  undefined4 *in_ECX;
  uint uVar3;
  int iVar4;
  char *pcVar5;
  short *psVar6;

  *param_1 = *in_ECX;
  param_1[1] = -in_ECX[1];
  if (DAT_006b1808 != 0) {
    param_1[2] = -((int)in_ECX[2] / DAT_006b1808);
  }
  psVar6 = &DAT_0068e534;
  pcVar5 = (char *)(in_ECX + 3);
  iVar4 = 8;
  do {
    cVar1 = *pcVar5;
    if ((cVar1 < '\0') || (iVar2 = (int)*psVar6 + (int)param_1, *(char *)(iVar2 + 0xc) == '\0')) {
      iVar2 = (int)*psVar6 + (int)param_1;
      *(undefined1 *)(iVar2 + 0x14) = 0;
    }
    else {
      *(undefined1 *)(iVar2 + 0x14) = 1;
    }
    if (cVar1 < '\0') {
      uVar3 = *(byte *)(iVar2 + 0xc) + 1;
      if (0xff < uVar3) {
        uVar3 = 0xff;
      }
    }
    else {
      uVar3 = 0;
    }
    pcVar5 = pcVar5 + 1;
    psVar6 = psVar6 + 1;
    iVar4 = iVar4 + -1;
    *(char *)(iVar2 + 0xc) = (char)uVar3;
  } while (iVar4 != 0);
  return;
}

Disassembly (objdump -d, 0x491bc0..0x491c45) resolving the button_pressed direction:

  491bf7: mov    cl,[esi]        ; cl = raw.buttons[i]
  491bf9: mov    dx,[edi]        ; dx = mouse_button_map[i]
  491bfc: shr    cl,7            ; cl = pressed_now (0/1)
  491bff: test   cl,cl
  491c01: jne    0x491c13        ; pressed now -> button_pressed = 0
  491c03: movswl dx,eax
  491c06: add    ebp,eax         ; eax = dest + mapped_slot
  491c08: cmp    cl,[eax+0xc]    ; cl(0) vs old button_frames[mapped]
  491c0b: je     0x491c13        ; old frames == 0 too -> button_pressed = 0
  491c0d: movb   [eax+0x14],1    ; else (was held, now released) -> button_pressed = 1
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
