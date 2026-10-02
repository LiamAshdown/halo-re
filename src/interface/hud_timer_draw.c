// hud_timer_draw  (Ghidra: chimera__fix_counters_timer_begin, renamed in the phase-4 review;
// the Chimera signature fix_counters_timer_begin_sig matches at the entry)
// address 0x4add10, size 656 bytes; real extent 0x4add10..0x4adf9f (656 bytes; Ghidra stopped at the anchor
// jump table at 0x4ade03 and reported 56). The five entry table at 0x4adfa0 maps anchor 0 and
// 2 to 0x4ade22, 1 and 3 to 0x4ade0a, 4 to 0x4ade1a.
// name confidence: 0.7 (chosen)   rewrite confidence: 0.8
// evidence: rewritten from objdump in the phase-4 review. The first rewrite treated the jump
// table as a table of handlers and wrote a second file, hud_chat_to_network.c, for 0x4add40,
// which is an instruction inside this function (an OpenSauce CE address that drifted); that
// file is deleted. The function draws the script countdown timer as three two-digit numbers
// (minutes, seconds, hundredths) with hud_draw_number (0x4ac0b0) at scale 2.0, spaced by 2.5
// times twice the HUDNumber screen_digit_width (Globals interface_bitmaps
// hud_digits_definition, tag +0x11), moving right for the left anchors and left for the right
// anchors. Colors: HUDGlobals not_much_time_left (+0x360) while ticks are left, time_out_flash
// (+0x380) once they ran out; flashing starts at timer_warning_ticks (the start time is rebased
// once so the flash starts there) and stays on after time ran out.
// Behaviour kept from the binary: the number placement is only partly initialized (scaling
// flags and the pad at +0x0e..+0x23 are stack garbage); hud_draw_number receives the word at
// 0x007c3108 as its unused first argument.
// register convention: none, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances;           // 0x0087bc14
extern Globals *global_globals;               // 0x00746fa0
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_messaging_globals *hud_messaging;  // 0x006b3a40
extern game_time_globals *game_time;          // 0x006f1d6c
extern int16_t current_local_player_index; // 0x007c3108, only passed on as the unused argument

extern int32_t __ftol(double x); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation
extern uint32_t hud_timer_get_ticks(void); // 0x4adcc0
extern void hud_draw_number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value,
                            int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale); // 0x4ac0b0

// Draws the HUD countdown timer when a script armed it.
void hud_timer_draw(void)
{
    uint16_t anchor[0x12]; // anchor word followed by 0x22 zero bytes, passed by address
    hud_number_placement placement;
    GlobalsInterfaceBitmaps *interface_bitmaps;
    datum_index digits_tag;
    int32_t now;
    int32_t digit_step;
    int16_t ticks;
    int32_t clock;
    int32_t minutes;
    int32_t seconds;
    int32_t sub_second;
    uint32_t flash;
    double spacing;
    int16_t i;

    if (hud_messaging->timer_active == 0) {
        return;
    }
    now = game_time->game_time;
    for (i = 1; i < 0x12; i++) {
        anchor[i] = 0;
    }
    anchor[0] = (uint16_t)hud_messaging->timer_anchor;
    flash = 0;
    digit_step = 0;
    ticks = (int16_t)hud_timer_get_ticks();
    placement.maximum_number_of_digits = 2;
    placement.number_of_fractional_digits = 4;
    placement.flags = 1; // show leading zeros
    placement.width_scale = 1.0f;
    placement.height_scale = 1.0f;
    placement.anchor_offset = hud_messaging->timer_offset;

    interface_bitmaps = global_globals->interface_bitmaps.count != 0
        ? (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer
        : (GlobalsInterfaceBitmaps *)0;
    digits_tag = *(datum_index *)&interface_bitmaps->hud_digits_definition.tag_id;
    if (digits_tag != (datum_index)-1) {
        HUDNumber *digits = (HUDNumber *)tag_instances[digits_tag & 0xffff].data;
        digit_step = __ftol((double)((float)(int32_t)digits->screen_digit_width + (float)(int32_t)digits->screen_digit_width));
    }
    switch (hud_messaging->timer_anchor) {
    case 1: // top right
    case 3: // bottom right
        placement.anchor_offset.x = (int16_t)(placement.anchor_offset.x + digit_step * 5);
        digit_step = -digit_step;
        break;
    case 4: // center
        placement.anchor_offset.x = (int16_t)(placement.anchor_offset.x + digit_step * -3);
        break;
    }

    if (ticks > 0) {
        placement.flash = *(hud_flash_parameters *)&hud_globals_tag_data->not_much_time_left_default_color;
        placement.disabled_color = hud_globals_tag_data->not_much_time_left_disabled_color;
        if (ticks <= hud_messaging->timer_warning_ticks) {
            flash = 1;
            if ((int16_t)hud_messaging->timer_ticks > hud_messaging->timer_warning_ticks) {
                hud_messaging->timer_ticks = (uint16_t)hud_messaging->timer_warning_ticks;
                hud_messaging->timer_start_time = hud_messaging->timer_warning_ticks - ticks + now;
            }
        }
    } else {
        placement.flash = *(hud_flash_parameters *)&hud_globals_tag_data->time_out_flash_default_color;
        placement.disabled_color = hud_globals_tag_data->time_out_flash_disabled_color;
        hud_messaging->timer_ticks = 0xffff;
        flash = 1;
        if (hud_messaging->timer_start_time == -1) {
            hud_messaging->timer_start_time = game_time->game_time;
        }
    }

    clock = ticks > 0 ? ticks : 0;
    sub_second = (int16_t)clock % 30;
    minutes = (int16_t)((int16_t)clock / 30) / 60;
    seconds = (int16_t)((int16_t)clock / 30) % 60;
    hud_draw_number((void *)(uint32_t)current_local_player_index, anchor, &placement, (int16_t)minutes, -1, flash,
                    hud_messaging->timer_start_time, 2.0f);

    spacing = (double)(int16_t)digit_step * 2.5;
    placement.anchor_offset.x = (int16_t)__ftol((double)placement.anchor_offset.x + spacing);
    hud_draw_number((void *)(uint32_t)current_local_player_index, anchor, &placement, (int16_t)seconds, -1, flash,
                    hud_messaging->timer_start_time, 2.0f);

    placement.anchor_offset.x = (int16_t)__ftol((double)placement.anchor_offset.x + spacing);
    hud_draw_number((void *)(uint32_t)current_local_player_index, anchor, &placement,
                    (int16_t)(sub_second * 100 / 30), -1, flash, hud_messaging->timer_start_time, 2.0f);
}

#if 0
Original Ghidra decompilation (0x4add10):

void chimera__fix_counters_timer_begin(void)

{
  short sVar1;
  int iVar2;

  if (*(char *)(DAT_006b3a40 + 0x487) != '\0') {
    sVar1 = *(short *)(DAT_006b3a40 + 0x484);
    hud_counter_get_value();
    if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
      iVar2 = 0;
    }
    else {
      iVar2 = *(int *)(DAT_00746fa0 + 0x144);
    }
    if (*(int *)(iVar2 + 0xbc) != -1) {
      FUN_006391b4();
    }
                    /* WARNING: Could not recover jumptable at 0x004ade03. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*(code *)(&PTR_LAB_004adfa0)[sVar1])();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
