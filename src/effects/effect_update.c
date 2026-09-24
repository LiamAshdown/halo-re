// effect_update  (Ghidra: particle_system_update; RENAMED per out/phase4/effects_types_notes.md's
// misattribution table: "0x451a30 particle_system_update -> effect_update -- reads the particle
// count at +0x38 with stride 0xe8" against effect_data, the 0xfc-byte effe table, not pctl)
// address 0x451a30, size 1358 bytes
// name confidence: 0.5   rewrite confidence: 0.4 (several register-passed callee arguments are
//   elided by Ghidra at their call sites; each is reconstructed from context and flagged below)
// evidence: types/effects.h effect (flags, location 0x10, color 0x18, velocity 0x24, object_index
//   0x3c, first_person_weapon_index 0x4c, event_index 0x4e, event_time 0x50, event_duration 0x54,
//   previous_event_fraction 0x58, particle_counts[32] 0xdc) and effect_flags; types/tags.h Effect
//   (flags, loop_start_event, loop_stop_event, events TagReflexive at 0x34) and EffectEvent
//   (skip_fraction, duration_bounds, particles TagReflexive at 0x38) and EffectParticle.count[2]
//   (0x6c/0x6e); types/objects.h object (flags bit _object_needs_cluster_update_bit,
//   location_leaf_index 0x98, location_cluster_index 0x9c, velocity 0x68, attachment_handles[8]
//   0x14c) and object_header; types/cache.h tag_instance; this module's effect_stop 0x450b20,
//   effect_start_event 0x451660, effect_delete 0x450be0, effect_property_random_value 0x451290,
//   effect_spawn_particles 0x451f90 and object_change_color_evaluate 0x4529d0; src/objects/
//   object_try_and_get.c, object_get_root_object_index.c and object_function_get_value.c for the
//   three foreign callees.
// register convention: __cdecl, both arguments Ghidra's own recognized stack parameters (the
//   effect handle, delta_time). Several *callees* have register-passed arguments Ghidra could
//   not attribute at these particular call sites (unlike the calls that show
//   "particle_system_delete_450be0(particle_system_index)" plainly, which is why those are kept
//   verbatim); each such case is called out individually below.
//   // blam-cc: stack -> (effect_handle, delta_time)
// UNSURE (grouped): (1) object_get_root_object_index's argument is not shown; reconstructed as
//   self->object_index, the only object index in scope. (2) object_function_get_value's two
//   calls show no arguments at all (same "invisible register arguments" pattern already
//   documented in src/objects/glow_update.c); modeled the same way, as a bare call. (3)
//   effect_stop's effect-handle argument (EAX) is not shown at its call site; reconstructed as
//   this function's own effect_handle parameter. (4) the FUN_00451660 (effect_start_event) call
//   in the "event just started" branch shows no arguments; reconstructed as
//   (effect_handle, tag->loop_start_event), since that is the only event index available in that
//   branch (the effect had just finished and its function turned back on). (5) the
//   particle_system_property_random_value (effect_property_random_value) call inside the
//   particle-count roll shows only its three stack arguments (seed, base_min, base_max); its four
//   register arguments (bit_index, self, a_bitset, b_bitset) are assumed to be all zero (no A/B
//   scale applied), the degenerate case that reduces the call to a plain random-range roll --
//   this cannot be confirmed from the decompile alone. (6) the `if (6 < bVar5) { <redo> }`
//   sequence truncates the *same* FPU value a second time with nothing visibly pushed in
//   between; almost certainly Ghidra lost a clamp (most likely against a literal 6.0) that this
//   rewrite cannot recover exactly, so the second __ftol() call is preserved verbatim rather than
//   inventing the clamp expression. (7) the final FUN_00451660 (effect_start_event) call passes
//   the freshly resolved next_event_index; its effect_handle argument (EAX) is not shown and is
//   reconstructed as this function's own effect_handle parameter, same as (3). (8) player_globals
//   offsets 0x18 and 0x58 are two 128-bit-wide (16 dword) visibility bitmasks this module is the
//   first to read; types/game.h's player_globals leaves 0x17..0x98 as an untyped tail
//   (unknown_17), so they are reached here by raw byte offset with a comment rather than adding
//   named fields to a header this module does not own.
// UNSURE: after the object.attachment_handles detach-and-delete path (tag EffectFlags bit 0,
//   "deleted_when_attachment_deactivates"), the decompile falls through to
//   `object_function_get_value(); if (self->change_color_index != -1) {...}` on the very same,
//   just-deleted effect record instead of returning -- kept exactly as decompiled (no `return`
//   added) since effect_delete only unlinks/frees the datum slot rather than zeroing it, so
//   reading the just-deleted record's own fields is harmless but almost certainly not intended by
//   the original source; flagged rather than "fixed".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"

