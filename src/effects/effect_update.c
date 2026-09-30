// effect_update  (Ghidra: particle_system_update; renamed: it reads effect_data, the 0xfc-byte effe table)
// address 0x451a30, size 1358 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/effects.h effect (flags 0x02, definition 0x04, the two function selectors 0x08/0x0a, change
//   color 0x0c, location 0x10/0x14, color 0x18, velocity 0x24, object 0x3c, a/b scale 0x44/0x48, event index
//   0x4e, event time 0x50, duration 0x54, previous fraction 0x58, particle counts 0xdc); Effect tag flags
//   (byte 0: bit 0 delete when disabled, bit 2 the global random stream), loop start / end event words
//   (+0x04 / +0x06), events (+0x34 count, 0x44 each at +0x38: skip fraction +0x04, duration bounds
//   +0x10/+0x14, particles +0x38 count, 0xe8 each at +0x3c: count bounds +0x6c/+0x6e, scale bitsets
//   +0xe0/+0xe4).
// REWRITTEN (from objdump 0x451a30..0x451f8c). With an attached object: a vanished object deletes the effect;
//   the root object's location (+0x98/+0x9c) and velocity (+0x68) are copied when it has flag bit 11, else the
//   cluster is -1. For an effect driven by its A function (flags bit 1): object_function_get_value(EAX object,
//   CX selector A, EDX &a_scale) false stops the effect (or, with tag flag bit 0, detaches it from the
//   object's attachment handles and deletes it); true restarts a stopped (bit 3) effect at event 0, or deletes
//   it when bit 5 is set. B scale and the change color are refreshed every tick. An effect in a cluster no local
//   player sees is suspended (bit 4) or deleted when not function-driven. Then, while time remains (at most 8
//   events per tick): the current event's time advances; a running event (bit 0) spawns particles and, when it
//   finishes, the next event is chosen (the loop start event after the loop end event for a function-driven
//   effect, else the next one; each skipped by a random roll under its skip fraction) or the effect stops
//   (bit 3, function-driven) or is deleted; a pending event starts: a random duration, per-particle counts
//   from effect_property_random_value (counts above 6 are spread over the local players), and the change
//   color. The draft passed nothing to object_function_get_value (a crash on the first campaign effect).
// blam-cc: stack -> effect_index, dt (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"
#include "units.h"
#include "fn_effects.h"

extern data_array *effect_data;                    // 0x0087abdc
extern data_array *object_data;                    // 0x008603b0
extern tag_instance *tag_instances;                // 0x0087bc14
extern player_globals *local_player_globals;       // 0x0087a478
extern random_seed random_seed_global;             // 0x00719cd0
extern random_seed effect_random_seed;             // 0x00719cd4

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack
extern uint32_t object_get_root_object_index(uint32_t object_index);            // 0x4f6fb0, blam-cc: ECX
extern uint8_t object_function_get_value(uint32_t object_index, int16_t selector, float *out_value);
    // 0x4f6e70, blam-cc: EAX -> object_index, CX -> selector, EDX -> out_value

extern void effect_delete(datum_index effect_handle);                          // 0x450be0


    // 0x451290, blam-cc: DL -> bit_index, EBX -> self, ESI -> a_bitset, EDI -> b_bitset, stack -> the rest


// One roll of the effect's random stream (tag flag bit 2 selects the global one), 0..1.
static real effect_update_roll_fraction(uint8_t *tag)
{
    random_seed *seed = (tag[0] & 4) ? &random_seed_global : &effect_random_seed;

    *seed = *seed * k_random_multiplier + k_random_increment;
    return (real)(*seed >> k_random_value_shift) * 1.5259022e-05f; // 0x672b84
}

