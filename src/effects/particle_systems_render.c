// particle_systems_render  (Ghidra: FUN_00454b40, still unnamed there; not individually named by
//   out/phase4/effects_types_notes.md; named here as the per-tick driver over
//   particle_system_render 0x454bf0, matching this module's particle_systems_update /
//   particle_systems_delete_all naming pattern)
// address 0x454b40, size 176 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/effects.h particle_system.location (+0x18, cluster_index at +0x1c); this
//   module's particle_new.c and particle_system_resolve_local_players.c establish the same
//   local_player_globals+0x58 per-cluster visibility bitset test.
// register convention: __cdecl, no arguments.
// UNSURE: the tail of the original decompile inlines datum_next 0x4d0630's own body instead of
//   calling it, exactly as src/effects/contrail_update.c documents for the same pattern; this
//   rewrite calls datum_next directly since it is semantically identical.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *particle_system_data;    // 0x0087abd4
extern player_globals *local_player_globals; // 0x0087a478

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void particle_system_render(datum_index particle_system_handle); // 0x454bf0, this module

// Per-tick driver: renders every particle system whose cluster is currently visible to a local
// player.
void particle_systems_render(void)
{
    datum_index system_index = datum_next(-1, particle_system_data);

    while (system_index != k_datum_index_none) {
        particle_system *system =
            &((particle_system *)particle_system_data->data)[(uint16_t)system_index];

        if (system->location.cluster_index != -1) {
            int16_t cluster = system->location.cluster_index;
            uint32_t *visible_clusters = (uint32_t *)((uint8_t *)local_player_globals + 0x58);

            if ((visible_clusters[cluster >> 5] & (1u << (cluster & 0x1f))) != 0) {
                particle_system_render(system_index);
            }
        }

        system_index = datum_next((int16_t)system_index, particle_system_data);
    }
}

#if 0
Original Ghidra decompilation (0x454b40):

void FUN_00454b40(void)

{
  uint uVar1;
  int iVar2;
  short *psVar3;
  short sVar4;
  int iVar5;

  iVar5 = DAT_0087abd4;
  uVar1 = datum_next();
  do {
    do {
      if (uVar1 == 0xffffffff) {
        return;
      }
      iVar2 = (uVar1 & 0xffff) * 0x158 + *(int *)(iVar5 + 0x34);
      if ((*(short *)(iVar2 + 0x1c) != -1) &&
         (sVar4 = *(short *)(iVar2 + 0x1c),
         (*(uint *)(DAT_0087a478 + 0x58 + ((int)sVar4 >> 5) * 4) & 1 << ((byte)sVar4 & 0x1f)) != 0))
      {
        FUN_00454bf0();
        iVar5 = DAT_0087abd4;
      }
      iVar2 = uVar1 + 1;
      uVar1 = 0xffffffff;
      sVar4 = (short)iVar2;
    } while ((sVar4 < 0) || (*(short *)(iVar5 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar5 + 0x22) + *(int *)(iVar5 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar1 = (int)*psVar3 << 0x10 | (int)(short)iVar2;
        break;
      }
      iVar2 = iVar2 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar5 + 0x22));
    } while ((short)iVar2 < *(short *)(iVar5 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