extern data_array *effect_data;                    // 0x0087abdc
extern data_array *object_data;                    // 0x008603b0
extern tag_instance *tag_instances;                // 0x0087bc14
extern player_globals *local_player_globals;       // 0x0087a478
extern random_seed random_seed_global;             // 0x00719cd0
extern random_seed effect_random_seed;             // 0x00719cd4

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint32_t object_get_root_object_index(uint32_t object_index);            // 0x4f6fb0
extern uint8_t object_function_get_value(void); // 0x4f6e70, UNSURE: args not visible here, same
    // as src/objects/glow_update.c's declaration of the same function
extern int32_t __ftol(void); // 0x6391b4, MSVC runtime float-to-int truncation, argument on the
    // x87 stack, UNSURE (see src/game/player_apply_pickup_effect.c for the established pattern)

extern void effect_delete(datum_index effect_handle);                          // 0x450be0, this module
extern void effect_stop(datum_index effect_handle, uint8_t stop_immediately);  // 0x450b20, this module
extern void effect_start_event(datum_index effect_handle, int16_t event_index); // 0x451660, this module
extern real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset,
    uint32_t b_bitset, random_seed *seed, real base_min, real base_max);       // 0x451290, this module
extern void effect_spawn_particles(effect *self);           // 0x451f90, this module
extern void object_change_color_evaluate(effect *self);     // 0x4529d0, this module

// Rolls the next pseudo-random fraction in [0, 1) from whichever seed the Effect tag's
// must_be_deterministic_pc flag (EffectFlags bit 2) selects. The raw code is
// `seed = &random_seed_global; if ((tag->flags & 4) == 0) seed = &effect_random_seed;`, i.e. the
// tag flag SET selects the shared engine stream and clear selects this module's own stream --
// which is also what types/effects.h's note on effect_random_seed says. An earlier draft of this
// helper had the test inverted (the identical inline copy in the event-start path below did
// not), so the two disagreed.
static real effect_update_roll_fraction(Effect *tag)
{
    random_seed *seed = &random_seed_global;
    if ((tag->flags & 4) == 0) {
        seed = &effect_random_seed;
    }
    *seed = *seed * k_random_multiplier + k_random_increment;
    return (real)(*seed >> k_random_value_shift) * 1.5259022e-05f;
}

