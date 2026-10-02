// particle_system_resolve_local_players  (Ghidra: FUN_00454080; really the particle systems' structure bsp activate
//   proc, slot 5 of structure_bsp_activate_procedures 0x69e8dc)
// address 0x454080, size 456 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN from objdump 0x454080..0x45424e (the draft was a guess at a "dead" routine; it runs on every bsp
//   switch). For every particle system (0x87abd4, 0x158 bytes): an attached one (+0x0c) takes its object's root
//   location (+0x18); a free one finds its leaf/cluster from its position (+0x20), and is deleted when outside the
//   new bsp. Then each particle type's list (tag +0x5c types; heads at +0x94 + 0x40 * type, particles 0x80 bytes in
//   0x87abd8, next +0x04) re-resolves every particle's location (+0x14 leaf, +0x18 cluster from +0x1c) and unlinks
//   and deletes the ones that fall outside.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "structures.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *particle_system_data;               // 0x0087abd4
extern data_array *particle_system_particle_data;      // 0x0087abd8
extern tag_instance *tag_instances;                    // 0x0087bc14
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;

extern datum_index datum_next(int16_t index, data_array *array); // 0x4d0630, DX, EDI
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, EAX, EDX
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX, ECX, EDX
extern void object_get_root_location(int32_t *out, uint32_t object_index); // 0x4f6b10, EAX, ECX
extern void particle_system_delete(datum_index handle); // 0x453f60

static int16_t particle_leaf_cluster(uint32_t leaf)
{
    if (leaf == 0xffffffff) {
        return -1;
    }
    return *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8);
}

void particle_system_resolve_local_players(void)
{
    datum_index handle;

    for (handle = datum_next(-1, particle_system_data); handle != k_datum_index_none;
         handle = datum_next((int16_t)handle, particle_system_data)) {
        uint8_t *system = (uint8_t *)particle_system_data->data + (handle & 0xffff) * 0x158;
        uint8_t *definition = (uint8_t *)tag_instances[((particle_system *)system)->definition_index & 0xffff].data;
        int32_t type_index;

        if (((particle_system *)system)->object_index != k_datum_index_none) {
            object_get_root_location((int32_t *)(system + 0x18), ((particle_system *)system)->object_index);
        } else {
            uint32_t leaf = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)(system + 0x20));
            int16_t cluster = particle_leaf_cluster(leaf);

            *(uint32_t *)&((particle_system *)system)->location.leaf_index = leaf;
            ((particle_system *)system)->location.cluster_index = cluster;
            if (cluster == -1) {
                particle_system_delete(handle);
                continue;
            }
        }
        for (type_index = 0; type_index < *(int32_t *)(definition + 0x5c); type_index++) {
            datum_index *link = (datum_index *)(system + 0x94 + type_index * 0x40);

            while (*link != k_datum_index_none) {
                uint8_t *particle = (uint8_t *)particle_system_particle_data->data + (*link & 0xffff) * 0x80;
                uint32_t leaf = bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)(particle + 0x1c));
                int16_t cluster = particle_leaf_cluster(leaf);

                *(uint32_t *)&((particle_system_particle *)particle)->location.leaf_index = leaf;
                ((particle_system_particle *)particle)->location.cluster_index = cluster;
                if (cluster == -1) {
                    datum_index doomed = *link;

                    datum_delete(particle_system_particle_data, doomed);
                    *link = ((particle_system_particle *)particle)->next_particle;
                } else {
                    link = (datum_index *)(particle + 4);
                }
            }
        }
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
