// encounter_choose_vocalizations  (Ghidra: encounter_choose_vocalizations; named for this rewrite)
// address 0x438580, size 2079 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: phase-4 summary ("scores nearby positions/targets to choose a retreat or regroup
//   destination for a squad under morale pressure"). It is the only caller of
//   ai_insert_scored_candidate_pair @0x4383f0 and ai_pick_weighted_candidate @0x438480, and
//   everything it produces is written to actor.command_status / unknown_1e8, the pair
//   encounter_recompute_morale reads back as "this member has a pending vocalization". The
//   line ids come from the int16 table at 0x00657194.
// register convention: plain __cdecl, one stack argument.
//   // blam-cc: stack -> encounter_index
//
// UNSURE (high) - this is the least certain rewrite of this batch:
//  - The 0x80-byte candidate table is four buckets of two ai_scored_candidate entries; the
//    bucket a prop lands in is chosen by the three cases below (2 = non-unit prop, 0 = a unit
//    the actor tag does not flag with 0x40 and whose prop + 0x76 is under 210, 1 = anything
//    else), and bucket 3 is reserved for the "has a unit prop at all" summary entry.
//  - Ghidra types the chosen-actor handle (local_cc) as a float and renders the null test as
//    `!= -NAN`; it is a datum_index compared against none. Written as such here.
//  - prop + 0x76 is scaled by 1/240 (0.004166667) and clamped to 1.0, and prop.distance
//    (+0x11c) is the range gate. actor + 0x1b8 / + 0x3b4 are the two morale floats.
//  - The distance search near the end passes the CHOSEN actor's unit index to
//    actor_find_prop_for_object on every iteration, not the member being examined. That is
//    what the original does; it is loop-invariant and looks like a Bungie slip.
//  - The 0x00657194 table is read as `table[bucket_index]`, so it has at least four int16
//    entries; its contents were not dumped.
//  - ai_communication_rate_player_proximity (0x4303f0) is called with (1, 0, 0) at both
//    sites and returns an x87 value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"

extern data_array *encounter_data;   // 0x008802c8
extern ai_globals *ai_globals_ptr;   // 0x00880354
extern data_array *actor_data;       // 0x00880360
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *prop_data;        // 0x008802c0
extern data_array *object_data;      // 0x008603b0
extern uint8_t *actor_type_procs[]; // 0x006853b8, one definition pointer per actor type (+0x4 flags byte)
extern int16_t ai_vocalization_line_table[4]; // 0x00657194, UNSURE length

extern float ai_communication_rate_player_proximity(uint8_t require_line_of_sight, datum_index *out_player_object_index,
    float *out_distance, datum_index object_index); // 0x4303f0, EBX object (the actor's unit), stack (1, 0, 0)
extern uint8_t ai_insert_scored_candidate_pair(ai_scored_candidate *list, datum_index handle,
    float score, datum_index payload, datum_index key);            // 0x4383f0
extern int16_t ai_pick_weighted_candidate(ai_scored_candidate *table,
    ai_scored_candidate *out_entry);                               // 0x438480, blam-cc: EBX -> table
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, ECX actor, stack object

