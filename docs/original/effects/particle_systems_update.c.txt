// particle_systems_update  (Ghidra: FUN_00454000, still unnamed there)
// address 0x454000, size 120 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: matches src/objects/flags_update.c's manual datum_next-and-rescan idiom over
//   particle_system_data (0x0087abd4, types/memory.h data_array fields last_index +0x2e, size
//   +0x22, data +0x34); the per-tick call into particle_system_update (0x4544f0, this module)
//   confirms that function's (delta_time, handle) argument order.
// register convention: none -- delta_time is the single Ghidra-recognized stack parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *particle_system_data; // 0x0087abd4

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630
extern uint8_t particle_system_update(float delta_time, datum_index handle); // 0x4544f0, this module

void particle_systems_update(float delta_time)
{
    data_array *systems = particle_system_data;
    datum_index handle = datum_next(-1, systems);

    while (handle != (datum_index)0xffffffff) {
        particle_system_update(delta_time, handle);
        handle = datum_next((int16_t)handle, systems);
    }
}

#if 0
Original Ghidra decompilation (0x454000):

void FUN_00454000(undefined4 param_1)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  short *psVar4;

  uVar2 = datum_next();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      FUN_004544f0(param_1,uVar2);
      iVar3 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar1 = (short)iVar3;
    } while ((sVar1 < 0) || (*(short *)(DAT_0087abd4 + 0x2e) <= sVar1));
    psVar4 = (short *)((int)sVar1 * (int)*(short *)(DAT_0087abd4 + 0x22) +
                      *(int *)(DAT_0087abd4 + 0x34));
    do {
      if (*psVar4 != 0) {
        uVar2 = (int)*psVar4 << 0x10 | (int)(short)iVar3;
        break;
      }
      iVar3 = iVar3 + 1;
      psVar4 = (short *)((int)psVar4 + (int)*(short *)(DAT_0087abd4 + 0x22));
    } while ((short)iVar3 < *(short *)(DAT_0087abd4 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
