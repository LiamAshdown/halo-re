// particle_system_spawn  (Ghidra: FUN_00453b10, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "particle_system_spawn 0x453b10")
// address 0x453b10, size 1085 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN (from objdump 0x453b10..0x453f52; the draft was confidence 0.3 with opaque callee calls). Offsets:
//   system +0x04 flags (bit 1: the initial burst), +0x08 definition, +0x0c object, +0x10 attachment, +0x14 burst
//   scale, +0x18 root location (cluster word +0x1c), +0x20 position, +0x58 type states (0x40 each: state index
//   +0x00, rate +0x30, accumulator +0x34, live count +0x3a, particle list +0x3c, +0x04 fade, +0x2c limit);
//   ParticleSystemType (0x80 each at pctl +0x60): flags +0x20, initial count +0x24, initial creation physics
//   +0x54, states +0x6c (0xc0 each, creation physics +0xb0).
//   A particle system on a weapon hidden from the local player's view (type flags 0x20000/0x10000 against
//   player_weapon_locality_for_object and render_local_player_gunner_seat_visible) spawns nothing. The target
//   count is the initial count (times the burst scale +0.5 with flag 0x400) for the burst, else the live count
//   plus the whole part of dt * rate, the fraction carried in the accumulator; halved when particle systems run
//   at reduced detail (0x0069c566 == 1). Markers come from the attached object (the attachment's marker name,
//   up to 8) with the root location, falling back to the local first person weapon; a system without an object
//   uses its own position. Up to 0x80 particles are created per call, each at a random marker through the
//   creation physics table, kept when scenario_location_from_point finds a cluster. A system still under its
//   limit (+0x2c) has its +0x04 multiplied by 0.3.
// blam-cc: stack -> system, type_index, dt (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;                   // 0x008603b0
extern data_array *particle_system_particle_data; // 0x0087abd8
extern tag_instance *tag_instances;               // 0x0087bc14
extern uint8_t particle_systems_enabled;          // 0x0069c566
extern int16_t current_local_player_index;        // 0x007c3108
extern uint8_t *first_person_weapon_interfaces;      // 0x006b2d98, stride 0x1ea0
extern const real_vector3d *global_origin3d_pointer; // 0x00696714
extern random_seed effect_random_seed;             // 0x00719cd4
extern void (*particle_creation_physics_table[3])(particle_system *system, int32_t type_index,
    particle_system_particle *particle, object_marker *marker); // 0x00657444

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX, EDX
extern int32_t player_weapon_locality_for_object(datum_index weapon_object_index); // 0x453a10
extern int16_t render_local_player_gunner_seat_visible(int16_t local_player_index); // 0x50fcd0, blam-cc: EAX
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum); // 0x4f6080
extern uint32_t first_person_weapon_get_marker_data(datum_index weapon_index, const char *marker_name,
    object_marker *out, uint32_t maximum); // 0x492ad0
extern void object_get_root_location(int32_t *out, uint32_t object_index); // 0x4f6b10, blam-cc: EAX, ECX
extern void scenario_location_from_point(bsp_leaf_reference *out, real_point3d *point); // 0x53e780, ESI, EDX

static real particle_roll(void)
{
    effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
    return (real)(effect_random_seed >> 0x10) * 1.5259022e-05f;
}

