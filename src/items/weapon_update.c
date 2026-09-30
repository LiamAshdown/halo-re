// weapon_update  (Ghidra: item_update_triggers; renamed per items_types_notes.md: "it is the
// weapon row's vtable +0x34, and the item row's +0x34 is 0x4bc5c0 (already item_update)")
// address 0x4c1530, size 2981 bytes
// name confidence: 0.55   rewrite confidence: 0.55 (raised by the phase-4 verification pass, which
//   re-derived this function against objdump disassembly rather than the decompilation; see the
//   notes in the body) (the largest function in this batch)
// evidence: types/items.h weapon_data (every field), weapon_trigger_state, weapon_magazine_state,
//   item_data.flags; types/objects.h object.parent_object/animation_index/flags;
//   types/tags.h Weapon (animation_graph via Object base at +0x44, weapon_flags, triggers,
//   magazines, heat_recovery_threshold/overheated_threshold, heat_loss_rate,
//   age_heat_recovery_penalty, secondary_trigger_mode, maximum_alternate_shots_loaded,
//   weapon_type), Unit.unit_flags (bit 23 = integrated_light_cntrls_weapon),
//   WeaponTriggerFlags (bit 6 = sticks_when_dropped, bit 9 = analog_rate_of_fire, bit 3 =
//   does_not_repeat_automatically), WeaponMagazine.rounds_reserved_maximum/rounds_loaded_maximum.
//   Every raw offset below was independently confirmed against types/items.h and types/tags.h
//   with an offsetof probe (see the phase-4 session notes) before being replaced with a named
//   field, including the weapon_trigger_state/weapon_magazine_state stride math.
// register convention: item index is a Ghidra-recognized __cdecl parameter.
// blam-cc: stack -> item_index
// UNSURE: the `object_try_and_get(parent_object, 0xffffffff)` and
// `object_try_and_get(parent_object, _object_mask_unit)` calls show only their type-mask
// argument in the decompilation; the object index is inferred from the value tested
// immediately beforehand, matching the convention established across this whole module. The
// return value's upper 24 bits (CONCAT31 with leftover register content) are dropped; this
// function always returns true to its only caller in the exported code.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "models.h"
#include "fn_items.h"
#include "fn_effects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t unit_updates_suppressed;      // 0x0071c419

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter,
    char use_matched_index); // 0x4f6c60, EAX, stack
extern char *weapon_blur_permutation_names[2]; // 0x006961b8

extern void weapon_action_notify_for_weapon(datum_index weapon_index, int32_t action_code); // 0x492790, EAX, EDI

extern void item_detonation_timer_start(uint32_t object_index); // 0x4bd450, this module
extern animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index, animation_state *state,
    int32_t *sound_tag_id, animation_random_stream random_stream); // 0x4d48d0, EAX, ESI, EBX, stack


// 0x4c1f6a: a hidden weapon (object flag 1) that is attached shows its blur on the holder
static uint32_t weapon_blur_target(uint32_t item_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[item_index & 0xffff].data;

    if ((((object *)obj)->flags & 1) && ((object *)obj)->parent_object != (datum_index)0xffffffff) {
        return ((object *)obj)->parent_object;
    }
    return item_index;
}

