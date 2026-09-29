// light_transient_add
// address 0x4f1600, size 253 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md names this function directly:
// "light_transient_add writes eight parallel-array bases 4 bytes apart with a common stride of
// 0x28")
// rewrite confidence: 0.55
// evidence: types/objects.h light_transient (every field) and its global table at 0x008609cc,
// k_maximum_transient_lights (8), light_transient_count (0x00860b0c); render_window_index is the same
// unresolved byte used by object_lights_update_all.c's queue path.
// UNSURE: vector3d_pack_normal_11_11_10 is called twice with zero visible arguments and its results are stored
// into light_transient.packed_forward/packed_up; what it actually computes is not established.
// register convention: datum_index light_tag in EAX (in_EAX); real_vector3d *color in EDX
// (in_EDX); real_point3d *position on the stack (param_1); two undefined4 values on the stack
// (param_2/param_3, forwarded to vector3d_pack_normal_11_11_10 — UNSURE how); float intensity on the stack
// (param_4).
// blam-cc: EAX=light_tag, EDX=color, stack=(position, param_2, param_3, intensity)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern int16_t light_transient_count;        // 0x00860b0c
extern light_transient light_transient_table[k_maximum_transient_lights]; // 0x008609cc
extern tag_instance *tag_instances;           // 0x0087bc14
extern uint8_t render_window_index;                  // UNSURE: not owned by this module

extern uint32_t color_real_to_argb_pack(float alpha, real_vector3d *color); // 0x44da60
extern uint32_t vector3d_pack_normal_11_11_10(real_vector3d *direction); // 0x5132d0, ESI

void light_transient_add(datum_index light_tag, real_vector3d *color, real_point3d *position,
    uint32_t direction, uint32_t param_3, float intensity)
{
    if (light_transient_count < k_maximum_transient_lights &&
        (color->i != 0.0f || color->j != 0.0f || color->k != 0.0f)) {
        light_transient *slot = &light_transient_table[light_transient_count];

        slot->color = color_real_to_argb_pack(1.0f, color);
        slot->intensity = (uint8_t)(int32_t)(intensity * 255.0f + 0.5f); // UNSURE: ROUND()
        slot->definition = tag_instances[light_tag & 0xffff].data;
        slot->position = *position;
        // 0x4f16a4: ESI = the second and third stack arguments (the light's forward and up)
        slot->packed_forward = vector3d_pack_normal_11_11_10((real_vector3d *)direction);
        slot->packed_up = vector3d_pack_normal_11_11_10((real_vector3d *)param_3);
        slot->render_window_index = render_window_index;
        slot->unknown_1e = -1;
        slot->unknown_1c = -1;
        slot->slot_index = light_transient_count;

        light_transient_count = light_transient_count + 1;
    }
}

#if 0
Original Ghidra decompilation (0x4f1600):

void FUN_004f1600(undefined4 *param_1,undefined4 param_2,undefined4 param_3,float param_4)

{
  int iVar1;
  undefined1 uVar2;
  int iVar3;
  uint in_EAX;
  uint uVar4;
  undefined4 uVar5;
  float *in_EDX;
  undefined1 local_4;

  if ((DAT_00860b0c < 8) && (((*in_EDX != 0.0 || (in_EDX[1] != 0.0)) || (in_EDX[2] != 0.0)))) {
    iVar1 = DAT_00860b0c * 0x28;
    uVar4 = color_real_to_argb_pack(1.0,in_EDX);
    *(uint *)(&DAT_008609e4 + iVar1) = uVar4;
    iVar3 = DAT_0087bc14;
    local_4 = (undefined1)(int)ROUND(param_4 * 255.0);
    (&DAT_008609ef)[iVar1] = local_4;
    *(undefined4 *)(&DAT_008609cc + iVar1) =
         *(undefined4 *)((in_EAX & 0xffff) * 0x20 + 0x14 + iVar3);
    *(undefined4 *)(&DAT_008609d0 + iVar1) = *param_1;
    *(undefined4 *)(&DAT_008609d4 + iVar1) = param_1[1];
    *(undefined4 *)(&DAT_008609d8 + iVar1) = param_1[2];
    uVar5 = FUN_005132d0();
    *(undefined4 *)(&DAT_008609dc + iVar1) = uVar5;
    uVar5 = FUN_005132d0();
    uVar2 = DAT_007c310a;
    *(undefined4 *)(&DAT_008609e0 + iVar1) = uVar5;
    (&DAT_008609ee)[iVar1] = uVar2;
    *(undefined2 *)(&DAT_008609ea + iVar1) = 0xffff;
    *(undefined2 *)(&DAT_008609e8 + iVar1) = 0xffff;
    *(short *)(&DAT_008609ec + iVar1) = DAT_00860b0c;
    DAT_00860b0c = DAT_00860b0c + 1;
  }
  return;
}
#endif
