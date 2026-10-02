// glow_particle_datum_new
// address 0x4fdde0, size 84 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fdde0 |
//   lightning_shard_datum_new | glow_particle_datum_new")
// rewrite confidence: 0.6
// evidence: types/objects.h globals list (glow_particle_data 0x008603a4), glow_particle
//   (handle 0x04); types/memory.h data_array (size 0x22, last_index 0x2e, data 0x34).
// register convention: none (no parameters); return value only.
// UNSURE: datum_new is modeled as returning just the handle (not the {handle, data_array*}
//   pair the original 64-bit return actually carries), matching light_new_attached.c's
//   established simplification; the array base is taken from glow_particle_data directly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *glow_particle_data; // 0x008603a4
extern datum_index datum_new(data_array *array); // memory module, 0x4d0480

glow_particle *glow_particle_datum_new(void)
{
    datum_index handle = datum_new(glow_particle_data);
    glow_particle *entry = 0;

    if (handle != (datum_index)0xffffffff) {
        int16_t index = (int16_t)handle;

        if (index >= 0 && index < glow_particle_data->last_index) {
            entry = (glow_particle *)((uint8_t *)glow_particle_data->data +
                                       glow_particle_data->size * index);
            if (entry->identifier == 0 ||
                ((int16_t)(handle >> 16) != 0 && (int16_t)(handle >> 16) != entry->identifier)) {
                entry = 0;
            }
        }
    }

    if (entry != 0) {
        entry->handle = handle;
    } else {
        // The original writes the packed datum_index to offset +4 of a NULL-based pointer
        // (i.e. absolute address 4) on the failure path too; not reproduced here as it would be
        // undefined behaviour in C, and nothing reads it back on that path regardless.
    }
    return entry;
}

#if 0
Original Ghidra decompilation (0x4fdde0):

int FUN_004fdde0(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  undefined8 uVar5;

  uVar5 = datum_new();
  iVar3 = (int)((ulonglong)uVar5 >> 0x20);
  if ((int)uVar5 == -1) {
    return 0;
  }
  sVar1 = (short)uVar5;
  if ((-1 < sVar1) && (sVar1 < *(short *)(iVar3 + 0x2e))) {
    iVar2 = (int)*(short *)(iVar3 + 0x22) * (int)sVar1;
    sVar1 = *(short *)(iVar2 + *(int *)(iVar3 + 0x34));
    iVar2 = iVar2 + *(int *)(iVar3 + 0x34);
    if ((sVar1 != 0) &&
       ((sVar4 = (short)((ulonglong)uVar5 >> 0x10), sVar4 == 0 || (sVar4 == sVar1))))
    goto LAB_004fde29;
  }
  iVar2 = 0;
LAB_004fde29:
  *(int *)(iVar2 + 4) = (int)uVar5;
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
