// flags_update
// address 0x4fba00, size 210 bytes
// name confidence: 0.55 (functions.md: "Advances the cloth simulation of every active flag
//   instance by one timestep")
// rewrite confidence: 0.5
// evidence: types/memory.h data_array (size 0x22, last_index 0x2e, data 0x34); types/objects.h
//   flag (object_index 0x08, definition_tag 0x0c); this module's flag_cloth_update 0x4fbae0
//   (param_1/2/3 = entry, tag data, dt, matching this call site exactly).
// register convention: single float stack parameter (dt); Ghidra shows a clean param_1 with no
//   in_REG marker.
// blam-cc: stack -> dt
// UNSURE: the throttle counter this increments and tests (`< 5`) lives at flag+0x06, inside
//   what types/objects.h currently folds into the single `uint32_t unknown_04` field (0x04..
//   0x08); this read/write refines that to a 16-bit sub-field without redefining the header.
//   FUN_004d0630 is treated as "return the first live datum_index in this data_array, or -1",
//   by analogy with data_iterator_next (0x4d05d0, types/memory.h data_iterator); the manual
//   scan in the rest of this function matches that function's known body shape closely enough
//   that this is very likely the same logic, inlined after the first call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *flag_data;       // 0x008603a8
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index datum_next(data_array *data); // 0x4d0630, UNSURE: implicit register
    // argument, mirrors datum_new's calling shape; returns the first live datum_index or -1
extern void flag_cloth_update(flag *entry, Flag *tag, float dt); // this module, 0x4fbae0

void flags_update(float dt) // blam-cc: stack -> dt
{
    data_array *flags = flag_data;
    datum_index current = datum_next(flags);

    for (;;) {
        int32_t next_index;

        if (current == (datum_index)0xffffffff) {
            return;
        }

        {
            flag *entry = (flag *)((uint8_t *)flags->data + (current & 0xffff) * flags->size);
            datum_index object_index = entry->object_index;
            void *tag_data = tag_instances[entry->definition_tag & 0xffff].data;
            int16_t *update_counter = (int16_t *)((uint8_t *)entry + 6); // UNSURE, see file header

            *update_counter = *update_counter + 1;
            if (object_index != (datum_index)0xffffffff && *update_counter < 5 && dt != 0.0f) {
                flag_cloth_update(entry, (Flag *)tag_data, dt);
                flags = flag_data; // reloaded in case the call touched the global
            }
        }

        next_index = (int32_t)(int16_t)((current & 0xffff) + 1);
        current = (datum_index)0xffffffff;
        if (next_index < 0 || flags->last_index <= next_index) {
            return; // the original loops back here only to hit the uVar2==-1 check above and
                    // return immediately; collapsed to a direct return
        }

        {
            int16_t *identifier = (int16_t *)((uint8_t *)flags->data + next_index * flags->size);
            int32_t i = next_index;

            do {
                if (*identifier != 0) {
                    current = (datum_index)(((uint32_t)(uint16_t)*identifier << 16) | (uint16_t)i);
                    break;
                }
                i = i + 1;
                identifier = (int16_t *)((uint8_t *)identifier + flags->size);
            } while ((int16_t)i < flags->last_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fba00):

void flags_update(float param_1)

{
  undefined4 uVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int iVar7;

  iVar7 = DAT_008603a8;
  uVar2 = FUN_004d0630();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      iVar5 = (uVar2 & 0xffff) * 0x16bc;
      iVar6 = *(int *)(iVar5 + 8 + *(int *)(iVar7 + 0x34));
      iVar5 = iVar5 + *(int *)(iVar7 + 0x34);
      uVar1 = *(undefined4 *)((*(uint *)(iVar5 + 0xc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      *(short *)(iVar5 + 6) = *(short *)(iVar5 + 6) + 1;
      if (((iVar6 != -1) && (*(short *)(iVar5 + 6) < 5)) && (param_1 != 0.0)) {
        flag_cloth_update(iVar5,uVar1,param_1);
        iVar7 = DAT_008603a8;
      }
      iVar6 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar6;
    } while ((sVar4 < 0) || (*(short *)(iVar7 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar7 + 0x22) + *(int *)(iVar7 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar6;
        break;
      }
      iVar6 = iVar6 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar7 + 0x22));
    } while ((short)iVar6 < *(short *)(iVar7 + 0x2e));
  } while( true );
}
#endif
