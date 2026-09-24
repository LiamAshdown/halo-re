// particle_system_resolve_local_players  (Ghidra: FUN_00454080, still unnamed there; named
//   directly by types/effects.h: "particle_system_resolve_local_players 0x454080 (location
//   only)")
// address 0x454080, size 456 bytes
// name confidence: 0.4   rewrite confidence: 0.2 (LOW -- dead code, callers=0)
// evidence: out/phase4/effects_functions.md: "Unused routine that would re-resolve local-player
//   associations for every particle system and its particle-type slots" (0 callers in this
//   batch); types/effects.h particle_system.location (+0x18), object_index (+0x0c),
//   type_states[4] (+0x58, particle_system_type_state.first_particle at +0x3c, matching this
//   function's +0x94 = +0x58 + 0*0x40 + 0x3c); particle_system_particle.location (+0x14),
//   next_particle (+0x04); types/tags.h ParticleSystem.particle_types (TagReflexive, +0x5c).
// register convention: __cdecl, no arguments.
// UNSURE (lower rigor -- dead code): object_get_root_location, called for an object-attached system in
//   place of the free-standing system's leaf/cluster probe, is not established anywhere in this
//   batch (0 callees resolved); declared opaque and called with no arguments, matching the
//   decompile. The tail of the original decompile inlines datum_next 0x4d0630's own body
//   instead of calling it, exactly as src/effects/contrail_update.c documents for the same
//   pattern; this rewrite calls datum_next directly since it is semantically identical.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *particle_system_data;          // 0x0087abd4
extern data_array *particle_system_particle_data; // 0x0087abd8
extern tag_instance *tag_instances;                // 0x0087bc14
extern void *global_globals;                       // 0x00746f90, passed to FUN_005013a0 in ECX
extern uint8_t *structure_bsp_globals;             // 0x00746f9c; +0xe4 is the per-leaf lookup table

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module
extern void particle_system_delete(datum_index particle_system_handle); // 0x453f60, this module
extern void object_get_root_location(void); // 0x4f6b10, module unresolved (objects?); UNSURE, dead code
extern int32_t FUN_005013a0(void *globals, real_point3d *point, int32_t index);
    // 0x5013a0; blam-cc: ECX -> globals, EDX -> point, EAX -> index

// Dead code (0 callers): would re-resolve every particle system's BSP location (and, for a
// free-standing system whose new cluster fails to resolve, delete it), then walk every particle
// type slot's particle list re-resolving each particle's own location and dropping any whose
// cluster also fails to resolve.
void particle_system_resolve_local_players(void)
{
    datum_index system_index = datum_next(-1, particle_system_data);

    while (system_index != k_datum_index_none) {
        particle_system *system =
            &((particle_system *)particle_system_data->data)[(uint16_t)system_index];
        ParticleSystem *tag = (ParticleSystem *)tag_instances[(uint16_t)system->definition_index].data;
        uint8_t deleted = 0;

        if (system->object_index == k_datum_index_none) {
            int32_t leaf = FUN_005013a0(global_globals, &system->position, 0);

            system->location.leaf_index = leaf;
            system->location.cluster_index = (leaf == -1) ? -1 :
                *(int16_t *)(*(uint8_t **)(structure_bsp_globals + 0xe4) + (uint32_t)leaf * 0x10 + 8);

            if (system->location.cluster_index == -1) {
                particle_system_delete(system_index);
                deleted = 1;
            }
        } else {
            object_get_root_location(); // UNSURE, see file header
        }

        if (!deleted) {
            int32_t i;

            for (i = 0; i < (int32_t)tag->particle_types.count; i++) {
                datum_index *link = &system->type_states[i].first_particle;

                while (*link != k_datum_index_none) {
                    particle_system_particle *p =
                        &((particle_system_particle *)particle_system_particle_data->data)[(uint16_t)*link];
                    int32_t leaf = FUN_005013a0(global_globals, (real_point3d *)&p->position, 0);

                    p->location.leaf_index = leaf;
                    p->location.cluster_index = (leaf == -1) ? -1 :
                        *(int16_t *)(*(uint8_t **)(structure_bsp_globals + 0xe4) + (uint32_t)leaf * 0x10 + 8);

                    if (p->location.cluster_index == -1) {
                        datum_index next = p->next_particle;
                        datum_delete(particle_system_particle_data, *link);
                        *link = next;
                    } else {
                        link = &p->next_particle;
                    }
                }
            }
        }

        system_index = datum_next((int16_t)system_index, particle_system_data);
    }
}

