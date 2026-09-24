// projectile_attach_apply  (Ghidra: FUN_004bf1c0; renamed per
// out/phase4/projectiles_types_notes.md "Renames this pass establishes")
// address 0x4bf1c0, size 457 bytes
// name confidence: 0.8   rewrite confidence: 0.8 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: out/phase4/projectiles_types_notes.md: "receiver of 0x33: zeroes velocity and
//   angular_velocity, sets flags 0x08 and object flag 0x20, object_attach_to_object, seeds
//   detonation_timer_rate, runs the super-combine sibling sweep"; the super-combine constants
//   and bit names (k_projectile_super_combine_attach_threshold, has_super_combining_explosion,
//   detonation_max_time_if_attached, random_attached_detonation_time) and the ProjectileFlags
//   bit order are documented in types/projectiles.h with this function cited. `objdump -d -M
//   intel bin/halo.exe` for 0x4bf1c0..0x4bf390 resolved every register Ghidra elided: the
//   projectile's own resolved handle and the parent's resolved handle both come from the
//   message via the same hash -> handle table read src/objects/object_delete_by_pooled_node_id.c
//   already established, object_try_and_get's (index in ECX, mask on stack) convention matches
//   that same file, and object_attach_to_object's (parent, child, marker) argument order matches
//   src/objects/object_attach_to_object.c's own signature.
// register convention: the incoming decoded-message record pointer in EAX (in_EAX), same shape
//   as projectile_detonation_message_apply's incoming_record.
// blam-cc: EAX -> incoming_record
// UNSURE: velocity and angular_velocity are zeroed here by copying the constant
//   global_origin3d_pointer (0,0,0) into both, which is what the disassembly shows
//   (`mov eax,ds:0x696714 ... mov [esi+0x68],...`) rather than a literal zero store; preserved
//   as an explicit real_vector3d zero-assign since the effect is identical.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *object_pooled_node_globals; // 0x00687130

extern uint8_t message_delta_decode_compound_field(void *out_state, void *incoming_record); // 0x4ec590, networking;
    // decodes the message body into out_state.
    // blam-cc: ECX -> out_state, EAX -> incoming_record (0x4ec590 opens with
    // `mov edi,[eax]` and passes ECX straight through to 0x4ed1d0)
extern int32_t message_delta_decode_compound_field_staged(void); // 0x4ec670, networking; drops the message
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index, int16_t marker_index); // 0x4f6440
extern real random_real_range(real min, real max); // 0x401050, math module

