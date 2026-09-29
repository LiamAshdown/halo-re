// actor_fill_unit_position_context  (Ghidra: actor_fill_unit_position_context, renamed)
// address 0x4296c0, size 211 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: types/objects.h object.forward (0x74)/parent_object (0x11c)/location_leaf_index
//   (0x098)/location_cluster_index (0x09c); types/ai.h header notes ai_marker_name_a
//   (0x0066bfa0, already declared in src/ai/actor_target_data_refresh.c) as the marker name
//   passed to object_get_node_local_transform. Phase-4 summary: "Fills a caller-provided
//   struct with the actor's unit position/orientation plus a pair of fields from the root
//   object at the top of its parent chain." Calls object_get_position (0x4f6900),
//   object_get_node_local_transform (0x4f6080, already established in this module's sibling
//   files) and object_get_root_object_velocities (UNSURE signature, not established elsewhere in this repo).
//   UNSURE: this is one of the least-confident rewrites in this pass. The buffer Ghidra shows
//   object_get_node_local_transform filling is only 12 bytes here (unlike the 0x6c-byte
//   object_marker the other established call site uses), suggesting a different flag
//   argument produces a smaller result; modeled as a bare real_point3d with flag 0, but not
//   independently confirmed with objdump. object_get_position's own output here is
//   immediately overwritten by that second call and so has no visible effect; kept as a call
//   for its side effects only.
// register convention: EBX -> unit_index (unaff_EBX), stack -> out_context.
//   // blam-cc: EBX -> unit_index, stack -> out_context

// REWRITTEN (from objdump 0x4296c0..0x429792): context +0x0c = object_get_position(EAX = +0x0c, ECX = unit);
//   +0x18 = the unit's forward (+0x74); +0x00 = the position of marker 0x66bfa0 (object_get_node_local_transform
//   flags 1, node_transform.position); +0x2c = the root velocities (EAX unit, ESI = +0x2c, EDI = NULL); +0x24/+0x28
//   = the root object's location dwords (+0x98/+0x9c), the root found by following parent_object (+0x11c).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *object_data; // 0x008603b0
extern char ai_marker_name_a[]; // 0x0066bfa0

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, blam-cc: EAX, ECX
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, blam-cc: EAX, ESI, EDI

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

// blam-cc: EBX -> unit_index, stack -> out_context
void actor_fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context)
{
    uint8_t *context = (uint8_t *)out_context;
    uint8_t *unit = object_get(unit_index);
    object_marker marker;
    datum_index root = k_datum_index_none;
    uint8_t *root_object;

    object_get_position((real_point3d *)(context + 0xc), unit_index);
    *(real_vector3d *)&((struct actor_unit_position_context *)context)->forward.i = *(real_vector3d *)&((unit_object *)unit)->base.forward.i;
    object_get_node_local_transform(unit_index, ai_marker_name_a, &marker, 1);
    *(real_point3d *)context = marker.node_transform.position;
    object_get_root_object_velocities(unit_index, (real_vector3d *)(context + 0x2c), 0);
    if (unit_index != k_datum_index_none) {
        datum_index cursor = unit_index;

        do {
            root = cursor;
            cursor = *(datum_index *)(object_get(cursor) + 0x11c);
        } while (cursor != k_datum_index_none);
    }
    root_object = object_get(root);
    *(uint32_t *)&((struct actor_unit_position_context *)context)->root_position_x = *(uint32_t *)(root_object + 0x98);
    *(uint32_t *)&((struct actor_unit_position_context *)context)->root_position_y = *(uint32_t *)(root_object + 0x9c);
}

#if 0
Original Ghidra decompilation (0x4296c0):

void FUN_004296c0(undefined4 *param_1)

{
  int iVar1;
  uint uVar2;
  uint unaff_EBX;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  object_get_position();
  param_1[6] = *(undefined4 *)(iVar1 + 0x74);
  param_1[7] = *(undefined4 *)(iVar1 + 0x78);
  param_1[8] = *(undefined4 *)(iVar1 + 0x7c);
  object_get_node_local_transform();
  *param_1 = local_c;
  param_1[1] = local_8;
  param_1[2] = local_4;
  FUN_004f6aa0();
  uVar2 = 0xffffffff;
  if (unaff_EBX != 0xffffffff) {
    do {
      uVar2 = unaff_EBX;
      unaff_EBX = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) +
                           0x11c);
    } while (unaff_EBX != 0xffffffff);
  }
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
  param_1[9] = *(undefined4 *)(iVar1 + 0x98);
  param_1[10] = *(undefined4 *)(iVar1 + 0x9c);
  return;
}
#endif
