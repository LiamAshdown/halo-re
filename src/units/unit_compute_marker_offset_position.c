// unit_compute_marker_offset_position  (Ghidra: unit_compute_marker_offset_position, renamed)
// address 0x55a170, size 359 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: biped_data.crouch_fraction (0x50c, types/units.h) matches puVar4[0x143]; the
//   crouch-timing blend mirrors unit_get_crouch_height_offset (0x55a2e0) almost exactly. Unit
//   tag offsets 0x400/0x404 are UNSURE (not yet named -- likely a pair of marker-position
//   fields blended by base_animation_state).
// register convention: object index in ECX, a direction vector in EDX, a mode selector in the
//   low 16 bits of EBX, and the output position accumulator in ESI; base_position/offsets are
//   Ghidra-recognized stack parameters.
//   // blam-cc: ECX -> object_index, EDX -> reference_direction, BX -> mode, ESI -> out_position,
//   //           stack -> base_position, offsets
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

// object_get_position (0x4f6900, defined in src/objects/object_get_position.c) writes the
// object position through the pointer in EAX and leaves that same pointer in EAX on return;
// the object index is in ECX. Ghidra binds a different subset of the two operands at each call
// site in this module, so the declaration is left unprototyped.
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX out, ECX object

// Fills out_position with a blend-mode-dependent offset position for a unit marker/attachment
// point. Mode 0 just copies the object's position. Any other mode seeds out_position from
// base_position, and mode 3 additionally offsets it along reference_direction and a perpendicular
// built from offsets. The Z component is then blended between the Unit tag's two marker Z
// offsets (0x400/0x404, UNSURE) by a fraction that is 0/1 for modes 1/2, or the biped's
// crouch_fraction (rate-limited toward the base_animation_state==3 "crouching" target when
// mid-transition) for every other mode.
void unit_compute_marker_offset_position(uint32_t object_index, real_vector3d *reference_direction,
                                          int16_t mode, real_point3d *out_position, float *base_position,
                                          float *offsets)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t *tag_data = (uint8_t *)tag;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    float fraction;

    if (mode == 0) {
        object_get_position(out_position, object_index); // 0x55a1a3: EAX = out (esi), ECX = the unit
    } else {
        *out_position = *(real_point3d *)base_position;
        if (mode == 3) {
            out_position->x += offsets[0] * reference_direction->i;
            out_position->y += offsets[0] * reference_direction->j;
            out_position->z += offsets[0] * reference_direction->k;
            out_position->x += offsets[1] * -reference_direction->j;
            out_position->y += offsets[1] * reference_direction->i;
            out_position->z = (out_position->z + offsets[1] * 0.0f) + offsets[2];
            return;
        }
    }

    if (mode == 1) {
        fraction = 0.0f;
    } else if (mode == 2) {
        fraction = 1.0f;
    } else {
        fraction = biped->crouch_fraction;
        if ((biped->flags & 1) == 0 && fraction > 0.0f && fraction < 1.0f) {
            float step = game_time->leftover_time * 29.999998f * tag->crouch_camera_velocity;
            if (unit->base_animation_state == 3) {
                fraction += step;
            } else {
                fraction -= step;
            }
        }
    }
    out_position->z += fraction * *(float *)((uint8_t *)tag_data + 0x404) +
                        (1.0f - fraction) * *(float *)((uint8_t *)tag_data + 0x400);
}

#if 0
Original Ghidra decompilation (0x55a170):

void FUN_0055a170(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  uint *puVar4;
  int iVar5;
  uint in_ECX;
  float *in_EDX;
  short unaff_BX;
  float *unaff_ESI;

  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (unaff_BX == 0) {
    object_get_position();
  }
  else {
    *unaff_ESI = *param_1;
    unaff_ESI[1] = param_1[1];
    unaff_ESI[2] = param_1[2];
    if (unaff_BX == 3) {
      fVar1 = in_EDX[1];
      fVar2 = *in_EDX;
      fVar3 = *param_2;
      *unaff_ESI = fVar3 * *in_EDX + *unaff_ESI;
      unaff_ESI[1] = fVar3 * in_EDX[1] + unaff_ESI[1];
      unaff_ESI[2] = fVar3 * in_EDX[2] + unaff_ESI[2];
      fVar3 = param_2[1];
      *unaff_ESI = fVar3 * -fVar1 + *unaff_ESI;
      unaff_ESI[1] = fVar3 * fVar2 + unaff_ESI[1];
      fVar1 = fVar3 * 0.0 + unaff_ESI[2];
      unaff_ESI[2] = fVar1;
      unaff_ESI[2] = fVar1 + param_2[2];
      return;
    }
  }
  if (unaff_BX == 1) {
    fVar1 = 0.0;
  }
  else if (unaff_BX == 2) {
    fVar1 = 1.0;
  }
  else {
    fVar1 = (float)puVar4[0x143];
    if ((((puVar4[0x133] & 1) == 0) && (0.0 < fVar1)) && (fVar1 < 1.0)) {
      fVar2 = *(float *)(DAT_006f1d6c + 0x1c) * 29.999998 * *(float *)(iVar5 + 0x4cc);
      if (*(char *)((int)puVar4 + 0x2a7) == '\x03') {
        fVar1 = fVar2 + fVar1;
      }
      else {
        fVar1 = fVar1 - fVar2;
      }
    }
  }
  unaff_ESI[2] = fVar1 * *(float *)(iVar5 + 0x404) + (1.0 - fVar1) * *(float *)(iVar5 + 0x400) +
                 unaff_ESI[2];
  return;
}
#endif