void effect_update(datum_index effect_index, real dt)
{
    effect *self = (effect *)((uint8_t *)effect_data->data + (effect_index & 0xffff) * 0xfc);
    uint8_t *tag = (uint8_t *)tag_instances[self->definition_index & 0xffff].data;
    uint8_t *events = *(uint8_t **)(tag + 0x38);
    int32_t event_count = *(int32_t *)(tag + 0x34);
    datum_index object_index = self->object_index;
    int16_t steps;

    if (object_index != k_datum_index_none) {
        uint8_t *obj = (uint8_t *)object_try_and_get(object_index, 0xffffffff);
        uint8_t *root;

        if (obj == 0) {
            effect_delete(effect_index);
            return;
        }
        root = *(uint8_t **)((uint8_t *)object_data->data +
            (object_get_root_object_index(object_index) & 0xffff) * 0xc + 8);
        if (*(uint32_t *)(root + 0x10) & 0x800) {
            *(uint32_t *)&((struct effect *)self)->location.leaf_index = *(uint32_t *)(root + 0x98);
            *(uint32_t *)&((struct effect *)self)->location.cluster_index = *(uint32_t *)(root + 0x9c);
            self->velocity = *(real_vector3d *)(root + 0x68);
        } else {
            ((struct effect *)self)->location.cluster_index = -1;
        }
        if (self->flags & 2) {
            if (object_function_get_value(object_index, self->a_scale_function_index, &self->a_scale)) {
                if (self->flags & 8) {
                    if (self->flags & 0x20) {
                        effect_delete(effect_index);
                    } else {
                        self->flags = (uint16_t)(self->flags & 0xfff7);
                        effect_start_event(effect_index, 0);
                    }
                }
            } else if (tag[0] & 1) {
                uint8_t *obj_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
                int16_t i;

                for (i = 0; i < *(int32_t *)&((struct Object *)obj_tag)->attachments.count; i++) {
                    if (*(datum_index *)(obj + 0x14c + i * 4) == effect_index) {
                        *(datum_index *)(obj + 0x14c + i * 4) = k_datum_index_none;
                        break;
                    }
                }
                effect_delete(effect_index);
                return;
            } else if ((self->flags & 0xc) == 0) {
                effect_stop(effect_index, 0);
            }
            object_function_get_value(self->object_index, self->b_scale_function_index, &self->b_scale);
            if (self->change_color_index != -1) {
                self->color = *(ColorRGB *)(obj + 0x1b8 + self->change_color_index * 0xc);
            }
        }
    }

    // suspended while no local player sees the effect's cluster
    {
        int16_t cluster = ((struct effect *)self)->location.cluster_index;
        uint8_t visible = 0;

        if (cluster != -1) {
            uint32_t *bits = (uint32_t *)((uint8_t *)local_player_globals + ((tag[0] & 4) ? 0x18 : 0x58));

            visible = (bits[cluster >> 5] & (1u << (cluster & 0x1f))) != 0;
        }
        if (visible) {
            if (self->flags & 0x10) {
                self->flags = (uint16_t)(self->flags & 0xffef);
            }
        } else if ((self->flags & 0x10) == 0) {
            if ((self->flags & 2) == 0) {
                effect_delete(effect_index);
                return;
            }
            self->flags = (uint16_t)(self->flags | 0x10);
        }
    }

    if (dt < 0.0f || dt != dt) {
        return;
    }
    for (steps = 0; ; steps++) {
        uint16_t flags = self->flags;
        uint8_t finished;
        real remaining;

        if ((flags & 8) || steps >= 8) {
            return;
        }
        remaining = self->event_duration - self->event_time;
        if (remaining <= dt) {
            dt = dt - remaining;
            finished = 1;
            self->event_time = self->event_duration;
        } else {
            finished = 0;
            self->event_time = self->event_time + dt;
            dt = -1.0f;
        }
        if (flags & 1) {
            if ((flags & 0x10) == 0) {
                effect_spawn_particles(self);
            }
            if (finished) {
                int16_t next;

                if ((self->flags & 2) && self->event_index == *(int16_t *)(tag + 6) &&
                    *(int16_t *)(tag + 4) != -1) {
                    next = *(int16_t *)(tag + 4);
                } else {
                    next = (int16_t)(self->event_index + 1);
                }
                while (next < event_count &&
                    effect_update_roll_fraction((uint8_t *)tag_instances[self->definition_index & 0xffff].data) <
                        *(float *)(events + next * 0x44 + 4)) {
                    next++;
                }
                if (next >= event_count) {
                    if (self->flags & 2) {
                        self->flags = (uint16_t)(self->flags | 8);
                        return;
                    }
                    effect_delete(effect_index);
                    return;
                }
                effect_start_event(effect_index, next);
            }
        } else if (finished) {
            uint8_t *event = events + self->event_index * 0x44;
            int32_t particle;

            self->flags = (uint16_t)(flags | 1);
            self->event_time = 0.0f;
            self->previous_event_fraction = -1.0f;
            {
                real fraction = effect_update_roll_fraction((uint8_t *)tag_instances[self->definition_index & 0xffff].data);

                self->event_duration = (*(float *)(event + 0x14) - *(float *)(event + 0x10)) * fraction +
                    *(float *)(event + 0x10);
            }
            for (particle = 0; particle < *(int32_t *)(event + 0x38); particle = (int16_t)(particle + 1)) {
                uint8_t *part = *(uint8_t **)(event + 0x3c) + particle * 0xe8;
                uint8_t count = (uint8_t)(int32_t)effect_property_random_value(5, self, *(uint32_t *)(part + 0xe0),
                    *(uint32_t *)(part + 0xe4), &effect_random_seed, (real)*(int16_t *)(part + 0x6c),
                    (real)*(int16_t *)(part + 0x6e));

                self->particle_counts[particle] = count;
                if (count > 6) {
                    self->particle_counts[particle] = (uint8_t)(int32_t)(((real)count - 6.0f) /
                        (real)*(int16_t *)((uint8_t *)local_player_globals + 0xc) + 6.0f);
                }
            }
            if ((self->flags & 0x10) == 0) {
                object_change_color_evaluate(self);
            }
        }
        if (!(dt >= 0.0f)) {
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x451a30):

/* WARNING: Removing unreachable block (ram,0x00451e37) */
/* WARNING: Removing unreachable block (ram,0x00451d82) */

void __cdecl particle_system_update(uint particle_system_index,float delta_time)

{
  float fVar1;
  float fVar2;
  byte *pbVar3;
  char cVar4;
  byte bVar5;
  undefined1 uVar6;
  ushort uVar7;
  uint *puVar8;
  uint uVar9;
  short sVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  short sVar14;
  bool local_15;
  int local_14;

  iVar11 = (particle_system_index & 0xffff) * 0xfc;
  iVar12 = iVar11 + *(int *)(DAT_0087abdc + 0x34);
  pbVar3 = *(byte **)((*(uint *)(iVar11 + 4 + *(int *)(DAT_0087abdc + 0x34)) & 0xffff) * 0x20 + 0x14
                     + DAT_0087bc14);
  if (*(int *)(iVar12 + 0x3c) != -1) {
    puVar8 = (uint *)object_try_and_get(0xffffffff);
    if (puVar8 == (uint *)0x0) {
LAB_00451f78:
      particle_system_delete_450be0(particle_system_index);
      return;
    }
    uVar9 = object_get_root_object_index();
    iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc);
    if ((*(uint *)(iVar11 + 0x10) & 0x800) == 0) {
      *(undefined2 *)(iVar12 + 0x14) = 0xffff;
    }
    else {
      *(undefined4 *)(iVar12 + 0x10) = *(undefined4 *)(iVar11 + 0x98);
      *(undefined4 *)(iVar12 + 0x14) = *(undefined4 *)(iVar11 + 0x9c);
      *(undefined4 *)(iVar12 + 0x24) = *(undefined4 *)(iVar11 + 0x68);
      *(undefined4 *)(iVar12 + 0x28) = *(undefined4 *)(iVar11 + 0x6c);
      *(undefined4 *)(iVar12 + 0x2c) = *(undefined4 *)(iVar11 + 0x70);
    }
    if ((*(byte *)(iVar12 + 2) & 2) != 0) {
      cVar4 = object_function_get_value();
      if (cVar4 == '\0') {
        if ((*pbVar3 & 1) != 0) {
          iVar11 = *(int *)(*(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x140);
          sVar10 = 0;
          if (0 < iVar11) {
            iVar12 = 0;
            while (puVar8[iVar12 + 0x53] != particle_system_index) {
              sVar10 = sVar10 + 1;
              iVar12 = (int)sVar10;
              if (iVar11 <= iVar12) {
                particle_system_delete_450be0(particle_system_index);
                return;
              }
            }
            puVar8[sVar10 + 0x53] = 0xffffffff;
          }
          goto LAB_00451b68;
        }
        if ((*(byte *)(iVar12 + 2) & 0xc) == 0) {
          FUN_00450b20(0);
        }
      }
      else {
        uVar7 = *(ushort *)(iVar12 + 2);
        if ((uVar7 & 8) != 0) {
          if ((uVar7 & 0x20) == 0) {
            *(ushort *)(iVar12 + 2) = uVar7 & 0xfff7;
            FUN_00451660();
          }
          else {
            particle_system_delete_450be0(particle_system_index);
          }
        }
      }
      object_function_get_value();
      if (*(short *)(iVar12 + 0xc) != -1) {
        puVar8 = puVar8 + *(short *)(iVar12 + 0xc) * 3 + 0x6e;
        *(uint *)(iVar12 + 0x18) = *puVar8;
        *(uint *)(iVar12 + 0x1c) = puVar8[1];
        *(uint *)(iVar12 + 0x20) = puVar8[2];
      }
    }
  }
  if (*(short *)(iVar12 + 0x14) == -1) {
LAB_00451c41:
    uVar7 = *(ushort *)(iVar12 + 2);
    if ((uVar7 & 0x10) != 0) goto LAB_00451c6b;
    if ((uVar7 & 2) == 0) {
LAB_00451b68:
      particle_system_delete_450be0(particle_system_index);
      return;
    }
    uVar7 = uVar7 | 0x10;
  }
  else {
    iVar11 = (int)*(short *)(iVar12 + 0x14) >> 5;
    if ((*pbVar3 & 4) == 0) {
      uVar9 = *(uint *)(DAT_0087a478 + 0x58 + iVar11 * 4);
    }
    else {
      uVar9 = *(uint *)(DAT_0087a478 + 0x18 + iVar11 * 4);
    }
    if ((uVar9 & 1 << ((byte)*(short *)(iVar12 + 0x14) & 0x1f)) == 0) goto LAB_00451c41;
    if ((*(ushort *)(iVar12 + 2) & 0x10) == 0) goto LAB_00451c6b;
    uVar7 = *(ushort *)(iVar12 + 2) & 0xffef;
  }
  *(ushort *)(iVar12 + 2) = uVar7;
LAB_00451c6b:
  sVar10 = 0;
  if (delta_time < 0.0) {
    return;
  }
  do {
    fVar1 = delta_time;
    uVar7 = *(ushort *)(iVar12 + 2);
    if ((uVar7 & 8) != 0) {
      return;
    }
    if (7 < sVar10) {
      return;
    }
    fVar2 = *(float *)(iVar12 + 0x54) - *(float *)(iVar12 + 0x50);
    local_15 = fVar2 < delta_time == (fVar2 == delta_time);
    if (local_15) {
      delta_time = -1.0;
      *(float *)(iVar12 + 0x50) = fVar1 + *(float *)(iVar12 + 0x50);
    }
    else {
      delta_time = delta_time - fVar2;
      *(undefined4 *)(iVar12 + 0x50) = *(undefined4 *)(iVar12 + 0x54);
    }
    local_15 = !local_15;
    if ((uVar7 & 1) == 0) {
      if (local_15) {
        iVar13 = *(short *)(iVar12 + 0x4e) * 0x44 + *(int *)(pbVar3 + 0x38);
        *(ushort *)(iVar12 + 2) = uVar7 | 1;
        iVar11 = DAT_0087bc14;
        *(undefined4 *)(iVar12 + 0x50) = 0;
        *(undefined4 *)(iVar12 + 0x58) = 0xbf800000;
        puVar8 = &random_seed_global;
        if ((**(byte **)((*(uint *)(iVar12 + 4) & 0xffff) * 0x20 + 0x14 + iVar11) & 4) == 0) {
          puVar8 = &DAT_00719cd4;
        }
        fVar1 = *(float *)(iVar13 + 0x14);
        fVar2 = *(float *)(iVar13 + 0x10);
        uVar9 = *puVar8 * 0x19660d + 0x3c6ef35f;
        *puVar8 = uVar9;
        sVar14 = 0;
        *(float *)(iVar12 + 0x54) = (fVar1 - fVar2) * (float)(uVar9 >> 0x10) * 1.5259022e-05 + fVar2
        ;
        if (0 < *(int *)(iVar13 + 0x38)) {
          local_14 = 0;
          do {
            particle_system_property_random_value
                      (&DAT_00719cd4,
                       (float)(int)*(short *)(local_14 * 0xe8 + *(int *)(iVar13 + 0x3c) + 0x6c),
                       (float)(int)*(short *)(local_14 * 0xe8 + 0x6e + *(int *)(iVar13 + 0x3c)));
            bVar5 = __ftol();
            *(byte *)(local_14 + 0xdc + iVar12) = bVar5;
            if (6 < bVar5) {
              uVar6 = __ftol();
              *(undefined1 *)(local_14 + 0xdc + iVar12) = uVar6;
            }
            sVar14 = sVar14 + 1;
            local_14 = (int)sVar14;
          } while (local_14 < *(int *)(iVar13 + 0x38));
        }
        if ((*(byte *)(iVar12 + 2) & 0x10) == 0) {
          FUN_004529d0(iVar12);
        }
      }
    }
    else {
      if ((uVar7 & 0x10) == 0) {
        particle_system_spawn_particles(iVar12);
      }
      if (local_15) {
        if ((((*(byte *)(iVar12 + 2) & 2) == 0) ||
            (*(short *)(iVar12 + 0x4e) != *(short *)(pbVar3 + 6))) ||
           (sVar14 = *(short *)(pbVar3 + 4), sVar14 == -1)) {
          sVar14 = *(short *)(iVar12 + 0x4e) + 1;
        }
        iVar11 = (int)sVar14;
        if (iVar11 < *(int *)(pbVar3 + 0x34)) {
          do {
            puVar8 = &random_seed_global;
            if ((**(byte **)((*(uint *)(iVar12 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 4) ==
                0) {
              puVar8 = &DAT_00719cd4;
            }
            uVar9 = *puVar8 * 0x19660d + 0x3c6ef35f;
            *puVar8 = uVar9;
            if (*(float *)(iVar11 * 0x44 + 4 + *(int *)(pbVar3 + 0x38)) <=
                (float)(uVar9 >> 0x10) * 1.5259022e-05) break;
            sVar14 = sVar14 + 1;
            iVar11 = (int)sVar14;
          } while (iVar11 < *(int *)(pbVar3 + 0x34));
        }
        if (*(int *)(pbVar3 + 0x34) <= (int)sVar14) {
          if ((*(ushort *)(iVar12 + 2) & 2) != 0) {
            *(ushort *)(iVar12 + 2) = *(ushort *)(iVar12 + 2) | 8;
            return;
          }
          goto LAB_00451f78;
        }
        FUN_00451660();
      }
    }
    sVar10 = sVar10 + 1;
    if (delta_time < 0.0) {
      return;
    }
  } while( true );
}
#endif
