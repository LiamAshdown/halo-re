// game_engine_get_max_look_pitch  (Ghidra: FUN_00471f90; renamed, no established name)
// address 0x471f90, size 131 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Returns the maximum look-pitch angle for a unit, using
// a zoom-weapon-specific value when the unit is holding a scoped weapon, otherwise a fixed
// default"); types/game.h local_player_control (unit +0x00); types/units.h unit_data
// (current_weapon_index +0x2f2, weapons[4] +0x2f8 -- `puVar2[current_weapon_index + 0xbe]` is
// exactly `weapons[current_weapon_index]` once the 4-byte element stride is accounted for:
// 0xbe * 4 == 0x2f8); game.h tag_instances (0x0087bc14, stride 0x20, tag data at +0x14).
// register convention: local-player index in AX (Ghidra's `in_AX`).
//   // blam-cc: AX -> local_player_index
// UNSURE: the weapon tag data offset +0x1a0 (the default and per-zoom-level max pitch value) is
// not attested in any header this module owns; types/tags.h's own Weapon struct was not consulted
// for this batch, so it is kept as a raw offset.

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern data_array *object_data;                          // 0x008603b0
extern tag_instance *tag_instances;                          // 0x0087bc14

extern real weapon_clamp_zoom_fov(datum_index item_index, int16_t zoom_level, real base_fov); // 0x4c2e50, DX zoom, stack (item, fov)

// blam-cc: AX -> local_player_index
// Returns the maximum look-pitch angle for the unit local_player_index is driving: the weapon
// tag's own zoom-adjusted value (via weapon_clamp_zoom_fov) when it is holding a weapon with a valid
// zoom marker, its tag's plain default pitch value otherwise, or a fixed 1.2217305 rad (70
// degrees) fallback when there is no unit at all.
real game_engine_get_max_look_pitch(int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    real result = 1.2217305f;

    if (look->unit != k_datum_index_none) {
        void *base = *(void **)((uint8_t *)object_data->data +
            (uint32_t)(uint16_t)look->unit * object_data->size + 8);
        object *o = (object *)base;
        unit_data *u = (unit_data *)((uint8_t *)base + k_unit_data_offset);
        uint8_t *tag_data = (uint8_t *)tag_instances[(uint16_t)o->definition_tag].data;

        if (u->current_weapon_index != -1 && u->weapons[u->current_weapon_index] != k_datum_index_none) {
            result = weapon_clamp_zoom_fov(u->weapons[u->current_weapon_index], look->desired_zoom_level, // 0x471ffc: DX = look+0x24
                                           *(real *)(tag_data + 0x1a0));
        } else {
            result = *(real *)(tag_data + 0x1a0);
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x471f90), from tools/pack.py 0x471f90:

float10 FUN_00471f90(void)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  short in_AX;
  float10 fVar4;

  fVar4 = (float10)1.2217305;
  uVar1 = *(uint *)(in_AX * 0x40 + 0x10 + DAT_006b145c);
  if (uVar1 != 0xffffffff) {
    puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
    iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if ((*(short *)((int)puVar2 + 0x2f2) != -1) &&
       (puVar2[*(short *)((int)puVar2 + 0x2f2) + 0xbe] != 0xffffffff)) {
      fVar4 = (float10)FUN_004c2e50(puVar2[*(short *)((int)puVar2 + 0x2f2) + 0xbe],
                                    *(undefined4 *)(iVar3 + 0x1a0));
      return fVar4;
    }
    fVar4 = (float10)*(float *)(iVar3 + 0x1a0);
  }
  return fVar4;
}
#endif
