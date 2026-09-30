// unit_is_area_clear_of_fast_objects  (Ghidra: FUN_00575c50; renamed from the phase2 proposal)
// address 0x575c50, size 473 bytes
// name confidence: 0.25 (phase2 proposal at 0.25, matches functions.md summary)
// rewrite confidence: 0.3 -- Ghidra's own stack-frame sizing for the tracked-position array
//   (afStackY_6001c[65535], auStackY_20020[32761]) is clearly a decompiler artifact, not a real
//   65535-entry buffer; the real bound is the local player count (types/units.h documents
//   0x0087a478's count at +0x0c, handles from +0x04), which this rewrite caps at a small fixed
//   size.
// evidence: types/objects.h object.parent_object (0x11c), .bounding_center (0x0a0),
//   .velocity (0x068); types/objects.h object_iterator (0x0c: type_mask, flags_mask,
//   unknown_05, index, handle); types/units.h globals note "0x0087a480 data_array *player_data
//   (players module, stride 0x200)" and "0x0087a478 the local player globals".
// register convention: none visible (void); operates on the global local-player list.
// UNSURE: the local-player iteration's own bound check (`sVar10 < 1`) suggests only a single
//   local player slot is ever considered despite the loop shape; reproduced literally.
// UNSURE: the proximity test's sense (skipping objects whose parent_object == -1, i.e. only
//   objects that ARE attached to something count against "clear") is preserved as decompiled
//   even though it reads unusually for an "area is clear" check.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_units.h"

extern data_array *object_data;   // 0x008603b0
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;   // 0x0087a480, stride 0x200

extern object * object_iterator_next(object_iterator *iterator); // 0x4f6f20

#define K_MAX_TRACKED_UNITS 4 // UNSURE bound, see file header

// Resolves a small globally tracked list of parentless unit objects and returns whether no
// other nearby (within 100 units, per the squared-distance test) attached object with
// significant velocity exists near any of them.
uint8_t unit_is_area_clear_of_fast_objects(void)
{
    real_point3d tracked_positions[K_MAX_TRACKED_UNITS];
    int32_t tracked_count = 0;
    int16_t slot = -1;
    uint8_t result = 1;

    if (*(int32_t *)local_player_globals->local_players != -1) {
        slot = 0;
    }

    while (slot != -1) {
        if (slot >= 0 && slot < 1) {
            uint32_t player_handle = *(uint32_t *)&local_player_globals->local_players[slot];
            if (player_handle != 0xffffffff) {
                uint32_t unit_handle = *(uint32_t *)((uint8_t *)player_data->data +
                                                      (player_handle & 0xffff) * 0x200 + 0x34);
                if (unit_handle != 0xffffffff) {
                    object *obj = ((object_header *)object_data->data)[unit_handle & 0xffff].data;
                    if (obj->parent_object == k_datum_index_none && tracked_count < K_MAX_TRACKED_UNITS) {
                        tracked_positions[tracked_count] = obj->bounding_center;
                        tracked_count++;
                    }
                }
            }
        }
        slot = (*(int32_t *)local_player_globals->local_players != -1 && slot < 0) ? 0 : -1;
    }

    if (tracked_count != 0) {
        object_iterator iter = {0};
        iter.type_mask = 0x86868686; // UNSURE: literal mask reproduced as-is
        iter.handle = k_datum_index_none;
        object *obj;

        while ((obj = object_iterator_next(&iter)) != 0) {
            int32_t i;
            for (i = 0; i < tracked_count; i++) {
                if (obj->parent_object != k_datum_index_none) {
                    float dx = obj->bounding_center.x - tracked_positions[i].x;
                    float dy = obj->bounding_center.y - tracked_positions[i].y;
                    float dz = obj->bounding_center.z - tracked_positions[i].z;
                    if (dx * dx + dy * dy + dz * dz < 100.0f &&
                        obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                        obj->velocity.i * obj->velocity.i >= 0.0011111111f) {
                        result = 0;
                        return !result;
                    }
                }
            }
        }
    }
    return !result;
}

#if 0
Original Ghidra decompilation (0x575c50):

bool FUN_00575c50(void)

{
  uint uVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  bool bVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  short sVar10;
  float afStackY_6001c [65535];
  uint auStackY_20020 [32761];
  uint uStack_20;
  float local_1c [4];
  undefined1 local_c;
  undefined2 local_a;
  int local_8;
  undefined4 local_4;

  sVar5 = 0;
  bVar6 = true;
  sVar10 = -1;
  if (*(int *)(DAT_0087a478 + 4) != -1) {
    sVar10 = 0;
  }
  if (sVar10 != -1) {
    do {
      if ((((sVar10 != -1) && (sVar10 < 1)) &&
          (uVar1 = *(uint *)(DAT_0087a478 + 4 + sVar10 * 4), uVar1 != 0xffffffff)) &&
         ((uVar1 = *(uint *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)),
          uVar1 != 0xffffffff &&
          (iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc),
          *(int *)(iVar9 + 0x11c) == -1)))) {
        iVar8 = (int)sVar5;
        (&uStack_20)[iVar8] = uVar1;
        local_1c[iVar8 * 3] = *(float *)(iVar9 + 0xa0);
        fVar2 = *(float *)(iVar9 + 0xa8);
        local_1c[iVar8 * 3 + 1] = *(float *)(iVar9 + 0xa4);
        local_1c[iVar8 * 3 + 2] = fVar2;
        sVar5 = sVar5 + 1;
      }
      sVar7 = -1;
      if ((*(int *)(DAT_0087a478 + 4) != -1) && (sVar10 < 0)) {
        sVar7 = 0;
      }
      sVar10 = sVar7;
    } while (sVar10 != -1);
    if (sVar5 != 0) {
      local_4 = 0x86868686;
      local_1c[3] = 2.8026e-45;
      local_c = 0;
      local_a = 0;
      local_8 = -1;
      while (iVar9 = object_iterator_next(local_1c + 3), iVar9 != 0) {
        sVar10 = 0;
        if (0 < sVar5) {
          do {
            iVar8 = (int)sVar10;
            if (((*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                   ((&uStack_20)[iVar8] & 0xffff) * 0xc) + 0x11c) != local_8) &&
                (fVar2 = *(float *)(iVar9 + 0xa0) - local_1c[iVar8 * 3],
                fVar4 = *(float *)(iVar9 + 0xa4) - local_1c[iVar8 * 3 + 1],
                fVar3 = *(float *)(iVar9 + 0xa8) - local_1c[iVar8 * 3 + 2],
                fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3 < 100.0)) &&
               (0.0011111111 <=
                *(float *)(iVar9 + 0x70) * *(float *)(iVar9 + 0x70) +
                *(float *)(iVar9 + 0x6c) * *(float *)(iVar9 + 0x6c) +
                *(float *)(iVar9 + 0x68) * *(float *)(iVar9 + 0x68))) {
              bVar6 = false;
              goto LAB_00575e17;
            }
            sVar10 = sVar10 + 1;
          } while (sVar10 < sVar5);
        }
      }
    }
  }
LAB_00575e17:
  return !bVar6;
}
#endif
