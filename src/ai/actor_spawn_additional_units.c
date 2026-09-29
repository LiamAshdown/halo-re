// actor_spawn_additional_units  (Ghidra: actor_spawn_additional_units, renamed)
// address 0x427280, size 716 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (spawn loop REWRITTEN from objdump 0x427370..0x427545)
// evidence: phase-4 summary "Spawns one or more additional units around an existing actor's
// position and attaches new actors to them, optionally randomizing their health/scale."
// types/ai.h actor.unknown_334/unknown_336 (a squad/platoon-index pair the header does not
// individually attribute here); types/units.h unit_data.swarm_actor_index (object+0x1f8) /
// actor_index (object+0x1f4). Calls actor_new_and_attach_to_unit (0x426ac0),
// actor_apply_unit_definition_properties (0x426cf0), object_placement_data_initialize /
// object_new / object_delete / object_get_position (all established), random_real_range
// (0x401050), and unit_apply_impulse/unit_find_placement_position, neither established elsewhere in this repo.
//   UNSURE: this is one of the least-confident rewrites in this pass. actor_new_and_attach_to_unit
//   is called here with only two of its twelve established parameters visible in Ghidra's
//   decompile; the remaining ten are left at whatever this function's own stack/registers
//   happen to hold, which this rewrite cannot reconstruct without objdump, so only the two
//   visible ones (reuse_existing, unit_index) are passed and the rest default to zero/none.
//   The object_placement_data_initialize call's returned value being fed to fcos/fsin (an
//   unkbyte10, i.e. an x87 register) rather than a float looks like decompiler noise from a
//   dead computation; the two trig calls are omitted here as they have no visible effect.
// register convention: EBX -> actor_variant_tag, DX -> spawn_count, stack -> source_actor_index,
//   health_scale.
//   // blam-cc: EBX -> actor_variant_tag, EDX -> spawn_count, stack -> source_actor_index,
//   //   health_scale

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0

extern real random_real_range(real min, real max); // 0x401050
extern double cos(double x);
extern double sin(double x);
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role); // 0x4f53a0
extern datum_index object_new(object_placement_data *placement); // 0x4f5460, UNSURE signature
extern void object_delete(datum_index object_index); // 0x4f5bd0, UNSURE signature
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900
extern void actor_apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index); // 0x426cf0
extern datum_index actor_new_and_attach_to_unit(
    char reuse_existing, datum_index unit_index, datum_index actor_variant_tag,
    uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor,
    char start_active, uint16_t unknown_60, int16_t unknown_62, uint16_t command_list_index, uint8_t unknown_68); // 0x426ac0
extern uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object, real_point3d *out_position,
    float radius, char grid_mode, char skip_reposition, char scale_radius, uint32_t object_index_a,
    real_vector3d *reference_direction); // 0x55a500, stack x7, EDX
extern void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse); // 0x559fa0, EAX, EDI