// blam-cc: stack -> encounter_index
// Picks up to two "somebody should say something about this" candidates out of the
// encounter's members and their perceived props, plus one morale-driven line for the member
// with the strongest player proximity, and stamps the chosen line ids onto those actors.
void encounter_choose_vocalizations(datum_index encounter_index)
{
    encounter *enc;
    actor *a;
    actor *chosen;
    prop *p;
    object *obj;
    uint8_t *actor_definition;
    datum_index actor_index;
    datum_index current;
    datum_index prop_index;
    datum_index next_prop;
    datum_index chosen_actor;
    datum_index nearest_actor;
    datum_index morale_actor;
    datum_index nearest_prop;
    ai_scored_candidate buckets[8];   // four buckets of two entries
    ai_scored_candidate picked[2];
    int16_t picked_bucket[2];
    int16_t morale_line;
    float proximity;
    float range;
    float scale;
    float boost;
    float freshness;
    float score;
    float bias;
    float best_distance;
    float dx;
    float dy;
    float dz;
    float distance;
    int16_t bucket;
    int16_t i;
    int16_t head_count;
    int32_t margin;
    uint8_t has_unit_prop;
    uint8_t any_candidate;
    uint8_t still_any;
    uint8_t inserted;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];

    for (i = 0; i < 8; i = i + 1) {
        buckets[i].handle = (datum_index)k_datum_index_none;
        buckets[i].score = 0.0f;
        buckets[i].payload = (datum_index)k_datum_index_none;
        buckets[i].key = (datum_index)k_datum_index_none;
    }
    picked_bucket[0] = -1;
    picked_bucket[1] = -1;
    morale_line = -1;
    nearest_actor = (datum_index)k_datum_index_none;
    morale_actor = (datum_index)k_datum_index_none;
    nearest_prop = (datum_index)k_datum_index_none;
    any_candidate = 0;

    actor_index = (datum_index)k_datum_index_none;
    if (ai_globals_ptr->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = ai_globals_ptr->unknown_08;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];
        actor_definition = (uint8_t *)tag_instances[a->actor_definition_tag & 0xffff].data;
        actor_index = a->next_in_encounter;
        has_unit_prop = 0;

        if (a->unit_index != (datum_index)k_datum_index_none) {
            proximity = ai_communication_rate_player_proximity(1, 0, 0, a->unit_index); // EBX = a->unit_index

            next_prop = a->first_prop;
            while (next_prop != (datum_index)k_datum_index_none) {
                prop_index = next_prop;
                p = &((prop *)prop_data->data)[prop_index & 0xffff];
                next_prop = p->next_in_actor;

                if (p->is_vault == 0) {
                    continue;
                }

                if (p->is_unit == 0) {
                    range = 9.0f;
                    bucket = 2;
                    bias = 0.4f;
                } else if ((actor_definition[4] & 0x40) == 0 && p->unknown_76 < 0xd2) {
                    range = 10.0f;
                    bucket = 0;
                    bias = 0.7f;
                } else {
                    range = 5.0f;
                    bucket = 1;
                    bias = 0.0f;
                }

                if (p->distance < range) {
                    scale = range / p->distance;
                    if (2.0f <= scale) {
                        scale = 2.0f;
                    }
                    boost = proximity;
                    if (proximity <= 1.5f) {
                        boost = 1.5f;
                    }
                    freshness = (float)(int32_t)p->unknown_76 * 0.004166667f;
                    if (1.0f <= freshness) {
                        freshness = 1.0f;
                    }
                    score = (2.0f - freshness) * boost * scale + bias;
                    if (p->is_parented != 0) {
                        score = score + 2.0f;
                    }
                    if (p->is_unit != 0) {
                        has_unit_prop = 1;
                    }
                    inserted = ai_insert_scored_candidate_pair(&buckets[bucket * 2], current,
                        score, prop_index, p->object_index);
                    if (inserted != 0) {
                        any_candidate = 1;
                    }
                }
            }

            if (has_unit_prop != 0) {
                obj = ((object_header *)object_data->data)[a->unit_index & 0xffff].data;
                head_count = *(int16_t *)((uint8_t *)obj + 0x42a);
                inserted = ai_insert_scored_candidate_pair(&buckets[3 * 2], current,
                    (float)(int32_t)head_count * 0.7f + proximity,
                    (datum_index)k_datum_index_none, (datum_index)k_datum_index_none);
                if (inserted != 0) {
                    any_candidate = 1;
                }
            }
        }
    }

    if (any_candidate != 0) {
        bucket = ai_pick_weighted_candidate(buckets, &picked[0]);
        picked_bucket[0] = bucket;

        if (enc->team != 2 || 7 < enc->unknown_4c) {
            still_any = 0;
            for (i = 0; i < 4; i = i + 1) {
                ai_scored_candidate *pair = &buckets[i * 2];
                if (i == bucket) {
                    pair[0].handle = (datum_index)k_datum_index_none;
                    pair[0].score = 0.0f;
                    pair[0].payload = (datum_index)k_datum_index_none;
                    pair[0].key = (datum_index)k_datum_index_none;
                    pair[1] = pair[0];
                } else if (pair[0].handle == picked[0].handle ||
                           pair[0].key == picked[0].key) {
                    pair[0] = pair[1];
                    pair[1].handle = (datum_index)k_datum_index_none;
                    pair[1].score = 0.0f;
                    pair[1].payload = (datum_index)k_datum_index_none;
                    pair[1].key = (datum_index)k_datum_index_none;
                }
                if (pair[0].handle != (datum_index)k_datum_index_none) {
                    still_any = 1;
                }
            }
            if (still_any != 0) {
                picked_bucket[1] = ai_pick_weighted_candidate(buckets, &picked[1]);
            }
        }
    }

    if (enc->team != 2 || 3 < enc->unknown_4c) {
        chosen_actor = (datum_index)k_datum_index_none;
        best_distance = 0.0f;

        actor_index = (datum_index)k_datum_index_none;
        if (ai_globals_ptr->actors_valid != 0) {
            if (encounter_index == (datum_index)k_datum_index_none) {
                actor_index = ai_globals_ptr->unknown_08;
            } else {
                actor_index = enc->first_actor;
            }
        }
        while (ai_globals_ptr->actors_valid != 0 &&
               actor_index != (datum_index)k_datum_index_none) {
            current = actor_index;
            a = &((actor *)actor_data->data)[current & 0xffff];
            actor_index = a->next_in_encounter;
            if (a->unit_index != (datum_index)k_datum_index_none) {
                proximity = ai_communication_rate_player_proximity(1, 0, 0, a->unit_index); // EBX = a->unit_index
                if ((actor_type_procs[a->type][4] & 2) != 0 /* 0x438a6c: byte +0x4 of the type definition */ && 2.0f < proximity &&
                    best_distance < proximity) {
                    best_distance = proximity;
                    chosen_actor = current;
                }
            }
        }

        morale_actor = chosen_actor;
        if (chosen_actor != (datum_index)k_datum_index_none) {
            chosen = &((actor *)actor_data->data)[chosen_actor & 0xffff];

            if (0.5f <= chosen->unknown_1b8 ||
                chosen->unknown_3b4 - chosen->unknown_1b8 <= 0.3f) {
                head_count = enc->weighted_actor_count;
                if (head_count == 1 && 1 < enc->unknown_1a) {
                    morale_line = 1;
                } else {
                    if (1 < head_count) {
                        margin = 2;
                        if (head_count < 3) {
                            margin = (int32_t)head_count;
                        }
                        if ((int32_t)head_count + margin <= (int32_t)enc->unknown_1a) {
                            morale_line = 4;
                            goto stamp;
                        }
                        if (1 < head_count && (int32_t)enc->unknown_1a - 1 <= (int32_t)head_count) {
                            morale_line = 5;
                            goto stamp;
                        }
                    }
                    if (0.8f < chosen->unknown_1b8) {
                        morale_line = 2;
                    }
                }
            } else {
                best_distance = 3.4028235e+38f;
                nearest_actor = (datum_index)k_datum_index_none;
                morale_line = 3;
                nearest_prop = (datum_index)k_datum_index_none;

                actor_index = (datum_index)k_datum_index_none;
                if (ai_globals_ptr->actors_valid != 0) {
                    if (encounter_index == (datum_index)k_datum_index_none) {
                        actor_index = ai_globals_ptr->unknown_08;
                    } else {
                        actor_index = enc->first_actor;
                    }
                }
                while (ai_globals_ptr->actors_valid != 0 &&
                       actor_index != (datum_index)k_datum_index_none) {
                    datum_index found;
                    current = actor_index;
                    a = &((actor *)actor_data->data)[current & 0xffff];
                    actor_index = a->next_in_encounter;
                    if (a->unit_index == (datum_index)k_datum_index_none ||
                        current == chosen_actor) {
                        continue;
                    }
                    dx = a->aim_origin.x - chosen->aim_origin.x;
                    dy = a->aim_origin.y - chosen->aim_origin.y;
                    dz = a->aim_origin.z - chosen->aim_origin.z;
                    distance = dx * dx + dy * dy + dz * dz;
                    if (best_distance != 3.4028235e+38f &&
                        distance >= best_distance * best_distance) {
                        continue;
                    }
                    // note: the chosen actor's unit, not this member's, exactly as compiled
                    found = actor_find_prop_for_object(chosen->unit_index, current); // 0x438c8e: ECX = this member (EBX)
                    if (found == (datum_index)k_datum_index_none) {
                        continue;
                    }
                    best_distance = distance;
                    nearest_actor = current;
                    nearest_prop = found;
                }
                if (nearest_actor == (datum_index)k_datum_index_none) {
                    nearest_prop = (datum_index)k_datum_index_none;
                }
            }
        }
    }

