// unit_apply_network_health_update  (Ghidra: unit_apply_network_health_update, renamed)
// address 0x55b5f0, size 395 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: biped_data.network_grenade_counts/network_body_vitality/network_shield_vitality/
//   network_shield_stunned (0x52c/0x530/0x534/0x538) and their baseline_* mirrors (0x540/0x544/
//   0x548/0x54c), network_update_sequence/network_delta_sequence (0x527/0x528),
//   network_baseline_valid (0x53c), unit_data.grenade_counts (0x31e), unit_data.unknown_475,
//   object.body_vitality/shield_vitality/shield_stun_ticks (0xe0/0xe4/0x104) -- all from
//   types/units.h and types/objects.h. The incoming record is unit_network_update_record
//   (types/units.h); this function is the half of the pair that *confirms* its first four
//   byte fields against what unit_submit_periodic_network_update (0x55b440) writes there.
// register convention: object_try_and_get takes the index in ECX and the type mask on the stack
//   (see src/objects/object_light_recompute_transform.c's disassembly-checked note), so Ghidra's
//   `object_try_and_get(1)` is mask = 1 (biped) with the index an unresolved register read.
// UNSURE: the staleness guard tests object.flags bit 0x8000000 on the object looked up from
//   param_1 through object_data, but every *write* goes to the object object_try_and_get
//   returned. The two are kept as separate pointers here because the original keeps them
//   separate; whether they are always the same object is an open question.
// UNSURE: the four network fields are cached *before* message_delta_decode_compound_field_forced/message_delta_decode_compound_field runs and written
//   back inside the is_full_update branch. If those two functions are the "commit the decoded
//   delta" step (which is how 0x566c90 uses message_delta_decode_compound_field), the cached values are the pre-decode
//   ones and the write-back is a deliberate revert; if they are pure predicates, the write-back
//   is a no-op. The structure is reproduced verbatim either way -- this is the single biggest
//   open question in the networking column and is listed for hook verification.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, index in ECX, mask on the stack
extern void message_delta_decode_compound_field_staged(void);             // 0x4ec670, UNSURE module: rejects the message
extern uint8_t message_delta_decode_compound_field(void);             // 0x4ec590, UNSURE module: accepts an unreliable message
extern char message_delta_decode_compound_field_forced(int32_t param_1);  // 0x4ec600, UNSURE module: accepts a reliable message

