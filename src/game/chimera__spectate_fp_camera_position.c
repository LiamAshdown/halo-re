// chimera__spectate_fp_camera_position  (Ghidra: chimera__spectate_fp_camera_position, already
// named)
// address 0x472020, size 222 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED against objdump 0x472020..0x4720fd; +8 is the seat (+0x84) or unit (+0x1a8) camera block)
// evidence: out/phase4/game_functions.md ("Computes the first-person camera object/offset for a
// local player's controlled unit, preferring the currently held weapon's camera marker over the
// unit's own"); modules.json's own evidence for this address ("resolves a player's unit index,
// seat index and the pointer to the seat's aiming/camera data (seat block at +0x2e8 stride
// 0x11c"); types/objects.h object (definition_tag +0x000, parent_object +0x11c);
// hud_find_nearby_teammate_for_nameplate.c for the established unit_get_camera_position
// (ECX -> unit_index, EDI -> out) convention; src/devices/device_frontfacing.c for the
// established object_try_and_get signature; types/objects.h object_type_mask (_object_mask_
// vehicle == 2).
//
// CORRECTED (this rewrite) against objdump (--start-address=0x472020 --stop-address=0x472100
// bin/halo.exe): the auto-generated summary says "weapon's camera marker", but the disassembly
// resolves the unit's *parent object* (its vehicle, when seated) and that vehicle tag's per-seat
// data at +0x2e8 (stride 0x11c, per modules.json), not a weapon tag at all. Ghidra's own
// decompile also completely drops the unit_get_camera_position call's real output (a
// real_point3d written to output+0xc that this function never reads back itself, but that is
// part of what it returns to its caller).
// register convention: local-player index in AX; the output struct pointer is Ghidra's
// `unaff_ESI`.
//   // blam-cc: ESI -> out, AX -> local_player_index
// UNSURE: the output struct (unit, seat, marker_offset, camera_position) has no header of its
// own in this module; modeled with a local anonymous layout matching exactly what is written.
// UNSURE: unit+0x2f0 ("seat_index" here) coincides with the byte offset types/units.h notes as
// "Vehicle vehicle_flags 0x2f0" for the vehicle-side extension union at the same base offset;
// which field it actually is when read off a *biped* (as here) is not confirmed by this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"

// camera_basis_out now lives in types/game.h (folded there by the phase-4 review; this file
// and game_engine_update_local_player_look.c are its two users).

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern data_array *object_data;                          // 0x008603b0
extern tag_instance *tag_instances;                          // 0x0087bc14

extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80,
    // blam-cc: ECX -> unit_index, EDI -> out
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// blam-cc: ESI -> out, AX -> local_player_index
void chimera__spectate_fp_camera_position(camera_basis_out *out, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    datum_index unit = look->unit;

    out->marker_offset = 0;
    out->unit = unit;
    out->seat_index = -1;

    if (unit != k_datum_index_none) {
        object *u = (object *)(*(void **)((uint8_t *)object_data->data +
            (uint32_t)(uint16_t)unit * object_data->size + 8));

        unit_get_camera_position(unit, &out->position);

        if (u->parent_object != k_datum_index_none) {
            object *parent = object_try_and_get(u->parent_object, _object_mask_vehicle);

            if (parent != 0) {
                uint8_t *vehicle_tag_data = (uint8_t *)tag_instances[(uint16_t)parent->definition_tag].data;
                int16_t seat_index = *(int16_t *)((uint8_t *)u + 0x2f0); // UNSURE: see header
                uint8_t *seat_array = *(uint8_t **)(vehicle_tag_data + 0x2e8); // UNSURE: raw
                    // pointer field, likely a TagReflexive's own "pointer" sub-field
                uint8_t *seat = seat_array + (int32_t)seat_index * 0x11c;

                out->marker_offset = seat + 0x84;
                out->unit = u->parent_object;
                out->seat_index = seat_index;
                u = (object *)(*(void **)((uint8_t *)object_data->data +
                    (uint32_t)(uint16_t)u->parent_object * object_data->size + 8));
            }
        }
        if (out->seat_index == -1) {
            uint8_t *tag_data = (uint8_t *)tag_instances[(uint16_t)u->definition_tag].data;

            out->marker_offset = tag_data + 0x1a8;
        }
    }
}

#if 0
Original Ghidra decompilation (0x472020), from tools/pack.py 0x472020 -- see the header comment
for the parts (the unit_get_camera_position output, and the parent/vehicle vs. "weapon" mixup)
this differs from.

void chimera__spectate_fp_camera_position(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  short in_AX;
  uint *puVar4;
  uint *puVar5;
  uint *unaff_ESI;

  iVar2 = DAT_006b145c;
  unaff_ESI[2] = 0;
  uVar1 = *(uint *)(in_AX * 0x40 + 0x10 + iVar2);
  *unaff_ESI = uVar1;
  *(undefined2 *)(unaff_ESI + 1) = 0xffff;
  if (uVar1 != 0xffffffff) {
    puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
    unit_get_camera_position();
    iVar2 = DAT_0087bc14;
    uVar1 = puVar5[0x47];
    if ((uVar1 != 0xffffffff) &&
       (puVar4 = (uint *)object_try_and_get(2), iVar3 = DAT_008603b0, puVar4 != (uint *)0x0)) {
      unaff_ESI[2] = (short)puVar5[0xbc] * 0x11c +
                     *(int *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + iVar2) + 0x2e8) + 0x84;
      *unaff_ESI = uVar1;
      *(short *)(unaff_ESI + 1) = (short)puVar5[0xbc];
      puVar5 = *(uint **)(*(int *)(iVar3 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
    }
    if ((short)unaff_ESI[1] == -1) {
      unaff_ESI[2] = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + iVar2) + 0x1a8;
    }
  }
  return;
}
#endif
