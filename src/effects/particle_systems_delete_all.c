// particle_systems_delete_all  (Ghidra: FUN_004535b0, still unnamed there)
// address 0x4535b0, size 73 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: types/memory.h data_array.valid (+0x24); calls the real particle_system_delete
//   (0x453f60, this module) over every live particle_system_data datum via datum_next, matching
//   functions.md's "deletes every active particle system, used to fully reset the particle
//   system pool." Confirmed against objdump -d -M intel, 0x4535b0..0x4535f8: EDI (the
//   particle_system_data array argument to datum_next) is loaded once at the top and never
//   reloaded inside the loop; only the final particle_system_particle_data valid-flag clear
//   reads its global fresh, right after the loop ends.
// register convention: none -- no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *particle_system_data;          // 0x0087abd4
extern data_array *particle_system_particle_data; // 0x0087abd8

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630
extern void particle_system_delete(datum_index handle); // 0x453f60, this module

void particle_systems_delete_all(void)
{
    data_array *systems = particle_system_data;

    if (systems != (data_array *)0 && systems->valid != 0) {
        datum_index handle = datum_next(-1, systems);

        while (handle != (datum_index)0xffffffff) {
            particle_system_delete(handle);
            handle = datum_next((int16_t)handle, systems);
        }

        systems->valid = 0;
        particle_system_particle_data->valid = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4535b0):

void FUN_004535b0(void)

{
  int iVar1;
  int iVar2;
  int iVar3;

  iVar2 = DAT_0087abd4;
  if ((DAT_0087abd4 != 0) && (*(char *)(DAT_0087abd4 + 0x24) != '\0')) {
    iVar3 = datum_next();
    iVar1 = DAT_0087abd8;
    while (DAT_0087abd8 = iVar1, iVar3 != -1) {
      particle_system_delete_453f60(iVar3);
      iVar3 = datum_next();
      iVar1 = DAT_0087abd8;
    }
    *(undefined1 *)(iVar2 + 0x24) = 0;
    *(undefined1 *)(iVar1 + 0x24) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
