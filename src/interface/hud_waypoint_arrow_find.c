// hud_waypoint_arrow_find  (Ghidra: FUN_004af070, renamed in the phase-4 review)
// address 0x4af070, size 94 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4af070..0x4af0cd. HUDGlobals +0x160/+0x164 is the waypoint_arrows block
// (HUDGlobalsWaypointArrow, stride 0x68, name first); 0x628d8b is the CRT stricmp. Used for
// the navpoint names of the activate_nav_point_* scripts and custom waypoints (types/game.h
// custom_waypoint::icon "resolved by name through 0x4af070").
// register convention: EDI name; the result is in AX.
//   // blam-cc: name -> EDI

#include "crt.h"
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
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c


// blam-cc: name -> EDI
// Index of the HUD waypoint arrow called name, -1 when there is none or no HUD globals tag.
int16_t hud_waypoint_arrow_find(const char *name)
{
    int16_t i;

    if (hud_globals_tag_data == 0) {
        return -1;
    }
    for (i = 0; (int32_t)i < (int32_t)hud_globals_tag_data->waypoint_arrows.count; i++) {
        const HUDGlobalsWaypointArrow *arrow =
            (const HUDGlobalsWaypointArrow *)hud_globals_tag_data->waypoint_arrows.pointer + i;
        if (_stricmp(name, arrow->name.string) == 0) {
            return i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4af070):

short FUN_004af070(void)

{
  int iVar1;
  short sVar2;
  char *unaff_EDI;

  if (DAT_0071941c == 0) {
    return -1;
  }
  sVar2 = 0;
  if (0 < *(int *)(DAT_0071941c + 0x160)) {
    iVar1 = 0;
    do {
      iVar1 = __stricmp(unaff_EDI,(char *)(iVar1 * 0x68 + *(int *)(DAT_0071941c + 0x164)));
      if (iVar1 == 0) {
        return sVar2;
      }
      sVar2 = sVar2 + 1;
      iVar1 = (int)sVar2;
    } while (iVar1 < *(int *)(DAT_0071941c + 0x160));
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