stamp:
    for (i = 0; i < 2; i = i + 1) {
        if (picked_bucket[i] != -1 && picked[i].handle != (datum_index)k_datum_index_none) {
            a = &((actor *)actor_data->data)[picked[i].handle & 0xffff];
            a->command_status = ai_vocalization_line_table[picked_bucket[i]];
            a->unknown_1e8 = picked[i].payload;
        }
    }

    if (morale_line != -1 && morale_actor != (datum_index)k_datum_index_none) {
        a = &((actor *)actor_data->data)[morale_actor & 0xffff];
        a->command_status = morale_line;
        a->unknown_1e8 = (datum_index)k_datum_index_none;
        if (nearest_actor != (datum_index)k_datum_index_none) {
            a = &((actor *)actor_data->data)[nearest_actor & 0xffff];
            a->command_status = 6;
            a->unknown_1e8 = nearest_prop;
        }
    }

    enc->unknown_47 = 1;
    enc->unknown_48 = 0;
    enc->unknown_4a = 0x78;
    enc->unknown_4c = 0;
}

#if 0
Original Ghidra decompilation (0x438580) -- abridged to the parts this rewrite reproduces;
the full listing is in out/phase2/ai and via `python tools/pack.py 0x438580`:

void FUN_00438580(uint param_1)

