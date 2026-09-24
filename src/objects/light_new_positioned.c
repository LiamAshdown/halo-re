// light_new_positioned
// address 0x4f0c10, size 215 bytes
// name confidence: 0.75 (out/phase4/objects_types_notes.md names this function directly:
// "light_new_positioned ... seeding its lifetime from the current tick counter")
// rewrite confidence: 0.4
// evidence: types/objects.h light (definition_tag 0x04, marker_link 0x58, next_light 0x10,
// unknown_78 0x78, marker_index 0x5c — the header's own note on the 0x5c..0x77 union between the
// attached and positioned forms; creation_tick 0x0c); the game-time tick field at
// DAT_006f1d6c+0x0c.
// UNSURE: offset 0x2c is `owner_object` in light_new_attached.c but is tested here as a marker
// index (`== -1`), which is not covered by the header's documented 0x5c..0x77 union — an
// additional overlap this pass did not have evidence to add to types/objects.h, so it is kept as
// a raw offset. The explicit-position branch's direction vector comes from an implicit `unaff_EBX`
// pointer with no corresponding parameter in Ghidra's own signature.
// register convention: datum_index light_tag on the stack (param_1); int32_t marker_index (-1
// for explicit placement) on the stack (param_2); int16_t marker_sub_index on the stack
// (param_3); real_point3d *position in EDX-sourced param_4 (stack per Ghidra); uint32_t param_5
// on the stack; real_vector3d *direction in EBX (unaff_EBX).
// blam-cc: stack=(light_tag, marker_index, marker_sub_index, position, param_5), EBX=direction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "objects.h"

extern data_array *light_data;      // 0x00860b14
extern hs_game_time_globals *game_time;         // 0x006f1d6c, game time globals; +0x0c is the current tick
extern int32_t light_frame_counter; // 0x008607c4

extern datum_index datum_new(data_array *array); // UNSURE: returns {handle, data_array*} as a
    // 64-bit pair in the original; only the handle is modeled here. memory module, 0x4d0480
extern void object_light_recompute_transform(uint32_t light_index); // this module, 0x4f2a00 (out of range)

datum_index light_new_positioned(datum_index light_tag, int32_t marker_index, int16_t marker_sub_index,
    real_point3d *position, uint32_t param_5, real_vector3d *direction)
{
    datum_index handle = datum_new(light_data);

    if (handle != (datum_index)0xffffffff) {
        light *entry = &((light *)light_data->data)[handle & 0xffff];
        uint8_t *raw = (uint8_t *)entry;

        entry->flags = 0;
        entry->marker_link = game_time->current_tick; // +0x0c, the current tick
        entry->definition_tag = light_tag;
        *(int32_t *)(raw + 0x2c) = marker_index; // UNSURE: overlaps light.owner_object
        entry->unknown_78 = param_5;
        entry->flags = 3; // _light_always_visible_bit | _light_attached_bit

        entry->next_light = (datum_index)0xffffffff;

        if (marker_index == -1) {
            *(real_point3d *)(raw + 0x30) = *position;
            *(real_vector3d *)(raw + 0x3c) = *direction;
        } else {
            *(int16_t *)(raw + 0x5c) = marker_sub_index;
            *(real_point3d *)(raw + 0x60) = *position;   // UNSURE: overlaps light.change_color_index
            *(real_vector3d *)(raw + 0x6c) = *direction;
        }

        object_light_recompute_transform(handle);
        entry->creation_tick = light_frame_counter - 1;
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x4f0c10):

uint FUN_004f0c10(undefined4 param_1,int param_2,undefined2 param_3,undefined4 *param_4,
                 undefined4 param_5)

{
  int iVar1;
  uint uVar2;
  undefined4 *unaff_EBX;
  int iVar3;
  undefined8 uVar4;

  uVar4 = datum_new();
  iVar1 = DAT_006f1d6c;
  uVar2 = (uint)uVar4;
  if (uVar2 != 0xffffffff) {
    iVar3 = (uVar2 & 0xffff) * 0x7c + *(int *)((int)((ulonglong)uVar4 >> 0x20) + 0x34);
    *(undefined2 *)(iVar3 + 2) = 0;
    *(undefined4 *)(iVar3 + 0x58) = *(undefined4 *)(iVar1 + 0xc);
    *(undefined4 *)(iVar3 + 4) = param_1;
    *(int *)(iVar3 + 0x2c) = param_2;
    *(undefined4 *)(iVar3 + 0x78) = param_5;
    *(undefined2 *)(iVar3 + 2) = 3;
    *(undefined4 *)(iVar3 + 0x10) = 0xffffffff;
    if (param_2 == -1) {
      *(undefined4 *)(iVar3 + 0x30) = *param_4;
      *(undefined4 *)(iVar3 + 0x34) = param_4[1];
      *(undefined4 *)(iVar3 + 0x38) = param_4[2];
      *(undefined4 *)(iVar3 + 0x3c) = *unaff_EBX;
      *(undefined4 *)(iVar3 + 0x40) = unaff_EBX[1];
      *(undefined4 *)(iVar3 + 0x44) = unaff_EBX[2];
    }
    else {
      *(undefined2 *)(iVar3 + 0x5c) = param_3;
      *(undefined4 *)(iVar3 + 0x60) = *param_4;
      *(undefined4 *)(iVar3 + 100) = param_4[1];
      *(undefined4 *)(iVar3 + 0x68) = param_4[2];
      *(undefined4 *)(iVar3 + 0x6c) = *unaff_EBX;
      *(undefined4 *)(iVar3 + 0x70) = unaff_EBX[1];
      *(undefined4 *)(iVar3 + 0x74) = unaff_EBX[2];
    }
    FUN_004f2a00(uVar2);
    *(int *)(iVar3 + 0xc) = DAT_008607c4 + -1;
  }
  return uVar2;
}
#endif
