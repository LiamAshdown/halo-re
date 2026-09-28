// breakable_surface_damage_in_blast_radius  (Ghidra: FUN_004fff20, still unnamed there. Phase-2
// proposed physics_point_reset_out_of_range, describing this as points moving out of an
// allowed range; the type recovery in out/phase4/physics_types_notes.md shows it is instead an
// explosion (damage_data) breaking every breakable surface of the current BSP whose distance to
// the blast origin is inside the combined blast + surface radius, so this rewrite uses the more
// accurate name.)
// address 0x4fff20, size 356 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: ScenarioStructureBSP.breakable_surfaces (TagReflexive at 0x16c/0x170) and
//   ScenarioStructureBSPBreakableSurface {centroid, radius, collision_surface_index} confirmed
//   directly against types/tags.h; breakable_surface_globals.health/active confirmed
//   field-by-field in out/phase4/physics_types_notes.md section 2; DamageEffect.radius[1] and
//   damage_data.origin used the same way as in breakable_surface_apply_damage (0x4ffde0).
// register convention: none -- the single argument is Ghidra-recognized on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"

extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78, see
                                    // breakable_surface_apply_damage.c
extern int16_t global_structure_bsp_index;      // 0x0069e8d8
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, owned by the
                                    // scenario/structures module (UNSURE name: other modules
                                    // call the same global structure_bsp_globals or
                                    // global_matg_multiplayer; +0xe4 leaves and +0x170
                                    // breakable_surfaces both land on named ScenarioStructureBSP
                                    // fields, confirming it is the tag data pointer itself).
extern tag_instance *tag_instances;             // 0x0087bc14
extern void breakable_surface_shatter(uint16_t surface_index, damage_data *damage, int32_t collision_surface_index); // 0x500090

void breakable_surface_damage_in_blast_radius(damage_data *damage)
{
    DamageEffect *effect =
        (DamageEffect *)tag_instances[(uint16_t)damage->damage_effect_tag].data;

    if ((breakable_surface_state->initialized != 0) &&
        ((effect->damage_upper_bound[0] != 0.0f) || (effect->damage_upper_bound[1] != 0.0f))) {
        float outer_radius = effect->radius[1];
        int16_t surface_index = 0;
        int32_t index = 0; // always equal to surface_index at the point it is used below;
                            // Ghidra keeps them as separate int/short SSA values
        if (0 < global_structure_bsp->breakable_surfaces.count) {
            do {
                // UNSURE: surface_index counts up from 0 and can never be -1, so this disjunct
                // never fires; kept verbatim because it is present in the decompiled binary.
                if ((surface_index == -1) ||
                    ((breakable_surface_state->active[global_structure_bsp_index][index >> 5] &
                      (1u << (index & 0x1f))) != 0)) {
                    ScenarioStructureBSPBreakableSurface *surface =
                        &((ScenarioStructureBSPBreakableSurface *)
                              global_structure_bsp->breakable_surfaces.pointer)[index];
                    float combined_radius = outer_radius + surface->radius;
                    float dy = damage->origin.y - surface->centroid.y;
                    float dz = damage->origin.z - surface->centroid.z;
                    float dx = damage->origin.x - surface->centroid.x;
                    if (dy * dy + dz * dz + dx * dx <= combined_radius * combined_radius) {
                        breakable_surface_state->health[global_structure_bsp_index][index] = 0.0f;
                        breakable_surface_state->active[global_structure_bsp_index][index >> 5] &=
                            ~(1u << (index & 0x1f));
                        breakable_surface_shatter((uint16_t)surface_index, damage,
                                                            surface->collision_surface_index);
                    }
                }
                surface_index = surface_index + 1;
                index = (int32_t)surface_index;
            } while (index < global_structure_bsp->breakable_surfaces.count);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fff20):

void FUN_004fff20(uint *param_1)

{
  char *pcVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  float *pfVar8;
  char *pcVar9;

  iVar4 = DAT_00746f9c;
  iVar7 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*DAT_006b8d78 != '\0') &&
     ((*(float *)(iVar7 + 0x1d4) != 0.0 || (*(float *)(iVar7 + 0x1d8) != 0.0)))) {
    fVar2 = *(float *)(iVar7 + 4);
    iVar7 = 0;
    if (0 < *(int *)(DAT_00746f9c + 0x16c)) {
      iVar6 = 0;
      pcVar9 = DAT_006b8d78;
      do {
        if ((((short)iVar7 == -1) ||
            ((*(uint *)(pcVar9 + ((iVar6 >> 5) + DAT_0069e8d8 * 8) * 4 + 1) &
             1 << ((byte)iVar6 & 0x1f)) != 0)) &&
           (iVar5 = *(int *)(iVar4 + 0x170), fVar3 = fVar2 + *(float *)(iVar6 * 0x30 + 0xc + iVar5),
           pfVar8 = (float *)(iVar6 * 0x30 + iVar5),
           ((float)param_1[0xb] - pfVar8[1]) * ((float)param_1[0xb] - pfVar8[1]) +
           ((float)param_1[0xc] - pfVar8[2]) * ((float)param_1[0xc] - pfVar8[2]) +
           ((float)param_1[10] - *pfVar8) * ((float)param_1[10] - *pfVar8) <= fVar3 * fVar3)) {
          iVar5 = (int)DAT_0069e8d8;
          pcVar1 = pcVar9 + (iVar5 * 0x100 + iVar6) * 4 + 0x204;
          pcVar1[0] = '\0';
          pcVar1[1] = '\0';
          pcVar1[2] = '\0';
          pcVar1[3] = '\0';
          *(uint *)(pcVar9 + ((iVar6 >> 5) + iVar5 * 8) * 4 + 1) =
               *(uint *)(pcVar9 + ((iVar6 >> 5) + iVar5 * 8) * 4 + 1) & ~(1 << ((byte)iVar6 & 0x1f))
          ;
          FUN_00500090(iVar7,param_1,pfVar8[4]);
          pcVar9 = DAT_006b8d78;
        }
        iVar7 = iVar7 + 1;
        iVar6 = (int)(short)iVar7;
      } while (iVar6 < *(int *)(iVar4 + 0x16c));
    }
  }
  return;
}
#endif