{
  ...
  local_80[0] = 0xffffffff;
  local_80[1] = 0;
  local_80[2] = 0xffffffff;
  local_80[3] = 0xffffffff;
  puVar15 = local_80;
  puVar16 = local_80 + 4;
  for (iVar12 = 0x1c; iVar12 != 0; iVar12 = iVar12 + -1) {
    *puVar16 = *puVar15;
    puVar15 = puVar15 + 1;
    puVar16 = puVar16 + 1;
  }
  local_b8[0] = -1;
  local_b8[1] = -1;
  local_c4 = -1;
  local_a4 = 0xffffffff;
  local_a8 = 0xffffffff;
  bVar7 = false;
  ...
  while ((*(char *)(DAT_00880354 + 1) != '\0' && (local_b0 = local_ac, local_ac != 0xffffffff))) {
    ...
      while (uVar17 = uVar21, uVar17 != 0xffffffff) {
        ...
        if (*(char *)(iVar13 + 0x127 + *(int *)(DAT_008802c0 + 0x34)) != '\0') {
          if (*(char *)(iVar14 + 0x60) == '\0') {
            fVar3 = 9.0; sVar9 = 2; local_cc = 0.4;
          }
          else if (((*(byte *)(local_c8 + 4) & 0x40) == 0) && (*(short *)(iVar14 + 0x76) < 0xd2)) {
            fVar3 = 10.0; sVar9 = 0; local_cc = 0.7;
          }
          else {
            fVar3 = 5.0; sVar9 = 1; local_cc = 0.0;
          }
          if (*(float *)(iVar14 + 0x11c) < fVar3) {
            fVar3 = fVar3 / *(float *)(iVar14 + 0x11c);
            if (2.0 <= fVar3) fVar3 = 2.0;
            fVar4 = fVar2;
            if (fVar2 <= 1.5) fVar4 = 1.5;
            fVar5 = (float)(int)*(short *)(iVar14 + 0x76) * 0.004166667;
            if (1.0 <= fVar5) fVar5 = 1.0;
            local_d0 = (2.0 - fVar5) * fVar4 * fVar3 + local_cc;
            if (*(char *)(iVar14 + 0x12e) != '\0') local_d0 = local_d0 + 2.0;
            if (*(char *)(iVar14 + 0x60) != '\0') bVar6 = true;
            cVar8 = FUN_004383f0(local_80 + sVar9 * 8,local_b0,local_d0,uVar17,
                                 *(undefined4 *)(iVar14 + 0x18));
            if (cVar8 != '\0') bVar7 = true;
          }
        }
      }
      if (bVar6) {
        local_c8 = (int)*(short *)(... + 0x42a);
        cVar8 = FUN_004383f0(local_20,local_b0,(float)local_c8 * 0.7 + fVar2,0xffffffff,0xffffffff);
        if (cVar8 != '\0') bVar7 = true;
      }
  }
  if (bVar7) {
    sVar9 = FUN_00438480(local_a0);
    local_b8[0] = sVar9;
    if ((*(short *)(iVar1 + 2) != 2) || (7 < *(short *)(iVar1 + 0x4c))) {
      ... bucket compaction, then
      if (bVar7) { uVar10 = FUN_00438480(local_90); local_b8[1] = uVar10; }
    }
  }
  if ((*(short *)(iVar1 + 2) != 2) || (3 < *(short *)(iVar1 + 0x4c))) {
    local_cc = -NAN;
    local_d0 = 0.0;
    ... pick the member with the strongest proximity and actor_type_procs[type][4] & 2
    local_a4 = (uint)local_cc;
    if (local_cc != -NAN) {
      ... the two morale branches, the nearest-member search and FUN_0043ea80
    }
  }
LAB_00438cc9:
  ... stamp actor+0x1e4 / +0x1e8 from local_b8[] / local_a0[]
  if ((local_c4 != -1) && (local_a4 != 0xffffffff)) { ... }
  *(undefined1 *)(iVar1 + 0x47) = 1;
  *(undefined1 *)(iVar1 + 0x48) = 0;
  *(undefined2 *)(iVar1 + 0x4a) = 0x78;
  *(undefined2 *)(iVar1 + 0x4c) = 0;
  return;
}
#endif