// Receiver of network message 0x33 (k_message_projectile_attach), sent by the attach response
// (projectile_response, this batch) when a projectile stuck to an object and both ends are
// authoritative. Resolves both handles, validates the projectile and its new parent, runs the
// super-combining-explosion sibling sweep against the parent's existing children (zeroing the
// timers of up to k_projectile_super_combine_attach_threshold siblings sharing this
// projectile's definition_tag, then flagging the projectile itself once that many are found),
// zeroes velocity and angular_velocity, marks the projectile attached and at rest,
// object_attach_to_object's it to the parent at the given marker, and seeds
// detonation_timer_rate from whichever of detonation_max_time_if_attached (fixed) or
// random_attached_detonation_time (randomised between timer[0] and timer[1]) the tag flags ask
// for.
void projectile_attach_apply(void *incoming_record)
{
    projectile_attach_message decoded;
    datum_index projectile_handle, parent_handle;
    object *self, *parent;

    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }
    if (message_delta_decode_compound_field(&decoded, incoming_record) == 0) {
        return;
    }

    projectile_handle = (datum_index)0xffffffff;
    if (decoded.object_hash != 0) {
        projectile_handle = (*(datum_index **)(object_pooled_node_globals + 0x28))[decoded.object_hash];
    }
    parent_handle = (datum_index)0xffffffff;
    if (decoded.parent_hash != 0) {
        parent_handle = (*(datum_index **)(object_pooled_node_globals + 0x28))[decoded.parent_hash];
    }

    self = object_try_and_get(projectile_handle, _object_mask_projectile);
    if (self == 0 || object_try_and_get(parent_handle, _object_mask_all) == 0) {
        return;
    }

    {
        Projectile *tag = (Projectile *)tag_instances[self->definition_tag & 0xffff].data;
        projectile_data *self_pd = (projectile_data *)((uint8_t *)self + k_projectile_data_offset);

        if ((tag->projectile_flags & _projectile_definition_has_super_combining_explosion_bit) != 0) {
            parent = ((object_header *)object_data->data)[parent_handle & 0xffff].data;
            datum_index sibling_index = parent->first_child_object;
            int16_t sibling_count = 0;
            while (sibling_index != (datum_index)0xffffffff) {
                object *sibling = ((object_header *)object_data->data)[sibling_index & 0xffff].data;
                projectile_data *sibling_pd = (projectile_data *)((uint8_t *)sibling + k_projectile_data_offset);
                if (sibling->definition_tag == self->definition_tag &&
                    (sibling_pd->flags & _projectile_super_detonation_counted_bit) == 0) {
                    sibling_pd->arming_timer = 0.0f;
                    sibling_pd->detonation_timer = 0.0f;
                    sibling_count++;
                }
                if (k_projectile_super_combine_attach_threshold < sibling_count) {
                    self_pd->flags |= _projectile_super_detonation_bit;
                    break;
                }
                sibling_index = sibling->next_object;
            }
        }

        self->velocity.i = 0.0f;
        self->velocity.j = 0.0f;
        self->velocity.k = 0.0f;
        self->angular_velocity.i = 0.0f;
        self->angular_velocity.j = 0.0f;
        self->angular_velocity.k = 0.0f;
        self_pd->flags |= _projectile_attached_bit;
        self->flags |= _object_at_rest_bit;

        object_attach_to_object(parent_handle, projectile_handle, decoded.parent_marker_index);

        if ((tag->projectile_flags & _projectile_definition_detonation_max_time_if_attached_bit) != 0) {
            real t = tag->timer[1];
            if (1.0f <= t * 30.0f) {
                self_pd->detonation_timer_rate = 1.0f / (t * 30.0f);
            }
        } else if ((tag->projectile_flags & _projectile_definition_random_attached_detonation_time_bit) != 0) {
            real t = random_real_range(tag->timer[0], tag->timer[1]);
            if (1.0f <= t * 30.0f) {
                self_pd->detonation_timer_rate = 1.0f / (t * 30.0f);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4bf1c0):

void FUN_004bf1c0(void)

{
  uint uVar1;
  uint *puVar2;
  undefined *puVar3;
  int iVar4;
  char cVar5;
  undefined4 *in_EAX;
  uint *puVar6;
  int iVar7;
  short sVar8;
  uint uVar9;
  float fVar10;
  undefined4 local_10;
  int local_c;
  int local_8;
  undefined4 local_4;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return;
  }
  cVar5 = FUN_004ec590();
  if (cVar5 != '\0') {
    local_10 = 0xffffffff;
    if (local_c != 0) {
      local_10 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_c * 4);
    }
    uVar9 = 0xffffffff;
    if (local_8 != 0) {
      uVar9 = *(uint *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_8 * 4);
    }
    puVar6 = (uint *)object_try_and_get(0x20);
    if ((puVar6 != (uint *)0x0) &&
       (iVar7 = object_try_and_get(0xffffffff), iVar4 = DAT_008603b0, iVar7 != 0)) {
      iVar7 = *(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if ((*(byte *)(iVar7 + 0x17c) & 8) != 0) {
        sVar8 = 0;
        uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) +
                         0x118);
        while (uVar1 != 0xffffffff) {
          puVar2 = *(uint **)(*(int *)(iVar4 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
          if ((*puVar2 == *puVar6) && ((puVar2[0x8b] & 0x40) == 0)) {
            puVar2[0x92] = 0;
            puVar2[0x90] = 0;
            sVar8 = sVar8 + 1;
          }
          if (5 < sVar8) {
            puVar6[0x8b] = puVar6[0x8b] | 0x80;
            break;
          }
          uVar1 = puVar2[0x45];
        }
      }
      puVar3 = PTR_DAT_00696714;
      puVar6[0x1a] = *(uint *)PTR_DAT_00696714;
      puVar6[0x1b] = *(uint *)(puVar3 + 4);
      puVar6[0x1c] = *(uint *)(puVar3 + 8);
      puVar6[0x23] = *(uint *)puVar3;
      puVar6[0x24] = *(uint *)(puVar3 + 4);
      puVar6[0x25] = *(uint *)(puVar3 + 8);
      puVar6[0x8b] = puVar6[0x8b] | 8;
      puVar6[4] = puVar6[4] | 0x20;
      object_attach_to_object(uVar9,local_10,local_4);
      if ((*(uint *)(iVar7 + 0x17c) & 4) == 0) {
        if ((*(uint *)(iVar7 + 0x17c) & 0x20) == 0) {
          return;
        }
        fVar10 = random_real_range(*(float *)(iVar7 + 0x1bc),*(float *)(iVar7 + 0x1c0));
      }
      else {
        fVar10 = *(float *)(iVar7 + 0x1c0);
      }
      if (1.0 <= fVar10 * 30.0) {
        puVar6[0x91] = (uint)(1.0 / (fVar10 * 30.0));
      }
    }
  }
  return;
}
#endif
