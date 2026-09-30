// interface_local_player_state_reset  (Ghidra: FUN_00494390, renamed per types/interface.h)
// address 0x494390, size 145 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/interface.h first_person_weapon_interface struct comment names this address
// interface_local_player_state_reset ("clears it... zeroes 0x7a8 dwords"); its own field
// comments confirm unit_index (+0x4), frame_sound_impulse (+0x1e98) and frame_sound_state (+0x1e9c) as the
// three post-zero sentinel writes; hud_state_reset is 0x4a98d0 per this header's own HUD-globals
// comment; src/effects/particle_system_new_on_marker.c's precedent default_color_block
// (0x006851fc, ColorARGB); src/game/hud_draw_world_relative_text.c's shared hud_text_draw_*
// globals.
// reconciled: 0x006851fc is a pointer to the opaque-white ColorARGB (0x00655138); one name global_white_argb: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_text.h"
#include <string.h>

extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern Globals *global_globals;                    // 0x00746fa0
extern const ColorARGB *global_white_argb;               // 0x006851fc, opaque white

extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734
extern int16_t hud_text_draw_column;          // 0x006e4736
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730
extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c
extern float hud_text_draw_color_a;           // 0x006e4738, alpha (ColorARGB order); float bits, stored with mov
extern float hud_text_draw_color_r;           // 0x006e473c
extern float hud_text_draw_color_g;           // 0x006e4740
extern float hud_text_draw_color_b;           // 0x006e4744

extern void hud_state_reset(void); // 0x4a98d0


// Resets the HUD runtime state and text language, then zeroes local player 0's entire
// first_person_weapon_interface, re-seeding unit_index/frame_sound_impulse/frame_sound_state to -1 and
// priming the shared HUD text-draw color to the module's default (opaque white) with the
// globals interface_bitmaps' font_terminal tag id.
void interface_local_player_state_reset(void)
{
    first_person_weapon_interface *fp;
    GlobalsInterfaceBitmaps *interface_bitmaps;

    hud_state_reset();
    text_language_initialize_from_string_list();

    fp = &first_person_weapon_interfaces[0];
    memset(fp, 0, sizeof(*fp));
    fp->unit_index = (datum_index)0xffffffff;
    fp->frame_sound_impulse = -1;
    fp->frame_sound_state = -1;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    hud_text_draw_font_tag_id = *(int32_t *)&interface_bitmaps->font_terminal.tag_id;
    hud_text_draw_color_a = global_white_argb->alpha;
    hud_text_draw_color_r = global_white_argb->red;
    hud_text_draw_color_g = global_white_argb->green;
    hud_text_draw_color_b = global_white_argb->blue;
    hud_text_draw_color_or_flags = 0xffff;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;
}

#if 0
Original Ghidra decompilation (0x494390):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00494390(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;

  FUN_004a98d0();
  text_language_initialize_from_string_list();
  puVar1 = DAT_006b2d98;
  puVar4 = DAT_006b2d98;
  for (iVar3 = 0x7a8; iVar2 = DAT_00746fa0, iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  puVar1[1] = 0xffffffff;
  puVar1[0x7a6] = 0xffffffff;
  *(undefined2 *)(puVar1 + 0x7a7) = 0xffff;
  if (*(int *)(iVar2 + 0x140) == 0) {
    iVar3 = 0;
  }
  else {
    iVar3 = *(int *)(iVar2 + 0x144);
  }
  DAT_006e472c = *(undefined4 *)(iVar3 + 0x1c);
  DAT_006e4738 = *(undefined4 *)PTR_DAT_006851fc;
  DAT_006e473c = *(undefined4 *)(PTR_DAT_006851fc + 4);
  DAT_006e4740 = *(undefined4 *)(PTR_DAT_006851fc + 8);
  DAT_006e4744 = *(undefined4 *)(PTR_DAT_006851fc + 0xc);
  DAT_006e4734._0_2_ = 0xffff;
  DAT_006e4734._2_2_ = 0;
  _DAT_006e4730 = 0;
  return;
}
#endif