// Applies an incoming biped health/shield/grenade network update. Rejects it when the object
// already has a resync pending (object.flags bit 0x8000000), the record is a reliable one, and
// the record's sequence bytes say it is not newer than what is already applied. On acceptance it
// takes the record's delta sequence, applies the cached network block to the object's live
// vitality fields, mirrors it into the baseline snapshot and raises the resend / baseline flags.
void unit_apply_network_health_update(uint32_t object_index, void *message)
{
    // the object the staleness guard inspects, addressed through object_data from param_1
    object *guard_object = ((object_header *)object_data->data)[object_index & 0xffff].data;
    // the object every write lands on; its index is the unresolved ECX read
    datum_index target_index = k_datum_index_none; // UNSURE: unresolved ECX read
    object *obj = object_try_and_get(target_index, 1);

    if (obj == 0) {
        message_delta_decode_compound_field_staged();
        return;
    }

    {
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
        unit_network_update_record *record =
            (unit_network_update_record *)((void **)message)[0x11]; // +0x44
        int32_t record_type = **(int32_t **)message;                 // *(int *)*param_2

        // Reject when a resync is pending, the message is the reliable kind, and either the
        // update sequence differs from ours, or the incoming delta sequence is not ahead of ours
        // inside a 30-step window. Both halves are unsigned byte arithmetic widened to 32 bits;
        // Ghidra's `(int)((uint)a - (uint)b + 0xff)` is reproduced exactly because the
        // subtraction is what wraps.
        if ((guard_object->flags & 0x8000000) != 0 && record_type == 1 &&
            (record->update_sequence != (uint8_t)biped->network_update_sequence ||
             ((uint32_t)record->delta_sequence <= (uint32_t)biped->network_delta_sequence &&
              (int32_t)(((uint32_t)record->delta_sequence -
                         (uint32_t)biped->network_delta_sequence) + 0xff) > 0x1d))) {
            message_delta_decode_compound_field_staged();
            return;
        }

        {
            // read as dwords: the original moves 0x52c..0x53b four dwords at a time, so the
            // int16/int8 fields carry their trailing pad bytes with them.
            uint32_t cached_grenade_counts = *(uint32_t *)&biped->network_grenade_counts;
            uint32_t cached_body_vitality  = *(uint32_t *)&biped->network_body_vitality;
            float    cached_shield_vitality = biped->network_shield_vitality;
            uint32_t cached_shield_stunned = *(uint32_t *)&biped->network_shield_stunned;
            char accepted = (record_type == 1) ? message_delta_decode_compound_field_forced(0) : message_delta_decode_compound_field();

            if (accepted != 0) {
                biped->network_delta_sequence = record->delta_sequence;
                obj->flags = obj->flags | 0x8000000;

                if (record->is_full_update != 0) {
                    biped->network_update_sequence = record->update_sequence;
                    *(uint32_t *)&biped->network_grenade_counts = cached_grenade_counts;
                    *(uint32_t *)&biped->network_body_vitality  = cached_body_vitality;
                    biped->network_shield_vitality              = cached_shield_vitality;
                    *(uint32_t *)&biped->network_shield_stunned = cached_shield_stunned;
                }

                cached_shield_vitality = cached_shield_vitality * 3.0f;

                // object 0x31e is unit_data.grenade_counts[2], written as one int16 -- not the
                // 0x31d desired_grenade_index byte next to it.
                *(int16_t *)&unit->grenade_counts[0] = (int16_t)cached_grenade_counts;
                *(uint32_t *)&obj->body_vitality = cached_body_vitality;
                if (record->shield_recharging == 1) {
                    obj->shield_vitality = cached_shield_vitality;
                }

                *(uint32_t *)&biped->baseline_grenade_counts = cached_grenade_counts;
                *(uint32_t *)&biped->baseline_body_vitality  = cached_body_vitality;
                biped->baseline_shield_vitality              = cached_shield_vitality;
                *(uint32_t *)&biped->baseline_shield_stunned = cached_shield_stunned;

                // the stun stamp is written as a uint16 from the LOW BYTE of the cached dword
                obj->shield_stun_ticks = (int16_t)(uint16_t)((char)cached_shield_stunned == 1);
                unit->unknown_475 = 1;
                biped->network_baseline_valid = 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x55b5f0):

void FUN_0055b5f0(uint param_1,undefined4 *param_2)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  float fVar4;
  undefined4 uVar5;
  char cVar6;
  int iVar7;

  iVar7 = object_try_and_get(1);
  if (iVar7 == 0) {
    FUN_004ec670();
    return;
  }
  iVar1 = param_2[0x11];
  if ((((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0x10) &
        0x8000000) != 0) && (*(int *)*param_2 == 1)) &&
     ((*(char *)(iVar1 + 4) != *(char *)(iVar7 + 0x527) ||
      (((uint)*(byte *)(iVar1 + 5) <= (uint)*(byte *)(iVar7 + 0x528) &&
       (0x1d < (int)(((uint)*(byte *)(iVar1 + 5) - (uint)*(byte *)(iVar7 + 0x528)) + 0xff))))))) {
    FUN_004ec670();
    return;
  }
  uVar2 = *(undefined4 *)(iVar7 + 0x52c);
  uVar3 = *(undefined4 *)(iVar7 + 0x530);
  fVar4 = *(float *)(iVar7 + 0x534);
  uVar5 = *(undefined4 *)(iVar7 + 0x538);
  if (*(int *)*param_2 == 1) {
    cVar6 = FUN_004ec600(0);
  }
  else {
    cVar6 = FUN_004ec590();
  }
  if (cVar6 != '\0') {
    *(undefined1 *)(iVar7 + 0x528) = *(undefined1 *)(iVar1 + 5);
    *(uint *)(iVar7 + 0x10) = *(uint *)(iVar7 + 0x10) | 0x8000000;
    if (*(char *)(iVar1 + 6) != '\0') {
      *(undefined1 *)(iVar7 + 0x527) = *(undefined1 *)(iVar1 + 4);
      *(undefined4 *)(iVar7 + 0x52c) = uVar2;
      *(undefined4 *)(iVar7 + 0x530) = uVar3;
      *(float *)(iVar7 + 0x534) = fVar4;
      *(undefined4 *)(iVar7 + 0x538) = uVar5;
    }
    fVar4 = fVar4 * 3.0;
    *(short *)(iVar7 + 0x31e) = (short)uVar2;
    *(undefined4 *)(iVar7 + 0xe0) = uVar3;
    if (*(char *)(iVar1 + 7) == '\x01') {
      *(float *)(iVar7 + 0xe4) = fVar4;
    }
    *(undefined4 *)(iVar7 + 0x540) = uVar2;
    *(undefined4 *)(iVar7 + 0x544) = uVar3;
    *(float *)(iVar7 + 0x548) = fVar4;
    *(undefined4 *)(iVar7 + 0x54c) = uVar5;
    *(ushort *)(iVar7 + 0x104) = (ushort)((char)uVar5 == '\x01');
    *(undefined1 *)(iVar7 + 0x475) = 1;
    *(undefined1 *)(iVar7 + 0x53c) = 1;
  }
  return;
}
#endif