// Per-tick update for a weapon item: heat/age/charge meters, the ready_timer countdown, the
// per-magazine recharge and reload state machines, and the per-trigger firing-decision switch.
// Returns true (the original packs unrelated leftover register bits into the unused upper 24
// bits of the return value; no caller in this module's export reads them).
int32_t weapon_update(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    int16_t i;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (unit_updates_suppressed == 1) {
        return 1;
    }

    // Animation-driven indicator/settle callbacks, only while an animation is actually playing.
    if (*(datum_index *)&weapon_tag->base.base.animation_graph.tag_id != (datum_index)0xffffffff &&
        item_obj->animation_index != -1) {
        // 0x4c15a7: EAX the graph, ESI the object's state (+0xd0), EBX 0, stream 1
        int16_t kind = (int16_t)animation_state_advance(*(datum_index *)&weapon_tag->base.base.animation_graph.tag_id,
                                                        (animation_state *)((uint8_t *)item_obj + 0xd0), 0,
                                                        (animation_random_stream)1);
        if (kind == 1) {
            weapon_set_state_indicator_flags(item_index);
        } else if (kind == 2) {
            weapon_force_settled_state(item_index);
        }
    }

    // Detonate-when-dropped: seed the item detonation timer once when the weapon has no holder.
    if ((weapon_tag->weapon_flags & 0x400) != 0 && item_obj->parent_object == (datum_index)0xffffffff) {
        item_detonation_timer_start(item_index);
    }

    // ready_timer: decrements by 1/24 per tick unless the holder's Object tag has the
    // integrated_light_cntrls_weapon flag (0x800000), in which case unit_update writes it
    // directly through weapon_set_ready_timer.
    if (wd->ready_timer > 0.0f) {
        int skip_decrement = 0;
        if (item_obj->parent_object == (datum_index)0xffffffff) {
            skip_decrement = 0;
        } else {
            object *holder = object_try_and_get(item_obj->parent_object, _object_mask_unit);
            if (holder == 0) {
                skip_decrement = 0;
            } else if (holder->definition_tag == (datum_index)0xffffffff) {
                skip_decrement = 0;
            } else {
                Unit *holder_tag = (Unit *)tag_instances[(uint16_t)holder->definition_tag].data;
                skip_decrement = (holder_tag->unit_flags & 0x800000) != 0;
            }
        }
        if (!skip_decrement) {
            wd->ready_timer = wd->ready_timer - 0.041666668f;
            if (wd->ready_timer < 0.0f) {
                wd->ready_timer = 0.0f;
            }
        }
    }

    // Heat: gains from firing (elsewhere) and decays here, scaled by the age penalty; crossing
    // overheated_threshold plays the overheat cue once and stores its handle.
    if (wd->heat > 0.0f) {
        if (weapon_tag->overheated_threshold <= wd->heat && (wd->flags & 1) == 0) {
            wd->flags = wd->flags | 1; // _weapon_overheated_bit
            int32_t action = 0xf;

            if (weapon_tag->weapon_type == 3 && (wd->flags & 4) != 0) {
                wd->flags = (wd->flags & ~(uint32_t)4) | 1; // clear alternate-shot-armed bit
                action = 0x10;
            }
            weapon_action_notify_for_weapon(item_index, action); // 0x4c16e0: EAX weapon, EDI 0xf / 0x10
            wd->overheat_effect_handle = weapon_stop_object_effect(item_index, *(datum_index *)&weapon_tag->overheated.tag_id);
        }

        if (wd->charged_fraction == 0.0f) {
            real loss = weapon_tag->heat_loss_rate * 0.033333335f;
            if (weapon_tag->age_heat_recovery_penalty > 0.0f) {
                loss = (1.0f - wd->age * weapon_tag->age_heat_recovery_penalty) * loss;
            }
            {
                real new_heat = wd->heat - loss;
                wd->heat = new_heat;
                if (new_heat < 0.0f) {
                    wd->heat = 0.0f;
                }
            }
            if ((wd->flags & 1) != 0 && (wd->flags & 2) == 0) {
                real fraction = (wd->heat - weapon_tag->heat_recovery_threshold) / loss;
                if ((fraction < 1.0f) != (fraction == 1.0f)) {
                    wd->flags = wd->flags | 2;
                }
            }
        }

        if ((wd->flags & 1) != 0 && wd->heat < weapon_tag->heat_recovery_threshold) {
            wd->flags = wd->flags & ~(uint32_t)3;
            if (wd->overheat_effect_handle != (datum_index)0xffffffff) {
                effect_stop(wd->overheat_effect_handle, 1);
            }
        }
    }
    wd->charged_fraction = 0.0f;

    if (wd->action_ticks > 0) {
        wd->action_ticks = wd->action_ticks - 1;
    }

    // Decide which triggers count as "pulled" this tick.
    {
        uint8_t pulled[2];
        int32_t local_trigger_index = 0;

        pulled[0] = 0;
        if ((wd->control_flags & 0x10) == 0 && wd->action_ticks < 1) {
            pulled[0] = (uint8_t)((wd->control_flags >> 1) & 1);
            if ((weapon_tag->weapon_flags & 0x1000) != 0 && (wd->control_flags & 4) != 0) {
                pulled[1] = 1;
            } else {
                pulled[1] = 0;
            }
        } else {
            pulled[1] = 0;
        }

        if (weapon_tag->secondary_trigger_mode == 1) {
            if (pulled[1] != 0 && weapon_tag->triggers.count > 0 && wd->triggers[0].firing_rate != 1.0f) {
                pulled[1] = 0;
            }
        } else if (weapon_tag->secondary_trigger_mode == 2 && pulled[1] != 0) {
            pulled[0] = 0;
        }

        if (item_obj->network_role != 1 && (wd->control_flags & 8) != 0 && weapon_tag->magazines.count > 0) {
            wd->flags = wd->flags | 8; // reuse of _weapon_ammo_prediction_pending_bit UNSURE
        }
        if ((wd->flags & 8) != 0) {
            weapon_trigger_begin_reload(item_index, 0, 1);
        }

        // Magazine recharge / reload-state-machine sweep.
        if (weapon_tag->magazines.count > 0) {
            for (i = 0; i < weapon_tag->magazines.count; i++) {
                WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + i;
                weapon_magazine_state *magazine = &wd->magazines[i];

                if (magazine_tag->rounds_recharged > 0 && magazine->rounds_loaded < magazine_tag->rounds_loaded_maximum) {
                    // Battery recharge. rounds_recharged is a per-second rate, so each tick adds
                    // rate/30 whole rounds and banks rate%30 thirtieths in
                    // rounds_recharged_accumulator (weapon_magazine_state 0x0a, renamed from
                    // unknown_0a this pass: this loop is the only reader or writer of that field
                    // anywhere in the export, and it proves the field is that accumulator). All
                    // of the arithmetic is signed int16, matching `sVar12 % 0x1e` and
                    // `sVar12 / 0x1e` in the original.
                    int16_t rate = magazine_tag->rounds_recharged;
                    int16_t old_loaded = magazine->rounds_loaded;

                    magazine->rounds_recharged_accumulator =
                        (int16_t)(magazine->rounds_recharged_accumulator + rate % 30);
                    magazine->rounds_loaded = (int16_t)(rate / 30 + old_loaded);
                    if (magazine->rounds_recharged_accumulator > 29) {
                        magazine->rounds_loaded = (int16_t)(magazine->rounds_loaded + 1);
                        magazine->rounds_recharged_accumulator =
                            (int16_t)(magazine->rounds_recharged_accumulator - 30);
                    }
                    if (magazine->rounds_loaded > magazine_tag->rounds_loaded_maximum) {
                        magazine->rounds_loaded = magazine_tag->rounds_loaded_maximum;
                    }
                }

                if (magazine->state_ticks != 0) {
                    magazine->state_ticks = magazine->state_ticks - 1;
                }

                if (magazine->state == _weapon_magazine_reloading) {
                    if (magazine->state_ticks == 1 || magazine->state_ticks - 1 < 0) {
                        if (item_obj->network_role == 1) {
                            weapon_magazine_reload_tick_predicted(item_index, i);
                        } else {
                            weapon_magazine_reload_tick(item_index, i);
                        }
                    }
                } else if (magazine->state == _weapon_magazine_chamber_pending) {
                    weapon_magazine_begin_chamber(item_index, i);
                } else if (magazine->state == _weapon_magazine_chambering && magazine->state_ticks == 0) {
                    magazine->state = 0;
                    magazine->state_ticks = 0;
                }
            }
        }

        // Per-trigger firing-effect / heat-error state machine.
        for (local_trigger_index = 0; local_trigger_index < weapon_tag->triggers.count; local_trigger_index++) {
            WeaponTrigger *tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + local_trigger_index;
            weapon_trigger_state *trigger = &wd->triggers[local_trigger_index];
            uint8_t is_pulled = pulled[local_trigger_index];

            if ((tag_trigger->flags & 0x200) != 0 && (id->flags & _item_held_by_player_bit) != 0) {
                pulled[local_trigger_index] = wd->primary_trigger > 0.05f;
                is_pulled = pulled[local_trigger_index];
            }
            if ((tag_trigger->flags & 0x40) != 0 && item_obj->parent_object == (datum_index)0xffffffff) {
                pulled[local_trigger_index] = 1;
                is_pulled = 1;
            }

            if (trigger->effect_state_ticks != 0) {
                trigger->effect_state_ticks = trigger->effect_state_ticks - 1;
            }

            // Tag trigger flag 0x10, a latching trigger: each rising edge of the raw pull
            // toggles _weapon_trigger_latched_bit, the previous pull is remembered in
            // _weapon_trigger_was_pulled_bit, and it is the latch -- not the raw pull -- that
            // the state machine below sees. The original reads, XORs and writes the flags word
            // once for the edge test and then again for the was-pulled update, so the second
            // read observes the toggle; that ordering is reproduced here.
            if ((tag_trigger->flags & 0x10) != 0) {
                if ((trigger->flags & _weapon_trigger_was_pulled_bit) == 0 && is_pulled != 0) {
                    trigger->flags = trigger->flags ^ (uint32_t)_weapon_trigger_latched_bit;
                }
                if (is_pulled == 0) {
                    trigger->flags = trigger->flags & ~(uint32_t)_weapon_trigger_was_pulled_bit;
                } else {
                    trigger->flags = trigger->flags | (uint32_t)_weapon_trigger_was_pulled_bit;
                }
                pulled[local_trigger_index] = (uint8_t)((trigger->flags >> 2) & 1);
                is_pulled = pulled[local_trigger_index];
            }

            if (is_pulled == 0) {
                trigger->flags = trigger->flags | (uint32_t)_weapon_trigger_not_pulled_bit;
            }

            // Both recovery meters decay by their tag rate and stick at zero. The clamp test is
            // Ghidra's `fVar4 < 0.0 != (fVar4 == 0.0)`, i.e. plain `<= 0.0`.
            if (0.0f < trigger->ejection_port_recovery) {
                trigger->ejection_port_recovery -= tag_trigger->ejection_port_recovery_rate;
                if (trigger->ejection_port_recovery <= 0.0f) {
                    trigger->ejection_port_recovery = 0.0f;
                }
            }
            if (0.0f < trigger->illumination_recovery) {
                trigger->illumination_recovery -= tag_trigger->illumination_recovery_rate;
                if (trigger->illumination_recovery <= 0.0f) {
                    trigger->illumination_recovery = 0.0f;
                }
            }

            switch (trigger->effect_state) {
            case 0: {
                int32_t ready = 1;
                if ((wd->control_flags & 0x10) == 0 && item_obj->parent_object != (datum_index)0xffffffff &&
                    tag_trigger->magazine != (uint16_t)-1) {
                    int16_t magazine_index = tag_trigger->magazine;
                    int16_t rounds_loaded = wd->magazines[magazine_index].rounds_loaded;

                    if ((rounds_loaded < tag_trigger->rounds_per_shot && (tag_trigger->flags & 4) == 0) ||
                        rounds_loaded < tag_trigger->minimum_rounds_loaded || rounds_loaded == 0) {
                        int32_t nag = 0;
                        if (item_obj->network_role == 0 && tag_trigger->rounds_per_shot > 0) {
                            int8_t empty_ticks = trigger->empty_ticks + 1;
                            trigger->empty_ticks = empty_ticks;
                            ready = wd->magazines[magazine_index].rounds_unloaded < 1;
                            if (empty_ticks > 10) {
                                ready = 1;
                                nag = 1;
                                trigger->empty_ticks = 0;
                            }
                        }
                        if (item_obj->network_role == 3 || nag) {
                            weapon_trigger_begin_reload(item_index, magazine_index, 1);
                        }
                    }
                }
                if (is_pulled == 0 || weapon_trigger_ready_to_fire(item_index, local_trigger_index) == 0 || !ready) {
                    if (trigger->idle_ticks < 0x7f) {
                        trigger->idle_ticks = trigger->idle_ticks + 1;
                    }
                } else {
                    weapon_trigger_fire_or_reload(item_index, local_trigger_index, 0);
                }
                break;
            }
            case 1:
                if (is_pulled == 0) {
                    weapon_trigger_fire_or_reload(item_index, local_trigger_index, 1);
                } else if (trigger->effect_state_ticks == 0 && wd->alternate_shots_loaded < weapon_tag->maximum_alternate_shots_loaded) {
                    weapon_trigger_continue_burst(item_index, local_trigger_index);
                }
                break;
            case 2:
                if (trigger->effect_state_ticks == 0) {
                    weapon_trigger_become_charged(item_index, local_trigger_index);
                } else if (is_pulled == 0) {
                    if (local_trigger_index == 0 && weapon_tag->triggers.count > 1 && (trigger->flags & 0x20) == 0) {
                        weapon_trigger_fire_or_reload(item_index, 0, 1);
                    } else {
                        trigger->effect_state = 0;
                        trigger->effect_state_ticks = 0;
                    }
                    if (trigger->effect_handle != (datum_index)0xffffffff) {
                        effect_stop(trigger->effect_handle, 1);
                        trigger->effect_handle = (datum_index)0xffffffff;
                    }
                }
                break;
            case 3:
                if (is_pulled == 0) {
                    weapon_trigger_enter_recovery(item_index, local_trigger_index);
                } else {
                    wd->charged_fraction = 1.0f - ((real)trigger->effect_state_ticks * 0.033333335f) / tag_trigger->charged_time;
                    if (trigger->effect_state_ticks == 0) {
                        weapon_trigger_handle_empty(item_index, local_trigger_index);
                    } else {
                        int16_t rounds_loaded = wd->magazines[tag_trigger->magazine].rounds_loaded;
                        if (rounds_loaded < tag_trigger->rounds_per_shot && (tag_trigger->flags & 4) == 0) {
                            weapon_trigger_enter_recovery(item_index, local_trigger_index);
                        }
                    }
                }
                break;
            case 4:
                if (trigger->effect_state_ticks == 0) {
                    if ((tag_trigger->flags & 8) == 0 || (id->flags & _item_held_by_player_bit) == 0 ||
                        (trigger->flags & _weapon_trigger_not_pulled_bit) != 0) {
                        trigger->effect_state = 0;
                        trigger->effect_state_ticks = 0;
                    } else {
                        weapon_trigger_effect_set_out_of_ammo(item_index, local_trigger_index);
                    }
                }
                break;
            case 5:
                if (is_pulled == 0 || wd->tracked_object_index == (datum_index)0xffffffff) {
                    weapon_trigger_reset_tracking(item_index, local_trigger_index);
                }
                break;
            case 6:
                if (trigger->effect_state_ticks != 0) {
                    weapon_trigger_fire_or_reload(item_index, local_trigger_index, 1);
                } else {
                    weapon_trigger_finish_shot(item_index, local_trigger_index);
                }
                break;
            case 7:
                if (is_pulled == 0) {
                    trigger->effect_state = 0;
                    trigger->effect_state_ticks = 0;
                }
                break;
            case 8:
                if (trigger->effect_state_ticks == 0) {
                    trigger->effect_state = 0;
                    trigger->effect_state_ticks = 0;
                }
                break;
            }

            // Firing-rate spin up/down and the blur permutation swap.
            if (is_pulled == 0) {
                real new_rate = trigger->firing_rate - tag_trigger->firing_deceleration_rate;
                trigger->firing_rate = (new_rate < 0.0f) ? 0.0f : new_rate;
                if ((trigger->flags & _weapon_trigger_blur_applied_bit) != 0 &&
                    trigger->firing_rate < tag_trigger->blurred_rate_of_fire) {
                    object_set_permutation_by_name(weapon_blur_target(item_index), weapon_blur_permutation_names[local_trigger_index],
                                                   -1, 0);
                    trigger->flags = trigger->flags & ~(uint32_t)_weapon_trigger_blur_applied_bit;
                }
            } else {
                real new_rate = tag_trigger->firing_acceleration_rate + trigger->firing_rate;
                trigger->firing_rate = (new_rate > 1.0f) ? 1.0f : new_rate;
                if (tag_trigger->blurred_rate_of_fire != 0.0f &&
                    (trigger->flags & _weapon_trigger_blur_applied_bit) == 0 &&
                    tag_trigger->blurred_rate_of_fire < trigger->firing_rate) {
                    object_set_permutation_by_name(weapon_blur_target(item_index), weapon_blur_permutation_names[local_trigger_index],
                                                   -1, 1);
                    trigger->flags = trigger->flags | _weapon_trigger_blur_applied_bit;
                }
            }

            // Accuracy bloom.
            {
                int8_t effect_state = trigger->effect_state;
                if (effect_state == 6 || effect_state == 4 || is_pulled != 0) {
                    real new_error = tag_trigger->error_acceleration_rate + trigger->error;
                    trigger->error = (new_error > 1.0f) ? 1.0f : new_error;
                } else {
                    real new_error = trigger->error - tag_trigger->error_deceleration_rate;
                    trigger->error = (new_error < 0.0f) ? 0.0f : new_error;
                }
            }
        }
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x4c1530):

int __cdecl item_update_triggers(uint item_index)

{
  short *psVar1;
  undefined2 *puVar2;
  float *pfVar3;
  float fVar4;
  float fVar5;
  byte bVar6;
  ushort uVar7;
  short sVar8;
  bool bVar9;
  bool bVar10;
  char cVar11;
  short sVar12;
  int iVar13;
  uint *puVar14;
  uint uVar15;
  int iVar16;
  int iVar17;
  uint *puVar18;
  uint *puVar19;
  byte local_24 [4];
  int local_20;
  int local_1c;
  int local_18;
  uint *local_14;
  uint *local_10;
  int local_c;

  local_18 = (item_index & 0xffff) * 0xc;
  puVar19 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_18);
  iVar13 = (*puVar19 & 0xffff) * 0x20;
  iVar16 = *(int *)(iVar13 + 0x14 + DAT_0087bc14);
  local_1c = iVar16;
  if (DAT_0071c419 == '\x01') goto LAB_004c20d4;
  local_14 = puVar19;
  if ((puVar19[0x94] != 0xffffffff) && (iVar13 = object_try_and_get(0xffffffff), iVar13 == 0)) {
    puVar19[0x94] = 0xffffffff;
  }
  if ((*(int *)(iVar16 + 0x44) != -1) && ((short)puVar19[0x34] != -1)) {
    sVar12 = FUN_004d48d0(1);
    if (sVar12 == 1) {
      FUN_004c5580();
    }
    else if (sVar12 == 2) {
      FUN_004c5630();
    }
  }
  iVar16 = local_1c;
  if (((*(uint *)(local_1c + 0x308) & 0x400) != 0) && (puVar19[0x47] == 0xffffffff)) {
    FUN_004bd450();
  }
  if ((0.0 < (float)puVar19[0x92]) &&
     (((((puVar19[0x47] == 0xffffffff ||
         (puVar14 = (uint *)object_try_and_get(3), puVar14 == (uint *)0x0)) ||
        (*puVar14 == 0xffffffff)) ||
       ((*(uint *)(*(int *)((*puVar14 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x800000)
        == 0)) &&
      (fVar4 = (float)puVar19[0x92], puVar19[0x92] = (uint)(fVar4 - 0.041666668),
      fVar4 - 0.041666668 < 0.0)))) {
    puVar19[0x92] = 0;
  }
  if (0.0 < (float)puVar19[0x8f]) {
    if ((*(float *)(iVar16 + 0x350) <= (float)puVar19[0x8f]) &&
       (uVar15 = puVar19[0x8b], (uVar15 & 1) == 0)) {
      puVar19[0x8b] = uVar15 | 1;
      if ((*(short *)(iVar16 + 0x4e2) == 3) && ((uVar15 & 4) != 0)) {
        puVar19[0x8b] = uVar15 & 0xfffffffb | 1;
      }
      FUN_00492790();
      uVar15 = FUN_004c48a0();
      local_14[0xb3] = uVar15;
      iVar16 = local_1c;
      puVar19 = local_14;
    }
    if ((float)puVar19[0x91] == 0.0) {
      fVar4 = *(float *)(iVar16 + 0x35c) * 0.033333335;
      if (0.0 < *(float *)(iVar16 + 0x440)) {
        fVar4 = (1.0 - (float)puVar19[0x90] * *(float *)(iVar16 + 0x440)) * fVar4;
      }
      fVar5 = (float)puVar19[0x8f];
      puVar19[0x8f] = (uint)(fVar5 - fVar4);
      if (fVar5 - fVar4 < 0.0) {
        puVar19[0x8f] = 0;
      }
      uVar15 = puVar19[0x8b];
      if ((((uVar15 & 1) != 0) && ((uVar15 & 2) == 0)) &&
         (fVar4 = ((float)puVar19[0x8f] - *(float *)(iVar16 + 0x34c)) / fVar4,
         fVar4 < 1.0 != (fVar4 == 1.0))) {
        puVar19[0x8b] = uVar15 | 2;
      }
    }
    if ((((puVar19[0x8b] & 1) != 0) && ((float)puVar19[0x8f] < *(float *)(iVar16 + 0x34c))) &&
       (puVar19[0x8b] = puVar19[0x8b] & 0xfffffffc, puVar19[0xb3] != 0xffffffff)) {
      FUN_00450b20(1);
    }
  }
  puVar19[0x91] = 0;
  if (0 < *(short *)((int)puVar19 + 0x23a)) {
    *(short *)((int)puVar19 + 0x23a) = *(short *)((int)puVar19 + 0x23a) + -1;
  }
  uVar7 = (ushort)puVar19[0x8c];
  if (((uVar7 & 0x10) == 0) && (*(short *)((int)puVar19 + 0x23a) < 1)) {
    local_24[0] = (byte)uVar7 >> 1 & 1;
    if (((*(uint *)(iVar16 + 0x308) & 0x1000) == 0) || ((uVar7 & 4) == 0)) goto LAB_004c184a;
    local_24[1] = '\x01';
  }
  else {
    local_24[0] = 0;
LAB_004c184a:
    local_24[1] = '\0';
  }
  if (*(short *)(iVar16 + 0x32c) == 1) {
    if (((local_24[1] != '\0') && (0 < *(int *)(iVar16 + 0x4fc))) && (puVar19[0x9c] != 0x3f800000))
    {
      local_24[1] = '\0';
    }
  }
  else if ((*(short *)(iVar16 + 0x32c) == 2) && (local_24[1] != '\0')) {
    local_24[0] = 0;
  }
  if (((puVar19[1] != 1) && ((uVar7 & 8) != 0)) && (0 < *(int *)(iVar16 + 0x4f0))) {
    puVar19[0x8b] = puVar19[0x8b] | 8;
  }
  if ((puVar19[0x8b] & 8) != 0) {
    weapon_trigger_begin_reload(item_index,0,1);
  }
  local_14 = (uint *)0x0;
  if (0 < *(int *)(local_1c + 0x4f0)) {
    iVar16 = 0;
    do {
      puVar14 = local_14;
      iVar13 = iVar16 * 0x70 + *(int *)(local_1c + 0x4f4);
      iVar17 = iVar16 * 0xc;
      if ((0 < *(short *)(iVar13 + 4)) &&
         ((short)puVar19[iVar16 * 3 + 0xae] < *(short *)(iVar13 + 10))) {
        sVar12 = *(short *)(iVar13 + 4);
        uVar15 = puVar19[iVar16 * 3 + 0xae];
        psVar1 = (short *)(iVar17 + 0x2ba + (int)puVar19);
        *psVar1 = *psVar1 + sVar12 % 0x1e;
        *(short *)(puVar19 + iVar16 * 3 + 0xae) = sVar12 / 0x1e + (short)uVar15;
        sVar12 = *(short *)(iVar17 + 0x2ba + (int)puVar19);
        if (0x1d < sVar12) {
          *(short *)(puVar19 + iVar16 * 3 + 0xae) = (short)puVar19[iVar16 * 3 + 0xae] + 1;
          *(short *)(iVar17 + 0x2ba + (int)puVar19) = sVar12 + -0x1e;
        }
        if (*(short *)(iVar13 + 10) < (short)puVar19[iVar16 * 3 + 0xae]) {
          *(short *)(puVar19 + iVar16 * 3 + 0xae) = *(short *)(iVar13 + 10);
        }
      }
      psVar1 = (short *)(iVar17 + 0x2b2 + (int)puVar19);
      if (*psVar1 != 0) {
        *psVar1 = *psVar1 + -1;
      }
      sVar12 = (short)puVar19[iVar16 * 3 + 0xac];
      if (sVar12 == 1) {
        if (*psVar1 == 1 || *psVar1 + -1 < 0) {
          if (puVar19[1] == 1) {
            FUN_004c3a20(item_index);
          }
          else {
            FUN_004c3900(item_index);
          }
        }
      }
      else if (sVar12 == 2) {
        FUN_004c3b00(item_index);
      }
      else if ((sVar12 == 3) && (*psVar1 == 0)) {
        puVar2 = (undefined2 *)
                 (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_18) + 0x2b0 + iVar17);
        *puVar2 = 0;
        puVar2[1] = 0;
      }
      local_14 = (uint *)((int)puVar14 + 1);
      iVar16 = (int)(short)local_14;
    } while (iVar16 < *(int *)(local_1c + 0x4f0));
  }
  local_20 = 0;
  iVar13 = 0;
  if (0 < *(int *)(local_1c + 0x4fc)) {
    local_14 = (uint *)0x0;
    do {
      puVar14 = local_14;
      puVar18 = (uint *)((int)local_14 * 0x114 + *(int *)(local_1c + 0x500));
      uVar15 = *puVar18;
      iVar16 = (int)local_14 * 0x28;
      local_10 = puVar18;
      if (((uVar15 & 0x200) != 0) && ((puVar19[0x7d] & 2) != 0)) {
        local_24[(int)local_14] = 0.05 < (float)puVar19[0x8d];
      }
      if (((uVar15 & 0x40) != 0) && (puVar19[0x47] == 0xffffffff)) {
        local_24[(int)puVar14] = 1;
      }
      sVar12 = *(short *)(iVar16 + 0x262 + (int)puVar19);
      if (sVar12 != 0) {
        *(short *)(iVar16 + 0x262 + (int)puVar19) = sVar12 + -1;
      }
      if ((*puVar18 & 0x10) != 0) {
        if (((puVar19[(int)puVar14 * 10 + 0x99] & 2) == 0) && (local_24[(int)puVar14] != 0)) {
          puVar19[(int)puVar14 * 10 + 0x99] = puVar19[(int)puVar14 * 10 + 0x99] ^ 4;
        }
        if (local_24[(int)puVar14] == 0) {
          uVar15 = puVar19[(int)puVar14 * 10 + 0x99] & 0xfffffffd;
        }
        else {
          uVar15 = puVar19[(int)puVar14 * 10 + 0x99] | 2;
        }
        local_24[(int)puVar14] = (byte)(uVar15 >> 2) & 1;
        puVar19[(int)puVar14 * 10 + 0x99] = uVar15;
      }
      bVar6 = local_24[(int)puVar14];
      if (bVar6 == 0) {
        puVar19[(int)puVar14 * 10 + 0x99] = puVar19[(int)puVar14 * 10 + 0x99] | 1;
      }
      if (0.0 < (float)puVar19[(int)puVar14 * 10 + 0x9d]) {
        fVar4 = (float)puVar19[(int)puVar14 * 10 + 0x9d] - (float)puVar18[0x3d];
        puVar19[(int)puVar14 * 10 + 0x9d] = (uint)fVar4;
        if (fVar4 < 0.0 != (fVar4 == 0.0)) {
          puVar19[(int)puVar14 * 10 + 0x9d] = 0;
        }
      }
      if (0.0 < (float)puVar19[(int)puVar14 * 10 + 0x9e]) {
        fVar4 = (float)puVar19[(int)puVar14 * 10 + 0x9e] - (float)puVar18[0x3c];
        puVar19[(int)puVar14 * 10 + 0x9e] = (uint)fVar4;
        if (fVar4 < 0.0 != (fVar4 == 0.0)) {
          puVar19[(int)puVar14 * 10 + 0x9e] = 0;
        }
      }
      switch(*(undefined1 *)(iVar16 + 0x261 + (int)puVar19)) {
      case 0:
        bVar9 = true;
        if ((((puVar19[0x8c] & 0x10) == 0) && (puVar19[0x47] != 0xffffffff)) &&
           (sVar12 = (short)puVar18[8], sVar12 != -1)) {
          sVar8 = (short)puVar19[sVar12 * 3 + 0xae];
          if (((sVar8 < *(short *)((int)puVar18 + 0x22)) && ((*puVar18 & 4) == 0)) ||
             ((sVar8 < (short)puVar18[9] || (sVar8 == 0)))) {
            bVar10 = false;
            if ((puVar19[1] == 0) && (0 < *(short *)((int)puVar18 + 0x22))) {
              cVar11 = (char)puVar19[(int)puVar14 * 10 + 0xa1] + '\x01';
              *(char *)(puVar19 + (int)puVar14 * 10 + 0xa1) = cVar11;
              bVar9 = *(short *)((int)puVar19 + sVar12 * 0xc + 0x2b6) < 1;
              if ('\n' < cVar11) {
                bVar9 = true;
                bVar10 = true;
                *(undefined1 *)(puVar19 + (int)puVar14 * 10 + 0xa1) = 0;
              }
            }
            if ((puVar19[1] == 3) || (bVar10)) {
              weapon_trigger_begin_reload(item_index,(short)puVar18[8],1);
            }
          }
        }
        if (((bVar6 == 0) || (cVar11 = FUN_004c3190(local_20), cVar11 == '\0')) || (!bVar9)) {
          if ((char)puVar19[(int)puVar14 * 10 + 0x98] < '\x7f') {
            *(char *)(puVar19 + (int)puVar14 * 10 + 0x98) =
                 (char)puVar19[(int)puVar14 * 10 + 0x98] + '\x01';
          }
        }
        else {
          FUN_004c3280(item_index,local_20,0);
        }
        break;
      case 1:
        if (bVar6 == 0) {
LAB_004c1cd7:
          FUN_004c3280(item_index,local_20,1);
        }
        else if ((*(short *)(iVar16 + 0x262 + (int)puVar19) == 0) &&
                ((short)puVar19[0x97] < *(short *)(local_1c + 0x32e))) {
          FUN_004c3c60();
        }
        break;
      case 2:
        if (*(short *)(iVar16 + 0x262 + (int)puVar19) == 0) {
          FUN_004c3bc0(local_20);
        }
        else if (bVar6 == 0) {
          if ((((short)local_20 == 0) && (1 < *(int *)(local_1c + 0x4fc))) &&
             ((puVar19[(int)puVar14 * 10 + 0x99] & 0x20) == 0)) {
            FUN_004c3280(item_index,0,1);
          }
          else {
            iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_18);
            *(undefined1 *)(iVar13 + 0x261 + iVar16) = 0;
            *(undefined2 *)(iVar13 + 0x262 + iVar16) = 0;
          }
          if (puVar19[(int)puVar14 * 10 + 0xa0] != 0xffffffff) {
            FUN_00450b20(1);
            puVar19[(int)puVar14 * 10 + 0xa0] = 0xffffffff;
          }
        }
        break;
      case 3:
        if (bVar6 == 0) {
          FUN_004c3d00(item_index,local_20);
        }
        else {
          local_c = (int)*(short *)(iVar16 + 0x262 + (int)puVar19);
          puVar19[0x91] = (uint)(1.0 - ((float)local_c * 0.033333335) / (float)puVar18[0x13]);
          if (*(short *)(iVar16 + 0x262 + (int)puVar19) == 0) {
            FUN_004c3de0();
          }
          else if (((short)puVar19[((short)puVar18[8] + 0x3a) * 3] < *(short *)((int)puVar18 + 0x22)
                   ) && ((*puVar18 & 4) == 0)) {
            FUN_004c3d00(item_index,local_20);
          }
        }
        break;
      case 4:
        if (*(short *)(iVar16 + 0x262 + (int)puVar19) == 0) {
          if ((((*puVar18 & 8) == 0) || ((puVar19[0x7d] & 2) == 0)) ||
             ((puVar19[(int)puVar14 * 10 + 0x99] & 1) != 0)) goto LAB_004c1eb8;
          FUN_004c3e70(local_20);
        }
        break;
      case 5:
        if ((bVar6 == 0) || (puVar19[0x94] == 0xffffffff)) {
          FUN_004c3eb0(local_20);
        }
        break;
      case 6:
        if (*(short *)(iVar16 + 0x262 + (int)puVar19) != 0) goto LAB_004c1cd7;
        FUN_004c48f0(local_20);
        break;
      case 7:
        if (bVar6 == 0) {
LAB_004c1eb8:
          iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_18);
LAB_004c1ee5:
          *(undefined1 *)(iVar13 + 0x261 + iVar16) = 0;
          *(undefined2 *)(iVar13 + 0x262 + iVar16) = 0;
        }
        break;
      case 8:
        if (*(short *)(iVar16 + 0x262 + (int)puVar19) == 0) {
          iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_18);
          goto LAB_004c1ee5;
        }
      }
      puVar18 = local_10;
      if (bVar6 == 0) {
        fVar4 = (float)puVar19[(int)puVar14 * 10 + 0x9c];
        fVar5 = (float)local_10[0x3f];
        puVar19[(int)puVar14 * 10 + 0x9c] = (uint)(fVar4 - fVar5);
        if (fVar4 - fVar5 < 0.0) {
          puVar19[(int)puVar14 * 10 + 0x9c] = 0;
        }
        if (((puVar19[(int)puVar14 * 10 + 0x99] & 0x10) != 0) &&
           ((float)puVar19[(int)puVar14 * 10 + 0x9c] < (float)local_10[5])) {
          object_set_permutation_by_name
                    ((&PTR_s__primary_blur_006961b8)[(int)local_14],0xffffffff,0);
          uVar15 = puVar19[(int)puVar14 * 10 + 0x99] & 0xffffffef;
          goto LAB_004c2047;
        }
      }
      else {
        fVar4 = (float)local_10[0x3e];
        fVar5 = (float)puVar19[(int)puVar14 * 10 + 0x9c];
        puVar19[(int)puVar14 * 10 + 0x9c] = (uint)(fVar4 + fVar5);
        if (1.0 < fVar4 + fVar5) {
          puVar19[(int)puVar14 * 10 + 0x9c] = 0x3f800000;
        }
        if ((((float)local_10[5] != 0.0) && ((puVar19[(int)puVar14 * 10 + 0x99] & 0x10) == 0)) &&
           ((float)local_10[5] < (float)puVar19[(int)puVar14 * 10 + 0x9c])) {
          object_set_permutation_by_name
                    ((&PTR_s__primary_blur_006961b8)[(int)local_14],0xffffffff,1);
          uVar15 = puVar19[(int)puVar14 * 10 + 0x99] | 0x10;
LAB_004c2047:
          puVar19[(int)puVar14 * 10 + 0x99] = uVar15;
        }
      }
      cVar11 = *(char *)(iVar16 + 0x261 + (int)puVar19);
      if (((cVar11 == '\x06') || (cVar11 == '\x04')) || (bVar6 != 0)) {
        pfVar3 = (float *)(puVar19 + (int)puVar14 * 10 + 0x9f);
        fVar4 = (float)puVar18[0x40] + *pfVar3;
        *pfVar3 = fVar4;
        if (1.0 < fVar4) {
          *pfVar3 = 1.0;
        }
      }
      else {
        fVar4 = (float)puVar19[(int)puVar14 * 10 + 0x9f] - (float)puVar18[0x41];
        puVar19[(int)puVar14 * 10 + 0x9f] = (uint)fVar4;
        if (fVar4 < 0.0) {
          puVar19[(int)puVar14 * 10 + 0x9f] = 0;
        }
      }
      local_14 = (uint *)(int)(short)(local_20 + 1);
      local_20 = local_20 + 1;
      iVar13 = local_1c;
    } while ((int)local_14 < *(int *)(local_1c + 0x4fc));
  }
LAB_004c20d4:
  return CONCAT31((int3)((uint)iVar13 >> 8),1);
}
#endif
