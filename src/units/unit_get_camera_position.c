// unit_get_camera_position  (Ghidra: unit_get_camera_position, already named)
// address 0x568f80, size 516 bytes
// name confidence: 0.6 (already carries this name; matches functions.md's summary)
// rewrite confidence: 0.85
// FIXED against objdump 0x568f80..0x569183: every marker path copies marker +0x60..+0x68 (node_transform.position,
//   the world position); the draft copied +0x2c (the node-relative transform), which put a seated unit's camera
//   (the a10 cryotube) near the world origin offset instead of at the seat's camera marker. Both marker names are
//   confirmed: "head" (0x66bfa0) and the gunner's seat +0x24 marker on this unit.
// evidence: types/units.h biped_data.crouch_fraction (0x50c), biped_data.flags (0x4cc);
//   types/tags.h Biped.crouch_camera_velocity (0x4cc), .standing_camera_height /
//   .crouching_camera_height (0x400/0x404); Unit.seats (TagReflexive at 0x2e4, pointer +4 =
//   0x2e8), UnitSeat (0x11c stride, camera_marker_name TagString at +0x84); types/objects.h
//   object.position (0x5c), object.parent_object (0x11c), object.type (0xb4),
//   object.vitality_flags (0x106); types/units.h unit_data.vehicle_seat_index (0x2f0),
//   .gunner_unit_index (0x328).
// register convention: unit index in ECX, destination real_point3d* in EDI.
//   // blam-cc: in_ECX -> unit_index, unaff_EDI -> out
// UNSURE: the marker-name argument (ECX at the point of each object_get_node_local_transform
//   call) could not be recovered from the decompilation for either call site; the second one
//   (reached once the seat's own camera_marker_name has already been checked non-empty) most
//   plausibly uses that same UnitSeat.camera_marker_name, which is what this rewrite passes.
//   The first (the "has a gunner, not seated, not the simple biped-alive case" early exit) has
//   no comparably strong candidate and is left as a NULL placeholder -- flagged so this path is
//   not trusted until re-checked against the disassembly.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
                                                uint32_t maximum_markers); // 0x4f6080

void unit_get_camera_position(uint32_t unit_index, real_point3d *out) // blam-cc: in_ECX -> unit_index, unaff_EDI -> out
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    object_marker marker;

    if (unit_obj->parent_object == k_datum_index_none) {
        if (((unit_obj->vitality_flags & _object_health_frozen_bit) == 0) && (unit_obj->type == _object_type_biped)) {
            Biped *biped_tag = (Biped *)tag_instances[unit_obj->definition_tag & 0xffff].data;
            object_get_position(out, unit_index);
            biped_data *biped = (biped_data *)((uint8_t *)unit_obj + k_unit_object_size);
            float height = biped->crouch_fraction;
            if (((biped->flags & 1) == 0) && (0.0f < height) && (height < 1.0f)) {
                float rate = game_time->leftover_time * 29.999998f * biped_tag->crouch_camera_velocity;
                if (unit->base_animation_state == _unit_base_animation_state_crouch) {
                    height = height + rate;
                } else {
                    height = height - rate;
                }
            }
            out->z = height * biped_tag->crouching_camera_height + (1.0f - height) * biped_tag->standing_camera_height + out->z;
            return;
        }
        if (unit->gunner_unit_index == k_datum_index_none) {
            // 0x569073: no gunner -- the unit's own "head" marker
            object_get_node_local_transform(unit_index, (char *)"head", &marker, 1);
        } else {
            // 0x569083: this unit's own seat block, indexed by the GUNNER's seat index -- that seat's marker
            object *gunner = ((object_header *)object_data->data)[unit->gunner_unit_index & 0xffff].data;
            Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
            UnitSeat *seat = (UnitSeat *)unit_tag->seats.pointer +
                ((unit_data *)((uint8_t *)gunner + k_unit_data_offset))->vehicle_seat_index;
            object_get_node_local_transform(unit_index, seat->marker_name.string, &marker, 1);
        }
        *out = marker.node_transform.position; // 0x5690b3 / 0x569160: [marker + 0x60], the WORLD marker position
        return;
    } else {
        // 0x5690d7: seated in a parent -- start from the parent's position
        object *parent = ((object_header *)object_data->data)[unit_obj->parent_object & 0xffff].data;
        Unit *parent_tag;
        UnitSeat *seat;
        *out = parent->position;
        if ((_object_mask_unit & (1 << (parent->type & 0x1f))) == 0) {
            return;
        }
        if (unit->vehicle_seat_index == -1) {
            return;
        }
        parent_tag = (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
        seat = (UnitSeat *)parent_tag->seats.pointer + unit->vehicle_seat_index;
        if (parent->type == _object_type_vehicle && seat->camera_marker_name.string[0] == '\0') {
            return;   // only a vehicle parent checks for an empty name; a biped parent is queried regardless
        }
        // 0x569147: the PARENT's marker named by the seat's camera_marker_name
        object_get_node_local_transform(unit_obj->parent_object, seat->camera_marker_name.string, &marker, 1);
        *out = marker.node_transform.position; // 0x5690b3 / 0x569160: [marker + 0x60], the WORLD marker position
        return;
    }
}

#if 0
Original Ghidra decompilation (0x568f80):

void unit_get_camera_position(void)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  uint *puVar4;
  float fVar5;
  uint in_ECX;
  uint *unaff_EDI;
  uint local_c;
  uint local_8;
  uint local_4;

  iVar3 = DAT_0087bc14;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (puVar2[0x47] == 0xffffffff) {
    if (((*(byte *)((int)puVar2 + 0x106) & 4) == 0) && ((short)puVar2[0x2d] == 0)) {
      iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      object_get_position();
      fVar1 = (float)puVar2[0x143];
      if (((puVar2[0x133] & 1) == 0) && ((0.0 < fVar1 && (fVar1 < 1.0)))) {
        fVar5 = *(float *)(DAT_006f1d6c + 0x1c) * 29.999998 * *(float *)(iVar3 + 0x4cc);
        if (*(char *)((int)puVar2 + 0x2a7) == '\x03') {
          fVar1 = fVar5 + fVar1;
        }
        else {
          fVar1 = fVar1 - fVar5;
        }
      }
      unaff_EDI[2] = (uint)(fVar1 * *(float *)(iVar3 + 0x404) +
                            (1.0 - fVar1) * *(float *)(iVar3 + 0x400) + (float)unaff_EDI[2]);
      return;
    }
    if (puVar2[0xca] != 0xffffffff) {
      object_get_node_local_transform();
      *unaff_EDI = local_c;
      unaff_EDI[1] = local_8;
      unaff_EDI[2] = local_4;
      return;
    }
  }
  else {
    puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar2[0x47] & 0xffff) * 0xc);
    *unaff_EDI = puVar4[0x17];
    unaff_EDI[1] = puVar4[0x18];
    unaff_EDI[2] = puVar4[0x19];
    if ((1 << ((byte)(short)puVar4[0x2d] & 0x1f) & 3U) == 0) {
      return;
    }
    if ((short)puVar2[0xbc] == -1) {
      return;
    }
    if (((short)puVar4[0x2d] == 1) &&
       (*(char *)((short)puVar2[0xbc] * 0x11c +
                  *(int *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + iVar3) + 0x2e8) + 0x84) ==
        '\0')) {
      return;
    }
  }
  object_get_node_local_transform();
  *unaff_EDI = local_c;
  unaff_EDI[1] = local_8;
  unaff_EDI[2] = local_4;
  return;
}
#endif
