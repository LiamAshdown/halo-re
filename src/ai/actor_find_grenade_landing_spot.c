// actor_find_grenade_landing_spot  (Ghidra: actor_find_grenade_landing_spot, renamed)
// address 0x410c90, size 263 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against 0x410c90 (range gates, last known position +0.2 z, outputs, ESI/stack scatter call))
// evidence: phase-4 summary "finds a suitable grenade landing point at the current threat's
// position when it is within the actor's configured grenade range"; on success writes the
// target's last_known_position (z nudged by +0.2) through a point out-parameter, the
// target's own handle through out_target_handle, and the target prop's
// relationship_object_index through out_relationship.
// register convention: actor_index in EAX, output point in ECX (both Ghidra in_EAX/in_ECX);
// out_target_handle and out_relationship are Ghidra-recognized stack parameters.
// blam-cc: EAX -> actor_index, ECX -> out_point, stack -> out_target_handle, out_relationship
// UNSURE: the two ActorVariant range floats compared here (0x194, 0x198) do not cleanly
// match a single named field pair in types/tags.h's ActorVariant (0x194 falls inside
// grenade_ranges[1], 0x198 is the start of collateral_damage_radius); kept as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern void actor_choose_random_point_near(real_point3d *inout_point, float radius); // 0x40faf0, this module

// blam-cc: EAX -> actor_index, ECX -> out_point, stack -> out_target_handle, out_relationship
uint8_t actor_find_grenade_landing_spot(datum_index actor_index, real_point3d *out_point,
                                         datum_index *out_target_handle, int32_t *out_relationship)
{
    actor *self;
    ActorVariant *variant;
    uint8_t result = 0;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *target_prop = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
        if (target_prop->enemy != 0 && target_prop->dead == 0) {
            int16_t kind = target_prop->state;
            if ((1 < kind && kind < 4) || kind == 4) {
                float min_range = *(float *)((uint8_t *)variant + 0x194);
                float max_range = *(float *)((uint8_t *)variant + 0x198);
                if (min_range < target_prop->distance && target_prop->distance < max_range) {
                    *out_point = target_prop->last_known_position;
                    result = 1;
                    out_point->z = out_point->z + 0.2f;
                    *out_target_handle = self->target_unit_index;
                    *out_relationship = target_prop->relationship_object_index;
                    if (self->playfight != 0) {
                        actor_choose_random_point_near(out_point, 1.5f);
                    }
                }
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x410c90):

undefined1 FUN_00410c90(undefined4 *param_1,undefined4 *param_2)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  undefined4 *in_ECX;
  undefined1 uVar5;

  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar2 = *(int *)((*(uint *)(iVar3 + 0x5c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar5 = 0;
  if (*(uint *)(iVar3 + 0x270) != 0xffffffff) {
    iVar4 = (*(uint *)(iVar3 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    if ((*(char *)(iVar4 + 0x60) != '\0') && (*(char *)(iVar4 + 0x127) == '\0')) {
      sVar1 = *(short *)(iVar4 + 0x24);
      if (((1 < sVar1) && (sVar1 < 4)) || (sVar1 == 4)) {
        if (*(float *)(iVar2 + 0x194) < *(float *)(iVar4 + 0x11c)) {
          if (*(float *)(iVar4 + 0x11c) < *(float *)(iVar2 + 0x198)) {
            *in_ECX = *(undefined4 *)(iVar4 + 0xbc);
            in_ECX[1] = *(undefined4 *)(iVar4 + 0xc0);
            in_ECX[2] = *(undefined4 *)(iVar4 + 0xc4);
            uVar5 = 1;
            in_ECX[2] = (float)in_ECX[2] + 0.2;
            *param_1 = *(undefined4 *)(iVar3 + 0x270);
            *param_2 = *(undefined4 *)(iVar4 + 0x110);
            if (*(char *)(iVar3 + 0x1ca) != '\0') {
              FUN_0040faf0(0x3fc00000);
            }
          }
        }
      }
    }
  }
  return uVar5;
}
#endif
