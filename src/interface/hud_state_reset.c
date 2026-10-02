// hud_state_reset  (Ghidra: FUN_004a98d0; named by types/interface.h's HUD runtime state note,
// "hud_state_reset @0x4a98d0")
// address 0x4a98d0, size 178 bytes
// name confidence: 0.5 (from types/interface.h)   rewrite confidence: 0.55
// evidence: types/interface.h hud_globals_flags, hud_messaging_globals, hud_unit_meter_state
// (every reset field -- displayed_shield/health/extra, shield/health/extra_event_time,
// unknown_22, sound_datums -- matches exactly), hud_weapon_interface_state, hud_waypoint_state
// (all -1, matching the header's own "type 0xf / object_index -1 marks the slot free" note),
// hud_globals_tag_data; Globals::interface_bitmaps and GlobalsInterfaceBitmaps::hud_globals
// (tag_id at +0x6c once the struct is sized out) for the tag lookup, mirroring the
// global_globals idiom already established in src/interface/hud_text_draw_configure.c.
// UNSURE: DAT_00873d40, a second global also set to hud_globals_tag_data, is not documented by
// any module read in this pass.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Globals *global_globals; // 0x00746fa0
extern tag_instance *tag_instances; // 0x0087bc14

extern hud_globals_flags *hud_flags;                 // 0x00719420
extern hud_messaging_globals *hud_messaging;          // 0x006b3a40
extern hud_unit_meter_globals *hud_unit_meters;       // 0x0071942c
extern hud_weapon_interface_state *hud_weapon_state;  // 0x00719430
extern hud_waypoint_state *hud_waypoints;             // 0x006b3a44
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern HUDGlobals *hud_messaging_parameters; // 0x00873d40, UNSURE: mirrors hud_globals_tag_data

extern void motion_sensor_reset(void); // 0x4b3660

// Clears and reinitializes the per-round HUD runtime-state buffers to their default values
// (unit meters snap on the first update via -1.0 displayed values, weapon/waypoint slots start
// all free), looks up the current hud_globals tag, and resets the motion sensor.
void hud_state_reset(void)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    int32_t i;

    hud_flags->hud_enabled = 0;
    hud_flags->help_text_shown = 0;
    hud_flags->unknown_02[0] = 0;
    hud_flags->unknown_02[1] = 0;
    hud_flags->hud_enabled = 1;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    hud_globals_tag_data = (HUDGlobals *)(tag_instances[(*(int32_t *)&interface_bitmaps->hud_globals.tag_id) & 0xffff].data);
    hud_messaging_parameters = hud_globals_tag_data;

    memset(hud_messaging, 0, sizeof(*hud_messaging));
    memset(hud_unit_meters, 0, sizeof(*hud_unit_meters));

    hud_unit_meters->players[0].auxiliary_meter_timers[0] = -1;
    hud_unit_meters->players[0].displayed_health = -1.0f;
    hud_unit_meters->players[0].displayed_shield = -1.0f;
    hud_unit_meters->players[0].shield_drain_time = -1.0f;
    hud_unit_meters->players[0].health_flash_start_time = -1;
    hud_unit_meters->players[0].motion_sensor_flash_start_time = -1;
    hud_unit_meters->players[0].last_unit = (datum_index)-1;
    hud_unit_meters->players[0].sounds_playing = 0;
    for (i = 0; i < 12; i = i + 1) {
        hud_unit_meters->players[0].sound_handles[i] = -1;
    }

    for (i = 0; i < 0x1f; i = i + 1) {
        ((int32_t *)hud_weapon_state)[i] = -1; // all 0x1f dwords, including the flags
    }

    memset(hud_waypoints, 0xff, sizeof(*hud_waypoints));

    motion_sensor_reset();
}

#if 0
Original Ghidra decompilation (0x4a98d0):

void FUN_004a98d0(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;

  puVar3 = DAT_00719420;
  *DAT_00719420 = 0;
  *(undefined1 *)puVar3 = 1;
  puVar3 = DAT_0071942c;
  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)(DAT_00746fa0 + 0x144);
  }
  DAT_0071941c = *(undefined4 *)((*(uint *)(iVar1 + 0x6c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar2 = DAT_006b3a40;
  DAT_00873d40 = DAT_0071941c;
  for (iVar1 = 0x122; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  puVar2 = puVar3;
  for (iVar1 = 0x17; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)((int)puVar3 + 0x22) = 0xffff;
  puVar3[1] = 0xbf800000;
  *puVar3 = 0xbf800000;
  puVar3[2] = 0xbf800000;
  puVar3[5] = 0xffffffff;
  puVar3[6] = 0xffffffff;
  puVar3[7] = 0xffffffff;
  *(undefined2 *)(puVar3 + 9) = 0;
  puVar3 = puVar3 + 10;
  for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 1;
  }
  puVar3 = DAT_00719430;
  for (iVar1 = 0x1f; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 1;
  }
  puVar3 = DAT_006b3a44;
  for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 1;
  }
  motion_sensor_reset();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