void particle_system_spawn(particle_system *system_record, int32_t type_index, float dt)
{
    uint8_t *system = (uint8_t *)system_record;
    uint8_t *type_state = system + 0x58 + (int16_t)type_index * 0x40;
    uint8_t *definition = (uint8_t *)tag_instances[((struct particle_system *)system)->definition_index & 0xffff].data;
    uint8_t *type = *(uint8_t **)(definition + 0x60) + (int16_t)type_index * 0x80;
    uint8_t initial = (uint8_t)((((struct particle_system *)system)->flags >> 1) & 1);
    uint8_t *state = initial ? 0 : *(uint8_t **)(type + 0x6c) + *(int16_t *)type_state * 0xc0;
    uint32_t type_flags = *(uint32_t *)(type + 0x20);
    datum_index object_index = ((struct particle_system *)system)->object_index;
    object_marker markers[8];
    int16_t locality;
    int16_t target;
    int16_t marker_count;
    int16_t spawned;

    locality = (int16_t)player_weapon_locality_for_object(object_index);
    if (locality != 0) {
        if (type_flags & 0x20000) {
            if (locality == -1 || !render_local_player_gunner_seat_visible(current_local_player_index)) {
                return;
            }
        }
        if ((type_flags & 0x10000) && locality == 1 && render_local_player_gunner_seat_visible(current_local_player_index)) {
            return;
        }
    }

    if (initial) {
        if (type_flags & 0x400) {
            target = (int16_t)(int32_t)((double)*(int16_t *)(type + 0x24) * ((struct particle_system *)system)->scale + 0.5);
        } else {
            target = *(int16_t *)(type + 0x24);
        }
    } else {
        double amount = (double)dt * *(float *)(type_state + 0x30);
        int32_t whole = (int32_t)amount;
        double accumulated = amount - (double)whole + *(float *)(type_state + 0x34);

        target = (int16_t)(*(uint16_t *)(type_state + 0x3a) + whole);
        *(float *)(type_state + 0x34) = (float)accumulated;
        if (accumulated > 1.0) {
            target = (int16_t)(target + 1);
            *(float *)(type_state + 0x34) = (float)(accumulated - 1.0);
        }
    }
    if (particle_systems_enabled == 1) {
        target = (int16_t)(int32_t)((double)target * 0.5);
    }
    if (*(int16_t *)(type_state + 0x3a) >= target) {
        goto done;
    }

    if (object_index != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
        uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
        char *marker_name = (char *)(*(uint8_t **)&((struct Object *)object_tag)->attachments.pointer + ((struct particle_system *)system)->attachment_index * 0x48 + 0x10);

        marker_count = (int16_t)object_get_node_local_transform(object_index, marker_name, markers, 8);
        object_get_root_location((int32_t *)(system + 0x18), object_index);
        if (marker_count == 0) {
            datum_index weapon = *(datum_index *)(first_person_weapon_interfaces + current_local_player_index * 0x1ea0 + 8);

            if (weapon != k_datum_index_none) {
                marker_count = (int16_t)first_person_weapon_get_marker_data(weapon, marker_name, markers, 8);
                object_get_root_location((int32_t *)(system + 0x18), weapon);
            }
        }
    } else {
        markers[0].node_transform.position = *(real_point3d *)&((struct particle_system *)system)->position.x;
        *(real_vector3d *)((uint8_t *)&markers[0] + 0x3c) = *global_origin3d_pointer;
        marker_count = 1;
    }

    if (((struct particle_system *)system)->location.cluster_index == -1 || *(int16_t *)(type_state + 0x3a) >= target) {
        goto done;
    }
    for (spawned = 0; marker_count != 0 && spawned < 0x80; ) {
        datum_index handle = datum_new(particle_system_particle_data);
        uint8_t *particle;
        int16_t physics;
        int16_t marker_index;

        if (handle == k_datum_index_none) {
            break;
        }
        particle = (uint8_t *)particle_system_particle_data->data + (handle & 0xffff) * 0x80;
        physics = initial ? *(int16_t *)(type + 0x54) : *(int16_t *)(state + 0xb0);
        ((struct particle_system_particle *)particle)->state_index = -1;
        ((struct particle_system_particle *)particle)->next_state_index = -1;
        particle[3] = 1;
        particle[2] = 1;
        ((struct particle_system_particle *)particle)->frame = -1.0f;
        ((struct particle_system_particle *)particle)->rotation = particle_roll() * 6.2831855f; // 0x672c20
        effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
        marker_index = (int16_t)(((effect_random_seed >> 0x10) * (uint32_t)(int32_t)marker_count) >> 0x10);
        particle_creation_physics_table[physics](system_record, type_index, (particle_system_particle *)particle,
            &markers[marker_index]);
        scenario_location_from_point((bsp_leaf_reference *)(particle + 0x14), (real_point3d *)(particle + 0x1c));
        if (((struct particle_system_particle *)particle)->location.cluster_index != -1) {
            *(int16_t *)(type_state + 0x3a) += 1;
            ((struct particle_system_particle *)particle)->next_particle = *(datum_index *)(type_state + 0x3c);
            *(datum_index *)(type_state + 0x3c) = handle;
        } else {
            datum_delete(particle_system_particle_data, handle);
        }
        spawned++;
        if (*(int16_t *)(type_state + 0x3a) >= target) {
            break;
        }
    }

done:
    if ((float)*(int16_t *)(type_state + 0x3a) < *(float *)(type_state + 0x2c)) {
        *(float *)(type_state + 4) = *(float *)(type_state + 4) * 0.3f; // 0x672c94
    }
}

#if 0
Original Ghidra decompilation (0x453b10):

void FUN_00453b10(int param_1,undefined4 param_2)

