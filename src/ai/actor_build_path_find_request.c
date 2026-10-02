// actor_build_path_find_request  (Ghidra: actor_build_path_find_request, renamed)
// address 0x41a9c0, size 236 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: the body zeroes exactly 0x12 dwords through EBX (rep stosd, ECX=0x12) -- the
// 0x48 bytes of path_find_request -- and then writes the same field set the two firing
// position builders fill in: +0x00 pathfinding_radius, +0x04 ignores_glass, +0x08 a unit
// handle, +0x0c none, +0x10 have_start = 1, +0x14 start_position, +0x20 start_surface_index.
// The radius comes from Actor.pathfinding_radius (Actor+0x8c) and, while the actor is
// driving (actor.unknown_15e > 0), from Vehicle.ai_pathfinding_radius (Vehicle+0x38c) when
// that is positive; the unit handle then comes from active_unit_index instead of unit_index.
// register convention: actor_index in EAX, request pointer in EBX (Ghidra's unaff_EBX);
// objdump confirms `mov eax,edi` immediately before `call 0x429570`, so the lead-position
// update takes the actor index.
// blam-cc: EAX -> actor_index, EBX -> request
// UNSURE: actor.unknown_164 / unknown_168..0x170 are named here by their role in this
// request (a pathfinding surface index and the position that goes with it); types/ai.h still
// calls them unknown_164 / unknown_168 and this file does not rename them.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;       // 0x00880360
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern void actor_update_target_lead_position(datum_index actor_index); // 0x429570, EAX -> actor_index

// blam-cc: EAX -> actor_index, EBX -> request
void actor_build_path_find_request(datum_index actor_index, path_find_request *request)
{
    actor *self;
    Actor *actor_definition;
    Vehicle *vehicle_definition;
    object *unit_object;
    datum_index unit_index;
    float radius;
    uint8_t ignores_glass;
    uint32_t *clear;
    int32_t i;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    unit_index = self->unit_index;
    actor_definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    radius = actor_definition->pathfinding_radius;

    if (self->vehicle_driving_type > 0) {
        unit_index = self->active_unit_index;
        unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        vehicle_definition = (Vehicle *)tag_instances[unit_object->definition_tag & 0xffff].data;
        if (vehicle_definition->ai_pathfinding_radius > 0.0f) {
            radius = vehicle_definition->ai_pathfinding_radius;
        }
    }

    actor_update_target_lead_position(actor_index);
    ignores_glass = self->ignores_glass;

    clear = (uint32_t *)request;
    for (i = 0x12; i != 0; i--) {
        *clear = 0;
        clear++;
    }

    request->exclude_object_index_a = unit_index;
    request->pathfinding_radius = radius;
    request->ignores_glass = ignores_glass;
    request->exclude_object_index_b = (datum_index)k_datum_index_none;
    request->have_start = 1;
    request->start_position.x = *(float *)&self->pathfinding_point;
    request->start_position.y = *(float *)&self->unknown_16c;
    request->start_position.z = *(float *)&self->unknown_170;
    request->start_surface_index = (uint32_t)self->pathfinding_surface_index;
}

#if 0
Original Ghidra decompilation (0x41a9c0):

void FUN_0041a9c0(void)

{
  undefined1 uVar1;
  undefined4 uVar2;
  uint in_EAX;
  int iVar3;
  undefined4 *unaff_EBX;
  uint uVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 local_4;

  iVar5 = (in_EAX & 0xffff) * 0x724;
  uVar4 = *(uint *)(iVar5 + 0x18 + *(int *)(DAT_00880360 + 0x34));
  iVar5 = iVar5 + *(int *)(DAT_00880360 + 0x34);
  local_4 = *(undefined4 *)
             (*(int *)((*(uint *)(iVar5 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x8c);
  if (0 < *(short *)(iVar5 + 0x15e)) {
    uVar4 = *(uint *)(iVar5 + 0x158);
    iVar3 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) &
                     0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (0.0 < *(float *)(iVar3 + 0x38c)) {
      local_4 = *(undefined4 *)(iVar3 + 0x38c);
    }
  }
  actor_update_target_lead_position();
  uVar1 = *(undefined1 *)(iVar5 + 0x376);
  puVar6 = unaff_EBX;
  for (iVar3 = 0x12; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  unaff_EBX[2] = uVar4;
  *unaff_EBX = local_4;
  *(undefined1 *)(unaff_EBX + 1) = uVar1;
  unaff_EBX[3] = 0xffffffff;
  uVar2 = *(undefined4 *)(iVar5 + 0x164);
  *(undefined1 *)(unaff_EBX + 4) = 1;
  unaff_EBX[5] = *(undefined4 *)(iVar5 + 0x168);
  unaff_EBX[6] = *(undefined4 *)(iVar5 + 0x16c);
  unaff_EBX[7] = *(undefined4 *)(iVar5 + 0x170);
  unaff_EBX[8] = uVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