// blam-cc: EBX -> actor_variant_tag, EDX -> spawn_count, stack -> source_actor_index,
//   health_scale
// Spawns up to spawn_count additional units around source_actor's position and attaches new
// actors to them (reusing the source's squad/platoon identity when it has no live unit of
// its own, otherwise its controlling actor's), optionally randomizing scale/health when
// health_scale is positive. Returns the number of units successfully spawned and attached.
int16_t actor_spawn_additional_units(datum_index actor_variant_tag, int16_t spawn_count,
                                     datum_index source_actor_index, float health_scale)
{
    int16_t spawned = 0;

    if (actor_variant_tag == (datum_index)k_datum_index_none || spawn_count <= 0) {
        return 0;
    }

    {
        object *source_object = ((object_header *)object_data->data)[source_actor_index & 0xffff].data;
        unit_data *source_unit = (unit_data *)((uint8_t *)source_object + k_unit_data_offset);
        int16_t encounter_index, squad_index;

        if (source_unit->swarm_actor_index == (datum_index)k_datum_index_none &&
            source_unit->actor_index == (datum_index)k_datum_index_none) {
            encounter_index = *(int16_t *)((uint8_t *)source_object + 0x334); // UNSURE offset
            squad_index = *(int16_t *)((uint8_t *)source_object + 0x336); // UNSURE offset
        } else {
            // UNSURE: the original always resolves the owner through unit.actor_index here,
            // even along the branch where only swarm_actor_index was confirmed non-none;
            // preserved literally rather than "fixed".
            actor *owner_actor = &((actor *)actor_data->data)[source_unit->actor_index & 0xffff];
            encounter_index = owner_actor->encounter_index;
            squad_index = owner_actor->squad_index;
        }

        if (encounter_index == -1 || squad_index == -1) {
            return 0;
        }

        {
            const uint8_t *variant_tag_data = (const uint8_t *)(tag_instances[actor_variant_tag & 0xffff].data);
            const uint32_t *variant = (const uint32_t *)variant_tag_data;
            const uint8_t *actor_tag_data = (const uint8_t *)(tag_instances[variant[4] & 0xffff].data);
            int16_t i;

            // REWRITTEN from objdump 0x427370..0x427545. Each spawn: a random heading from the global seed
            //   (angle = (seed >> 16) / 65535 * 2pi), the placement's forward = (cos, sin, 0), the position = the
            //   source object's + 0.3 * forward + 0.3 up. A unit (object +0xb4 dword 0) is then placed with
            //   unit_find_placement_position(unit, -1, 0, 1.0, 1, 0, 0; EDX = that position). The actor attach passes
            //   (reuse, unit, variant, encounter, squad, 0, -1, 0, 2, 0, -1, 0). With health_scale > 0 a unit gets the
            //   impulse (forward.i * r1, forward.j * r1, r2) * health_scale, with r1 in [0.5, 1] and r2 in [0.8, 1.5].
            for (i = 0; i < spawn_count; i++) {
                object_placement_data placement;
                datum_index new_object;
                float angle;
                uint32_t random_bits;

                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                random_bits = random_seed_global >> 16;
                angle = (float)(int32_t)random_bits * 1.5259022e-05f * 6.2831855f;
                object_placement_data_initialize(&placement, (datum_index)variant[8], (datum_index)k_datum_index_none);
                placement.forward.i = (float)cos(angle);
                placement.forward.j = (float)sin(angle);
                placement.forward.k = 0.0f;
                object_get_position(&placement.position, source_actor_index);
                placement.position.x = placement.forward.i * 0.3f + placement.position.x;
                placement.position.y = placement.forward.j * 0.3f + placement.position.y;
                placement.position.z = placement.forward.k * 0.3f + (placement.position.z + 0.3f);

                new_object = object_new(&placement);
                if (new_object == (datum_index)k_datum_index_none) {
                    continue;
                }
                {
                    uint8_t *new_obj = (uint8_t *)((object_header *)object_data->data)[new_object & 0xffff].data;
                    char reuse_existing = (char)((*(const uint32_t *)actor_tag_data >> 0x1a) & 1);
                    datum_index new_actor;

                    if (*(uint32_t *)&((object *)new_obj)->type == 0) {
                        unit_find_placement_position(new_object, 0xffffffff, 0, 1.0f, 1, 0, 0, 0,
                            (real_vector3d *)&placement.position);
                    }
                    actor_apply_unit_definition_properties(actor_variant_tag, new_object);
                    new_actor = actor_new_and_attach_to_unit(reuse_existing, new_object, actor_variant_tag,
                        (uint32_t)(int32_t)encounter_index, squad_index, 0, (datum_index)k_datum_index_none, 0, 2, 0,
                        0xffff, 0);
                    if (new_actor == (datum_index)k_datum_index_none) {
                        object_delete(new_object);
                        continue;
                    }
                    if (health_scale > 0.0f) {
                        real_vector3d impulse;
                        float r1 = random_real_range(0.5f, 1.0f);
                        float r2;

                        impulse.i = placement.forward.i * r1;
                        impulse.j = placement.forward.j * r1;
                        r2 = random_real_range(0.8f, 1.5f);
                        impulse.i = impulse.i * health_scale;
                        impulse.j = impulse.j * health_scale;
                        impulse.k = r2 * health_scale;
                        if (*(uint32_t *)&((object *)new_obj)->type == 0) {
                            unit_apply_impulse(new_object, &impulse);
                        }
                    }
                    spawned = spawned + 1;
                }
            }
        }
    }

    return spawned;
}

#if 0
Original Ghidra decompilation (0x427280):

short FUN_00427280(uint param_1,float param_2)

{
  uint *puVar1;
  int iVar2;
  short sVar3;
  short sVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  short sVar8;
  ushort in_DX;
  uint unaff_EBX;
  short sVar9;
  unkbyte10 Var10;
  uint local_a4;

  sVar9 = 0;
  sVar3 = 0;
  if ((unaff_EBX != 0xffffffff) && (0 < (short)in_DX)) {
    iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    if ((*(int *)(iVar5 + 0x1f8) == -1) && (*(int *)(iVar5 + 500) == -1)) {
      sVar8 = *(short *)(iVar5 + 0x334);
      sVar4 = *(short *)(iVar5 + 0x336);
    }
    else {
      iVar5 = (*(uint *)(iVar5 + 500) & 0xffff) * 0x724;
      sVar8 = *(short *)(iVar5 + 0x34 + *(int *)(DAT_00880360 + 0x34));
      sVar4 = *(short *)(iVar5 + *(int *)(DAT_00880360 + 0x34) + 0x3a);
    }
    if ((sVar8 != -1) && (sVar4 != -1)) {
      iVar5 = *(int *)((unaff_EBX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      puVar1 = *(uint **)((*(uint *)(iVar5 + 0x10) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (0 < (short)in_DX) {
        local_a4 = (uint)in_DX;
        do {
          random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
          Var10 = object_placement_data_initialize(*(undefined4 *)(iVar5 + 0x20),0xffffffff);
          fcos(Var10);
          fsin(Var10);
          object_get_position();
          uVar6 = object_new();
          if (uVar6 != 0xffffffff) {
            iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
            if (*(short *)(iVar2 + 0xb4) == 0) {
              FUN_0055a500(uVar6,0xffffffff,0,0x3f800000,1,0,0);
            }
            FUN_00426cf0(uVar6);
            iVar7 = actor_new_and_attach_to_unit(*puVar1 >> 0x1a & 0xffffff01,uVar6);
            if (iVar7 == -1) {
              object_delete();
              sVar9 = sVar3;
            }
            else {
              if (0.0 < param_2) {
                random_real_range(0.5,1.0);
                random_real_range(0.8,1.5);
                if (*(short *)(iVar2 + 0xb4) == 0) {
                  FUN_00559fa0();
                }
              }
              sVar9 = sVar3 + 1;
              sVar3 = sVar9;
            }
          }
          local_a4 = local_a4 - 1;
        } while (local_a4 != 0);
      }
    }
  }
  return sVar9;
}
#endif