{
  short *psVar1;
  uint *puVar2;
  int iVar3;
  char cVar4;
  short sVar5;
  short sVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  short sVar10;
  int iVar11;
  float10 extraout_ST0;
  float10 fVar12;
  short local_37c;
  int local_368;
  undefined1 local_360 [60];
  undefined4 local_324;
  undefined4 local_320;
  undefined4 local_31c;
  undefined4 local_300;
  undefined4 local_2fc;
  undefined4 local_2f8;

  psVar1 = (short *)((short)param_2 * 0x40 + 0x58 + param_1);
  iVar7 = (short)param_2 * 0x80 +
          *(int *)(*(int *)((*(uint *)(param_1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x60);
  uVar9 = *(uint *)(param_1 + 4) >> 1;
  if ((uVar9 & 1) == 0) {
    local_368 = *psVar1 * 0xc0 + *(int *)(iVar7 + 0x6c);
  }
  else {
    local_368 = 0;
  }
  sVar5 = FUN_00453a10(*(undefined4 *)(param_1 + 0xc));
  if ((sVar5 == 0) ||
     ((((*(uint *)(iVar7 + 0x20) & 0x20000) == 0 ||
       ((sVar5 != -1 && (cVar4 = FUN_0050fcd0(), cVar4 != '\0')))) &&
      (((*(uint *)(iVar7 + 0x20) & 0x10000) == 0 ||
       ((sVar5 != 1 || (cVar4 = FUN_0050fcd0(), cVar4 == '\0')))))))) {
    if ((uVar9 & 1) == 0) {
      sVar5 = __ftol();
      local_37c = psVar1[0x1d] + sVar5;
      fVar12 = (extraout_ST0 - (float10)(int)sVar5) + (float10)*(float *)(psVar1 + 0x1a);
      *(float *)(psVar1 + 0x1a) = (float)fVar12;
      if ((float10)1.0 < fVar12) {
        local_37c = local_37c + 1;
        *(float *)(psVar1 + 0x1a) = (float)(fVar12 - (float10)1.0);
      }
    }
    else if ((*(uint *)(iVar7 + 0x20) & 0x400) == 0) {
      local_37c = *(short *)(iVar7 + 0x24);
    }
    else {
      local_37c = __ftol();
    }
    if (DAT_0069c566 == '\x01') {
      local_37c = __ftol();
    }
    if (psVar1[0x1d] < local_37c) {
      uVar8 = *(uint *)(param_1 + 0xc);
      if (uVar8 == 0xffffffff) {
        local_300 = *(undefined4 *)(param_1 + 0x20);
        local_2fc = *(undefined4 *)(param_1 + 0x24);
        local_2f8 = *(undefined4 *)(param_1 + 0x28);
        local_324 = *(undefined4 *)PTR_DAT_00696714;
        local_320 = *(undefined4 *)(PTR_DAT_00696714 + 4);
        local_31c = *(undefined4 *)(PTR_DAT_00696714 + 8);
        sVar5 = 1;
      }
      else {
        puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc);
        sVar5 = object_get_node_local_transform
                          (uVar8,*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                                         + 0x144) + 0x10 + *(short *)(param_1 + 0x10) * 0x48,
                           local_360,8);
        FUN_004f6b10();
        if ((sVar5 == 0) &&
           (iVar3 = *(int *)(DAT_007c3108 * 0x1ea0 + 8 + DAT_006b2d98), iVar3 != -1)) {
          sVar5 = first_person_weapon_get_marker_data
                            (iVar3,*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                                    ) + 0x144) + 0x10 +
                                   *(short *)(param_1 + 0x10) * 0x48,local_360,8);
          FUN_004f6b10();
        }
      }
      if (*(short *)(param_1 + 0x1c) != -1) {
        sVar6 = psVar1[0x1d];
        sVar10 = 0;
        iVar3 = DAT_0087abd8;
        while ((((sVar6 < local_37c && (sVar5 != 0)) && (sVar10 < 0x80)) &&
               (uVar8 = datum_new(), uVar8 != 0xffffffff))) {
          iVar11 = (uVar8 & 0xffff) * 0x80 + *(int *)(iVar3 + 0x34);
          if ((uVar9 & 1) == 0) {
            sVar6 = *(short *)(local_368 + 0xb0);
          }
          else {
            sVar6 = *(short *)(iVar7 + 0x54);
          }
          *(undefined2 *)(iVar11 + 8) = 0xffff;
          *(undefined2 *)(iVar11 + 10) = 0xffff;
          *(undefined1 *)(iVar11 + 3) = 1;
          *(undefined1 *)(iVar11 + 2) = 1;
          *(undefined4 *)(iVar11 + 0x44) = 0xbf800000;
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          *(float *)(iVar11 + 0x40) = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          (**(code **)(&DAT_00657444 + sVar6 * 4))
                    (param_1,param_2,iVar11,
                     local_360 + (short)((DAT_00719cd4 >> 0x10) * (int)sVar5 >> 0x10) * 0x6c);
          FUN_0053e780();
          iVar3 = DAT_0087abd8;
          if (*(short *)(iVar11 + 0x18) == -1) {
            datum_delete();
          }
          else {
            psVar1[0x1d] = psVar1[0x1d] + 1;
            *(undefined4 *)(iVar11 + 4) = *(undefined4 *)(psVar1 + 0x1e);
            *(uint *)(psVar1 + 0x1e) = uVar8;
          }
          sVar10 = sVar10 + 1;
          sVar6 = psVar1[0x1d];
        }
      }
    }
    if ((float)(int)psVar1[0x1d] < *(float *)(psVar1 + 0x16)) {
      *(float *)(psVar1 + 2) = *(float *)(psVar1 + 2) * 0.3;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