#if 0
Original Ghidra decompilation (0x454080):

void FUN_00454080(void)

{
  uint uVar1;
  int iVar2;
  short sVar3;
  uint uVar4;
  int iVar5;
  short *psVar6;
  short sVar7;
  int iVar8;
  uint *puVar9;
  int iVar10;
  int iVar11;

  uVar4 = datum_next();
  iVar2 = DAT_0087abd8;
  do {
    if (uVar4 == 0xffffffff) {
      return;
    }
    iVar11 = (uVar4 & 0xffff) * 0x158 + *(int *)(DAT_0087abd4 + 0x34);
    iVar8 = *(int *)((*(uint *)(iVar11 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar11 + 0xc) == -1) {
      iVar5 = FUN_005013a0();
      *(int *)(iVar11 + 0x18) = iVar5;
      if (iVar5 == -1) {
        sVar7 = -1;
      }
      else {
        sVar7 = *(short *)(iVar5 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      }
      *(short *)(iVar11 + 0x1c) = sVar7;
      if (sVar7 != -1) goto LAB_004540f2;
      particle_system_delete_453f60(uVar4);
    }
    else {
      FUN_004f6b10();
LAB_004540f2:
      iVar5 = 0;
      sVar7 = 0;
      if (0 < *(int *)(iVar8 + 0x5c)) {
        do {
          puVar9 = (uint *)(iVar5 * 0x40 + 0x94 + iVar11);
          uVar1 = *puVar9;
          while (uVar1 != 0xffffffff) {
            iVar10 = (*puVar9 & 0xffff) * 0x80 + *(int *)(iVar2 + 0x34);
            iVar5 = FUN_005013a0();
            *(int *)(iVar10 + 0x14) = iVar5;
            if (iVar5 == -1) {
              sVar3 = -1;
            }
            else {
              sVar3 = *(short *)(iVar5 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
            }
            *(short *)(iVar10 + 0x18) = sVar3;
            if (sVar3 == -1) {
              datum_delete();
              *puVar9 = *(uint *)(iVar10 + 4);
            }
            else {
              puVar9 = (uint *)(iVar10 + 4);
            }
            uVar1 = *puVar9;
          }
          sVar7 = sVar7 + 1;
          iVar5 = (int)sVar7;
        } while (iVar5 < *(int *)(iVar8 + 0x5c));
      }
    }
    iVar8 = uVar4 + 1;
    uVar4 = 0xffffffff;
    sVar7 = (short)iVar8;
    if ((-1 < sVar7) && (sVar7 < *(short *)(DAT_0087abd4 + 0x2e))) {
      psVar6 = (short *)((int)sVar7 * (int)*(short *)(DAT_0087abd4 + 0x22) +
                        *(int *)(DAT_0087abd4 + 0x34));
      do {
        if (*psVar6 != 0) {
          uVar4 = (int)*psVar6 << 0x10 | (int)(short)iVar8;
          break;
        }
        iVar8 = iVar8 + 1;
        psVar6 = (short *)((int)psVar6 + (int)*(short *)(DAT_0087abd4 + 0x22));
      } while ((short)iVar8 < *(short *)(DAT_0087abd4 + 0x2e));
    }
  } while( true );
}
#endif
