// object_apply_shield_charge_and_notify
// address 0x4ee4d0, size 260 bytes
// name confidence: 0.3 (still FUN_004ee4d0 in Ghidra; named from out/phase4/objects_functions.md's
// summary: "Applies an energy/charge value to a target object's shield-flicker fields and
// dispatches a follow-up event, gated by a lookup-table validity check.")
// rewrite confidence: 0.25
// evidence: types/objects.h object.shield_damage_ticks (0xfc), object.current_shield_damage
// (0xe8), object.recent_shield_damage (0xf4), object.vitality_flags (0x106,
// _object_shield_depleted_bit).
// UNSURE: message_delta_decode_compound_field is called with no visible arguments or return handling, yet the locals
// `local_c`, `local_8` and `local_4` are read immediately afterward with no visible assignment;
// exactly as with object_damage_apply_line_of_sight, this means message_delta_decode_compound_field writes them through
// implicit pointers this decompile lost. They are transcribed as literal, apparently-unwritten
// reads. object_try_and_get's object-index argument is likewise not visible. message_delta_decode_compound_field_staged,
// unit_update_stance_and_jump and PTR_DAT_00687130 are all outside this module.
// register convention: a pointer-to-pointer parameter in EAX (in_EAX).
// blam-cc: EAX=param_1

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern uint8_t *object_pooled_node_globals; // 0x00687130, UNSURE: not owned by this module

extern int8_t message_delta_decode_compound_field(void *globals, void *out_value); // UNSURE: out of range, 0x4ec590
extern uint8_t message_delta_decode_compound_field_staged(void **context); // UNSURE: zero visible args; out of range, 0x4ec670
extern void object_set_shield_depleted_flag(uint32_t object_index); // this module, 0x4edb10
extern void object_throttled_multiplayer_sound_event(void); // this module, 0x4ee370 // this module, 0x4ee370 (object_throttled_multiplayer_sound_event)
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still); // UNSURE: out of range, 0x566de0

void object_apply_shield_charge_and_notify(void **param_1)
{
    int32_t local_c;   // UNSURE: apparently written by message_delta_decode_compound_field through an implicit pointer
    float local_8;     // UNSURE: same
    int8_t local_4;    // UNSURE: same

    if (*(int32_t *)*param_1 != 0) {
        message_delta_decode_compound_field_staged(0); // UNSURE: the EAX operand is not visible here
        return;
    }

    if (message_delta_decode_compound_field(param_1, &local_c) == 0) {
        return;
    }

    if (local_c != 0) {
        int32_t effect = (*(int32_t **)(object_pooled_node_globals + 0x28))[local_c]; // UNSURE: array base at +0x28

        if (effect != -1) {
            object *target = object_try_and_get(effect, _object_mask_unit);
            // 0x4ee514 mov ecx,edi -- the handle just tested above

            if (target != 0) {
                if (0.0f < local_8) {
                    target->shield_damage_ticks = 0;
                    if ((target->vitality_flags & _object_shield_depleted_bit) == 0) {
                        target->current_shield_damage = 1.0f;
                    }
                    target->recent_shield_damage = local_8 + target->recent_shield_damage;
                    if (1.0f < target->current_shield_damage) {
                        target->current_shield_damage = 1.0f;
                    }
                    if (1.0f < target->recent_shield_damage) {
                        target->recent_shield_damage = 1.0f;
                    }
                }
                if (local_4 == 1) {
                    object_set_shield_depleted_flag(0); // UNSURE: object index argument not visible
                }
                unit_update_stance_and_jump(effect, 0, 0, 0, 0, 0, 0, 0xffffffff, 0, 0);
            }
        }
    }

    object_throttled_multiplayer_sound_event();
}

#if 0
Original Ghidra decompilation (0x4ee4d0):

void FUN_004ee4d0(void)

{
  int iVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int local_c;
  float local_8;
  char local_4;

  if (*(int *)*in_EAX == 0) {
    cVar2 = FUN_004ec590();
    if (cVar2 != '\0') {
      if ((local_c != 0) &&
         (iVar1 = *(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_c * 4), iVar1 != -1)) {
        iVar3 = object_try_and_get(3);
        if (iVar3 != 0) {
          if (0.0 < local_8) {
            *(undefined4 *)(iVar3 + 0xfc) = 0;
            if ((*(byte *)(iVar3 + 0x106) & 8) == 0) {
              *(undefined4 *)(iVar3 + 0xe8) = 0x3f800000;
            }
            *(float *)(iVar3 + 0xf4) = local_8 + *(float *)(iVar3 + 0xf4);
            if (1.0 < *(float *)(iVar3 + 0xe8)) {
              *(undefined4 *)(iVar3 + 0xe8) = 0x3f800000;
            }
            if (1.0 < *(float *)(iVar3 + 0xf4)) {
              *(undefined4 *)(iVar3 + 0xf4) = 0x3f800000;
            }
          }
          if (local_4 == '\x01') {
            FUN_004edb10();
          }
          FUN_00566de0(iVar1,0,0,0,0,0,0,0xffffffff,0,0);
        }
      }
      FUN_004ee370();
      return;
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