// Advances one effect for one tick: resolves its attached object's cluster/velocity and the
// object-function-driven emitting state, updates visibility, then walks its event timeline
// (starting the next event, rolling per-event particle counts and spawning particles) up to
// k_effect_events_per_update times to absorb the leftover delta_time.
void effect_update(datum_index effect_handle, real delta_time)
{
    effect *self = &((effect *)effect_data->data)[(uint16_t)effect_handle];
    Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
    int16_t event_count_cursor;
    uint16_t flags;

    if (self->object_index != k_datum_index_none) {
        object *attached = object_try_and_get(self->object_index, _object_mask_all);
        if (attached == (object *)0) {
            effect_delete(effect_handle);
            return;
        }

        uint32_t root_index = object_get_root_object_index(self->object_index); // UNSURE, see file header
        object *root = ((object_header *)object_data->data)[(uint16_t)root_index].data;

        if ((root->flags & _object_needs_cluster_update_bit) == 0) {
            self->location.cluster_index = -1;
        } else {
            self->location.leaf_index = root->location_leaf_index;
            self->location.cluster_index = root->location_cluster_index;
            self->velocity = root->velocity;
        }

        if ((self->flags & _effect_looping_bit) != 0) {
            uint8_t function_is_zero = object_function_get_value(); // UNSURE, see file header

            if (function_is_zero == 0) {
                if ((tag->flags & 1) != 0) { // EffectFlags bit 0: deleted_when_attachment_deactivates
                    // Object tag attachments.count: TagReflexive at Object tag +0x140.
                    int32_t tag_attachment_count =
                        *(int32_t *)((uint8_t *)tag_instances[(uint16_t)attached->definition_tag].data + 0x140);
                    int16_t slot = 0;

                    if (0 < tag_attachment_count) {
                        int32_t index = 0;
                        while (attached->attachment_handles[index] != effect_handle) {
                            slot++;
                            index = slot;
                            if (tag_attachment_count <= index) {
                                effect_delete(effect_handle);
                                return;
                            }
                        }
                        attached->attachment_handles[slot] = k_datum_index_none;
                    }
                    // Ghidra's `goto LAB_00451b68`, which is the unconditional delete -- NOT
                    // LAB_00451c41, the visibility check the two labels sit next to. An earlier
                    // draft of this file aimed this goto at the wrong one of the two.
                    goto delete_effect;
                }
                if ((self->flags & (_effect_stopping_bit | _effect_finished_bit)) == 0) {
                    effect_stop(effect_handle, 0); // UNSURE, see file header
                }
            } else {
                flags = self->flags;
                if ((flags & _effect_finished_bit) != 0) {
                    if ((flags & _effect_stop_immediately_bit) == 0) {
                        self->flags = flags & (uint16_t)~_effect_finished_bit;
                        effect_start_event(effect_handle, tag->loop_start_event); // UNSURE, see file header
                    } else {
                        effect_delete(effect_handle);
                        // UNSURE: falls through to read the just-deleted record below, see file header
                    }
                }
            }

            object_function_get_value(); // UNSURE, see file header; return value unused
            if (self->change_color_index != -1) {
                ColorRGB *change_colors = (ColorRGB *)((uint8_t *)attached + 0x1b8);
                self->color.red = change_colors[self->change_color_index].red;
                self->color.green = change_colors[self->change_color_index].green;
                self->color.blue = change_colors[self->change_color_index].blue;
            }
        }
    }

    if (self->location.cluster_index == -1) {
check_visibility_flags:   // Ghidra LAB_00451c41
        flags = self->flags;
        if ((flags & _effect_hidden_bit) != 0) {
            goto continue_events;
        }
        if ((flags & _effect_looping_bit) == 0) {
delete_effect:            // Ghidra LAB_00451b68
            effect_delete(effect_handle);
            return;
        }
        flags = flags | _effect_hidden_bit;
    } else {
        int32_t word_index = self->location.cluster_index >> 5;
        uint32_t visible_mask;

        if ((tag->flags & 4) == 0) { // must_be_deterministic_pc clear
            visible_mask = *(uint32_t *)((uint8_t *)local_player_globals + 0x58 + word_index * 4);
                // UNSURE: unnamed player_globals field, see file header
        } else {
            visible_mask = *(uint32_t *)((uint8_t *)local_player_globals + 0x18 + word_index * 4);
                // UNSURE: unnamed player_globals field, see file header
        }

        if ((visible_mask & (1u << (self->location.cluster_index & 0x1f))) == 0) {
            goto check_visibility_flags;
        }
        flags = self->flags;
        if ((flags & _effect_hidden_bit) == 0) {
            goto continue_events;
        }
        flags = flags & (uint16_t)~_effect_hidden_bit;
    }
    self->flags = flags;

continue_events:
    event_count_cursor = 0;
    if (delta_time < 0.0f) {
        return;
    }

    for (;;) {
        real tick_delta = delta_time;
        uint16_t current_flags = self->flags;
        int32_t next_event_index;

        if ((current_flags & _effect_finished_bit) != 0) {
            return;
        }
        if (event_count_cursor > 7) { // k_effect_events_per_update
            return;
        }

        {
            real time_left_in_event = self->event_duration - self->event_time;
            uint8_t event_finished_this_pass = !(time_left_in_event > delta_time);

            if (!event_finished_this_pass) {
                delta_time = -1.0f;
                self->event_time = tick_delta + self->event_time;
            } else {
                delta_time = delta_time - time_left_in_event;
                self->event_time = self->event_duration;
            }

            if ((current_flags & _effect_event_started_bit) == 0) {
                if (event_finished_this_pass) {
                    EffectEvent *event = &((EffectEvent *)tag->events.pointer)[self->event_index];

                    self->flags = current_flags | _effect_event_started_bit;
                    self->event_time = 0.0f;
                    self->previous_event_fraction = -1.0f;

                    {
                        random_seed *seed = &random_seed_global;
                        if ((tag->flags & 4) == 0) {
                            seed = &effect_random_seed;
                        }
                        *seed = *seed * k_random_multiplier + k_random_increment;
                        self->event_duration = (event->duration_bounds[1] - event->duration_bounds[0]) *
                            (real)(*seed >> k_random_value_shift) * 1.5259022e-05f + event->duration_bounds[0];
                    }

                    if (0 < (int32_t)event->particles.count) {
                        EffectParticle *particle_types = (EffectParticle *)event->particles.pointer;
                        int32_t i;

                        for (i = 0; i < (int32_t)event->particles.count; i++) {
                            real count_value = effect_property_random_value(0, self, 0, 0,
                                &effect_random_seed, (real)particle_types[i].count[0],
                                (real)particle_types[i].count[1]); // UNSURE, see file header
                            uint8_t count_byte = (uint8_t)__ftol();
                            (void)count_value;
                            self->particle_counts[i] = count_byte;
                            if (count_byte > 6) {
                                count_byte = (uint8_t)__ftol(); // UNSURE, see file header
                                self->particle_counts[i] = count_byte;
                            }
                        }
                    }

                    if ((self->flags & _effect_hidden_bit) == 0) {
                        object_change_color_evaluate(self);
                    }
                }
            } else {
                if ((current_flags & _effect_hidden_bit) == 0) {
                    effect_spawn_particles(self);
                }

                if (event_finished_this_pass) {
                    next_event_index = self->event_index + 1;
                    if ((self->flags & _effect_looping_bit) != 0 &&
                        self->event_index == (int16_t)tag->loop_stop_event &&
                        tag->loop_start_event != (uint16_t)0xffff) {
                        next_event_index = tag->loop_start_event;
                    }

                    while (next_event_index < (int32_t)tag->events.count) {
                        EffectEvent *candidate = &((EffectEvent *)tag->events.pointer)[next_event_index];
                        real fraction = effect_update_roll_fraction(tag);
                        if (candidate->skip_fraction <= fraction) {
                            break;
                        }
                        next_event_index++;
                    }

                    if ((int32_t)tag->events.count <= next_event_index) {
                        if ((self->flags & _effect_looping_bit) != 0) {
                            self->flags = self->flags | _effect_finished_bit;
                            return;
                        }
                        goto delete_effect;
                    }
                    effect_start_event(effect_handle, (int16_t)next_event_index); // UNSURE, see file header
                }
            }
        }

        event_count_cursor++;
        if (delta_time < 0.0f) {
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
