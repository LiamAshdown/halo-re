// antennas_update  (Ghidra: antennas_update, already named)
// address 0x4fad20, size 235 bytes
// name confidence: 0.55 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Advances the physics simulation of every active antenna instance
//   by one (clamped) timestep")
// rewrite confidence: 0.4
// evidence: types/objects.h antenna (unknown_06 0x06, degenerate 0x05, definition_tag 0x08,
//   object_index 0x0c); global 0x008603ac antenna_data, 0x0087bc14 tag_instances; callees
//   datum_next (established memory-module helper, this call site confirms its shape:
//   int16_t after_index, data_array *array), antenna_update_physics (0x4fae10, already
//   completed elsewhere in this session -- see src/objects/antenna_update_physics.c).
// register convention: dt is the sole stack parameter (Ghidra's own
//   "antennas_update(float param_1)").
// UNSURE: the original calls its datum-iteration helper once explicitly and then re-implements
//   the same walk inline for subsequent antennas; this rewrite calls datum_next repeatedly in a
//   plain loop instead, which is mathematically the same operation (confirmed by comparing the
//   inlined tail against datum_next's own established body) rather than duplicating the walk a
//   second time. The per-antenna "grace period" counter at antenna+0x06 (compared against 5) is
//   preserved as a raw increment with no established meaning beyond types/objects.h's
//   unknown_06.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *antenna_data; // 0x008603ac
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index datum_next(int16_t after_index, data_array *array); // memory module, 0x4d0630
extern void antenna_update_physics(antenna *ant, Antenna *antenna_tag, float dt); // 0x4fae10

void antennas_update(float dt)
{
    datum_index handle = datum_next(-1, antenna_data);

    while (handle != k_datum_index_none) {
        antenna *ant = (antenna *)antenna_data->data + (handle & 0xffff);

        if (ant->degenerate == 0) {
            ant->update_counter = ant->update_counter + 1;
            if ((ant->object_index != k_datum_index_none) && (ant->update_counter < 5)) {
                Antenna *tag = (Antenna *)tag_instances[ant->definition_tag & 0xffff].data;
                float clamped_dt = (dt <= 0.06666667f) ? dt : 0.06666667f;
                antenna_update_physics(ant, tag, clamped_dt);
            }
        }

        handle = datum_next((int16_t)handle, antenna_data);
    }
}

#if 0
Original Ghidra decompilation (0x4fad20):

void antennas_update(float param_1)

{
  undefined4 uVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  float local_4;

  iVar6 = DAT_008603ac;
  uVar2 = FUN_004d0630();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      iVar5 = (uVar2 & 0xffff) * 700 + *(int *)(iVar6 + 0x34);
      uVar1 = *(undefined4 *)((*(uint *)(iVar5 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(char *)(iVar5 + 5) == '\0') {
        *(short *)(iVar5 + 6) = *(short *)(iVar5 + 6) + 1;
        if ((*(int *)(iVar5 + 0xc) != -1) && (*(short *)(iVar5 + 6) < 5)) {
          if (param_1 <= 0.06666667) {
            local_4 = param_1;
          }
          else {
            local_4 = 0.06666667;
          }
          antenna_update_physics(iVar5,uVar1,local_4);
          iVar6 = DAT_008603ac;
        }
      }
      iVar5 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar5;
    } while ((sVar4 < 0) || (*(short *)(iVar6 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar6 + 0x22) + *(int *)(iVar6 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar6 + 0x22));
    } while ((short)iVar5 < *(short *)(iVar6 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
