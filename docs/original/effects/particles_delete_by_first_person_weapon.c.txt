// particles_delete_by_first_person_weapon  (Ghidra: FUN_00455c80, still unnamed there; named
//   directly by types/effects.h: "particles_delete_by_first_person_weapon 0x455c80")
// address 0x455c80, size 147 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: types/effects.h particle.first_person_weapon_index (+0x0f), flags
//   (_particle_first_person_bit), object_index (+0x08).
// register convention: first person weapon row index in BX (unaff_BX).
//   // blam-cc: unaff_BX -> first_person_weapon_index
// UNSURE: the tail of the original decompile inlines datum_next 0x4d0630's own body instead of
//   calling it, exactly as src/effects/contrail_update.c documents for the same pattern; this
//   rewrite calls datum_next directly since it is semantically identical.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *particle_data; // 0x0087abd0

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module

// Deletes every first person, object-attached particle whose first_person_weapon_index matches
// the given row -- used when a first person weapon slot is being released.
void particles_delete_by_first_person_weapon(uint8_t first_person_weapon_index)
{
    datum_index particle_index = datum_next(-1, particle_data);

    while (particle_index != k_datum_index_none) {
        particle *self = &((particle *)particle_data->data)[(uint16_t)particle_index];

        if (self->first_person_weapon_index == first_person_weapon_index &&
            (self->flags & _particle_first_person_bit) != 0 &&
            self->object_index != k_datum_index_none) {
            datum_delete(particle_data, particle_index);
        }

        particle_index = datum_next((int16_t)particle_index, particle_data);
    }
}

#if 0
Original Ghidra decompilation (0x455c80):

void FUN_00455c80(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  short *psVar5;
  short sVar6;
  ushort unaff_BX;

  iVar1 = DAT_0087abd0;
  uVar2 = datum_next();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      iVar3 = (uVar2 & 0xffff) * 0x70;
      iVar4 = iVar3 + *(int *)(iVar1 + 0x34);
      if (((*(byte *)(iVar3 + 0xf + *(int *)(iVar1 + 0x34)) == unaff_BX) &&
          ((*(byte *)(iVar4 + 2) & 0x40) != 0)) && (*(int *)(iVar4 + 8) != -1)) {
        datum_delete();
      }
      iVar3 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar6 = (short)iVar3;
    } while ((sVar6 < 0) || (*(short *)(iVar1 + 0x2e) <= sVar6));
    psVar5 = (short *)((int)sVar6 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
    do {
      if (*psVar5 != 0) {
        uVar2 = (int)*psVar5 << 0x10 | (int)(short)iVar3;
        break;
      }
      iVar3 = iVar3 + 1;
      psVar5 = (short *)((int)psVar5 + (int)*(short *)(iVar1 + 0x22));
    } while ((short)iVar3 < *(short *)(iVar1 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
