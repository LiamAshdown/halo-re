// first_person_weapon_update_active_state  (Ghidra: FUN_00492430; named per
// symbols/review_queue.txt "0x492430 first_person_weapon_update_active_state")
// address 0x492430, size 123 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/interface_functions.md "Determines whether the local player's
// first-person weapon view should be active based on zoom/action state, toggles it, and
// refreshes weapon animation controls when active."; symbols/review_queue.txt entries for
// 0x445ac0 (camera_get_type_for_player); 0x472740 is local_player_get_zoom_level (src/game).
// register convention: no explicit arguments; reads current_local_player_index directly.
// Review pass (phase 4): 0x472740 reads player_control_globals + index * 0x40 + 0x34, which is
// local_players[index].desired_zoom_level (the 0x10 byte header plus field 0x24), not a weapon
// index; there is no conflict with types/game.h. The weapon model is attached only in the
// first person camera while the player is not zoomed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t current_local_player_index; // 0x007c3108 (src/effects precedent name)
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

extern int16_t camera_get_type_for_player(int16_t player_index); // 0x445ac0, module camera; blam-cc: player_index in CX (in_CX)
extern int32_t local_player_get_zoom_level(int16_t local_player_index); // 0x472740, src/game; blam-cc: CX -> local_player_index
extern void first_person_weapon_set_attached(int16_t local_player_index, uint8_t attached); // 0x493e50, this module
extern void first_person_weapon_update_animation_controls(int16_t local_player_index); // 0x493740, this module

// Each frame, decides whether the local player's first-person weapon model should be attached:
// only while its unit and current weapon both exist, the active camera is first-person, and the
// zoom/action lookup above reports no override in progress. Applies that decision through
// first_person_weapon_set_attached, then refreshes the animation controls if it ends up attached.
void first_person_weapon_update_active_state(void)
{
    first_person_weapon_interface *fp;
    int16_t camera_type;
    int16_t zoom_level;
    uint8_t attach;

    if (current_local_player_index == -1) {
        return;
    }

    fp = &first_person_weapon_interfaces[current_local_player_index];

    if (fp->unit_index == (datum_index)0xffffffff) {
        return;
    }
    if (fp->weapon_index == (datum_index)0xffffffff) {
        return;
    }

    camera_type = camera_get_type_for_player(current_local_player_index);
    if (camera_type == 0) {
        zoom_level = (int16_t)local_player_get_zoom_level(current_local_player_index);
        attach = 1;
        if (zoom_level != -1) {
            attach = 0;
        }
    } else {
        attach = 0;
    }

    first_person_weapon_set_attached(current_local_player_index, attach);

    if (fp->attached != 0) {
        first_person_weapon_update_animation_controls(current_local_player_index);
    }
}

#if 0
Original Ghidra decompilation (0x492430):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00492430(void)

{
  short sVar1;
  int iVar2;
  char *pcVar3;
  undefined4 local_4;

  if ((short)_DAT_007c3108 == -1) {
    return;
  }
  iVar2 = (short)_DAT_007c3108 * 0x1ea0;
  pcVar3 = (char *)(iVar2 + DAT_006b2d98);
  if (*(int *)(iVar2 + 4 + DAT_006b2d98) == -1) {
    return;
  }
  if (*(int *)(pcVar3 + 8) == -1) {
    return;
  }
  sVar1 = camera_get_type_for_player();
  if (sVar1 == 0) {
    sVar1 = FUN_00472740();
    local_4 = 1;
    if (sVar1 == -1) goto LAB_00492486;
  }
  local_4 = 0;
LAB_00492486:
  chimera__first_person_node_base_address(local_4);
  if (*pcVar3 == '\0') {
    return;
  }
  first_person_weapon_update_animation_controls();
  return;
}

FUN_00472740 (0x472740), inlined above as local_player_get_zoom_level:

undefined4 FUN_00472740(void)

{
  undefined4 uVar1;
  short in_CX;

  uVar1 = 0xffffffff;
  if (in_CX != -1) {
    uVar1 = CONCAT22((short)((uint)(in_CX * 0x40) >> 0x10),
                     *(undefined2 *)(in_CX * 0x40 + 0x34 + DAT_006b145c));
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
