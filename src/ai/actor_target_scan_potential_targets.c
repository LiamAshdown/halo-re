// actor_target_scan_potential_targets  (Ghidra: actor_target_scan_potential_targets; named per
//   ai_target_distance_qsort_compare.c, which documents itself as the qsort comparator this
//   function passes to _qsort)
// address 0x41d7e0, size 2847 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED end to end against objdump 0x41d7e0..0x41e2fe (calls, acceptance tree, both finalize passes))
// evidence: types/ai.h actor.first_prop/swarm/swarm_index/cluster_unit_index/unknown_3a0/
//   unknown_3a4 (already-named fields, several cross-referenced from ai_types_notes.md
//   directly at this function's address) and prop.kind/owner_actor_index/distance/is_unit/
//   is_vault/is_parented/pair_index/unknown_63/unknown_6a/unknown_76/unknown_20; types/tags.h
//   ScenarioStructureBSP.clusters/cluster_data (confirmed by offsetof() against types/tags.h:
//   clusters at 0x134, cluster_data at 0x140, cluster_data.pointer therefore at 0x14c);
//   types/objects.h object.cluster_stamp (0x14, "compared against 0x008603cc" -- the exact
//   global this function bumps), object_cluster_reference, object_placement_cursor,
//   object_globals.collecting_in_clusters; types/units.h unit_data.swarm_next_unit_index
//   (0x1fc, the "chained through" field types/ai.h's own cluster_unit_index comment names);
//   sibling files that already resolve several of this function's exact callees:
//   object_get_root_parent_placement (0x4f5f70) and, most usefully,
//   object_test_in_atmosphere_zone.c (0x4f76e0), which walks the identical
//   object_get_root_parent_placement -> object_cluster_reference chain against a PVS bitmask
//   and is the template the chain-walk below is built from; actor_target_data_release.c
//   (0x41b980), which establishes the exact actor_replace_object_reference / actor_unlink_prop /
//   datum_delete(prop_data, pair_index) idiom this function's two "drop a candidate" sites
//   reuse; actor_get_current_mode_combat_grade (0x40e760, called here with no visible operand,
//   exactly as actor_process_order_request.c's own call site shows); actor_find_or_allocate_prop and
//   actor_target_data_refresh (0x41c4b0), whose signatures actor_target_evaluate_squad_link.c
//   (0x41e320, itself called at this function's tail) already establishes and this file reuses
//   verbatim.
// register convention: actor index is a genuine stack parameter (Ghidra recognizes `param_1`
//   directly, not an unresolved register read).
// blam-cc: stack -> actor_index
//
// UNSURE, substantially: the control flow inside the main scan loop is a dense tree of
// re-convergent gotos (three separate paths land on the same two labels, one of which then
// falls through into a shared "reset" statement only on some of those paths) and the two
// candidate-list finalize passes at the tail are themselves goto-driven early-exit loops. Both
// are preserved as gotos rather than restructured, the same choice actor_target_evaluate_
// squad_link.c documents for the same reason: flattening this by hand risks silently changing
// which branch a given combination of conditions falls into, and this batch has no disassembly
// budget left to cross-check a restructure the way actor_scan_allies_for_backup_request.c did.
// UNSURE: the two local candidate lists (Ghidra's local_c08+local_c04 and local_604+local_600)
// have no header type; TYPES-GAP structs are declared locally below. Which list is "list_a"
// (selected for a prop with is_unit set, finalized with a hard cap of 4 and actor_find_or_allocate_prop's
// flag=1) versus "list_b" (is_unit clear, a cap of max(seen+2,4) and flag=0) is read directly
// off the two finalize passes; no stronger name than that is asserted.
// UNSURE: object_placement_cursor.cluster_globals[2] is exactly the same "which of the two
// three-global families applies" slot object_get_root_parent_placement.c and
// object_test_in_atmosphere_zone.c both flag as unresolved; reused here rather than re-litigated.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern data_array *swarm_data;      // 0x0088035c
extern data_array *object_data;     // 0x008603b0
extern data_array *encounter_data;  // 0x008802c8
extern datum_index *collideable_cluster_first;    // 0x008603d0
extern data_array *collideable_object_references; // 0x008603d4
extern datum_index *noncollideable_cluster_first;    // 0x008603c0
extern data_array *noncollideable_object_references; // 0x008603c4
extern ScenarioStructureBSP *global_structure_bsp;    // 0x00746f9c
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern int32_t object_cluster_stamp;           // 0x008603cc

extern void _qsort(void *base, int32_t count, int32_t size,
                   int32_t (*cmp)(const void *, const void *)); // 0x623410
extern int ai_target_distance_qsort_compare(void *record_a, void *record_b); // 0x41d7a0, this module

extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index); // 0x40e760, sibling session
extern int16_t object_get_root_parent_placement(uint32_t object_index,
    object_placement_cursor *out_cursor); // 0x4f5f70, blam-cc: EAX -> object_index, ESI -> out_cursor
extern void actor_target_evaluate_squad_link(uint32_t actor_index, datum_index object_cursor,
    int16_t *candidates_a, int16_t *candidates_b); // 0x41e320, this module
extern datum_index actor_find_or_allocate_prop(uint32_t actor_index, datum_index object_index, char flag); // 0x43e270, UNSURE signature
extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force, char allow_reassign); // 0x41c4b0, this batch, UNSURE signature
extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference); // 0x428470, stack, ESI, EDI
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove); // 0x43ea20, EAX, EDI
extern void datum_delete(data_array *array, datum_index handle);  // 0x4d0510

// TYPES-GAP: the local sort record ai_target_distance_qsort_compare.c already documents
// (object_index, prop_index, distance), and the packed {seen_count, entry_count} + fixed
// 128-entry array this function builds it into (Ghidra's local_c08/local_c04 and
// local_604/local_600, each pair adjacent on the stack and indexed as one blob).
// ai_target_candidate now lives in types/ai.h (folded from this file).

// ai_target_candidate_list now lives in types/ai.h (folded from this file).

// Per-tick perception scan: rebuilds this actor's prop list membership in the two candidate
// buckets (unit props and non-unit props) from its BSP visibility row (or, for a swarm actor,
// the union of every swarm component unit's visibility row), stamps every currently-visible
// object's object.cluster_stamp for this tick, drops props that no longer qualify (deleting
// their datum, and their paired datum's, exactly as actor_target_data_release.c's idiom does),
// then finalizes each candidate list: nearest-first, instantiate/refresh up to 4 real props,
// evicting any existing prop reference beyond that cap.
void actor_target_scan_potential_targets(datum_index actor_index) // blam-cc: stack -> actor_index
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    prop *props = (prop *)prop_data->data;
    uint32_t *pvs_bitmap = 0;
    uint32_t swarm_pvs[16];
    ai_target_candidate_list list_a; // props with is_unit set; actor_find_or_allocate_prop flag=1
    ai_target_candidate_list list_b; // props with is_unit clear; actor_find_or_allocate_prop flag=0
    int32_t row_dwords;
    int32_t stamp;
    datum_index next, current;

    list_a.seen_count = 0;
    list_a.entry_count = 0;
    list_b.seen_count = 0;
    list_b.entry_count = 0;

    row_dwords = (global_structure_bsp->clusters.count + 0x1f) >> 5;

    if (!self->swarm) {
        int16_t cluster_ref = *(int16_t *)&self->unknown_138[0x148 - 0x138]; // UNSURE, see file header
        if (cluster_ref != -1) {
            pvs_bitmap = (uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                       row_dwords * cluster_ref * 4);
        }
    } else {
        swarm *sw = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        uint8_t any = 0;
        int16_t i;

        for (i = 0; i < 16; i++) {
            swarm_pvs[i] = 0;
        }
        for (i = 0; i < sw->component_count; i++) {
            object_header *hdr = (object_header *)object_data->data + (sw->unit_index[i] & 0xffff);
            object *unit_obj = hdr->data;
            int16_t cluster = unit_obj->location_cluster_index;
            if (cluster != -1) {
                int32_t j;
                for (j = row_dwords - 1; j >= 0; j--) {
                    swarm_pvs[j] |= *(uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                                   row_dwords * cluster * 4 + j * 4);
                }
                any = 1;
            }
        }
        if (any) {
            pvs_bitmap = swarm_pvs;
        }
    }

    object_cluster_stamp++;
    object_globals_pointer->collecting_in_clusters = 1; // UNSURE: see file header
    stamp = object_cluster_stamp;

    next = self->first_prop;
    while (current = next, current != k_datum_index_none) {
        prop *p = &props[current & 0xffff];
        uint8_t accept; // Ghidra's bVar1
        uint8_t unit_bucket; // Ghidra's bVar5: true routes into list_a's 128-entry array

        next = p->next_in_actor;

        if (p->kind < 4 || 5 < p->kind) {
            float dist_sq = p->distance * p->distance;
            actor *owner = (p->owner_actor_index == k_datum_index_none)
                               ? (actor *)0
                               : &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
            uint16_t threshold_bits; // Ghidra's uVar23

            unit_bucket = 0;

            if (p->is_parented) {
                accept = 1;
            } else if (owner != (actor *)0 && !(owner->active != 0 && owner->keep_unit_alive == 0)) {
                accept = 0;
            } else if (p->unknown_63 != 0 || 0 < p->unknown_6a) {
                accept = 1;
            } else if (1600.0f < dist_sq) {
                accept = 0;
            } else if (!p->is_vault) {
                if (!p->is_unit) {
                    accept = dist_sq < 225.0f;
                    if (3 < self->unknown_6e) {
                        unit_bucket = 1;
                        goto merged;
                    }
                    if (self->unknown_1cc == 0) {
                        threshold_bits = (uint16_t)(dist_sq < 16.0f) << 8 |
                                          (uint16_t)(dist_sq == 16.0f) << 0xe;
                        goto shared_threshold;
                    }
                    unit_bucket = 0; // fell through: neither shortcut applied
                } else {
                    threshold_bits = (uint16_t)(dist_sq < 36.0f) << 8 |
                                      (uint16_t)(dist_sq == 36.0f) << 0xe;
                    accept = 1;
shared_threshold:
                    unit_bucket = 1;
                    if (threshold_bits == 0) goto merged;
                    unit_bucket = 0;
                }
            } else {
                // prop.is_vault: gate on the actor's encounter and, failing that, on distance /
                // combat grade / awareness, exactly mirroring the original's nested structure.
                datum_index encounter_idx = self->encounter_index;
                accept = 1;
                if (encounter_idx != k_datum_index_none) {
                    encounter *enc = &((encounter *)encounter_data->data)[encounter_idx & 0xffff];
                    int32_t gate = (enc->unknown_58 <= self->unknown_3a0) ? self->unknown_3a0
                                                                           : enc->unknown_58;
                    if (gate != -1) {
                        object_header *ohdr = (object_header *)object_data->data + (p->object_index & 0xffff);
                        unit_data *u = (unit_data *)((uint8_t *)ohdr->data + k_unit_data_offset);
                        int32_t last_seen = u->unknown_41c; // UNSURE: units.h names this "game tick stamp"
                        if (last_seen == -1 || last_seen < gate) {
                            accept = 0;
                        }
                    }
                    if (!(enc->unknown_45 == 0 && enc->unknown_44 == 0 && enc->unknown_42 == 0)) {
                        goto encounter_gate_open;
                    }
                    if (!accept) goto merged;
                    if (dist_sq < 225.0f) {
                        accept = 1;
                        goto merged;
                    }
                    goto not_accepted;
                }
encounter_gate_open:
                if (p->unknown_20 <= 0.0f) {
                    if (!p->is_unit || p->unknown_76 < 0x97) {
                        int16_t grade = actor_get_current_mode_combat_grade(actor_index);
                        if (grade < 2) {
                            float grade_threshold = 16.0f;
                            if (!p->is_unit && self->awareness_level < 3) {
                                grade_threshold = 64.0f;
                            }
                            if (dist_sq < grade_threshold) {
                                accept = 1;
                                goto merged;
                            }
                        }
not_accepted:
                        accept = 0;
                    } else {
                        accept = 0;
                    }
                } else {
                    accept = 1;
                }
            }
merged:
            if (accept && pvs_bitmap != 0) {
                accept = 0;
                {
                    object_placement_cursor cursor;
                    int16_t ref = object_get_root_parent_placement(p->object_index, &cursor);
                    while (ref != -1) {
                        if (pvs_bitmap[(int16_t)ref >> 5] & (1u << ((uint8_t)ref & 0x1f))) {
                            accept = 1;
                            break;
                        }
                        if (cursor.next_reference == k_datum_index_none) {
                            ref = -1;
                        } else {
                            data_array *references = (data_array *)cursor.cluster_globals[2];
                            object_cluster_reference *node =
                                (object_cluster_reference *)references->data + (cursor.next_reference & 0xffff);
                            cursor.next_reference = node->next_reference;
                            ref = (int16_t)node->object_index;
                        }
                    }
                }
            }

            // Stamp every object this candidate touches as visited this tick: the owner's
            // whole unit cluster (or, for a swarmed owner, every swarm component unit), then
            // the tracked object itself.
            if (p->has_parent && p->owner_actor_index != k_datum_index_none) {
                actor *owner2 = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                datum_index cluster_head = owner2->swarm_index;
                if (cluster_head == k_datum_index_none) {
                    datum_index u = self->cluster_unit_index;
                    while (u != k_datum_index_none) {
                        object_header *ohdr = (object_header *)object_data->data + (u & 0xffff);
                        object *uobj = ohdr->data;
                        if (uobj->cluster_stamp != stamp) {
                            uobj->cluster_stamp = stamp;
                        }
                        u = ((unit_data *)((uint8_t *)uobj + k_unit_data_offset))->swarm_next_unit_index;
                    }
                } else {
                    swarm *sw2 = &((swarm *)swarm_data->data)[cluster_head & 0xffff];
                    int16_t i;
                    for (i = 0; i < sw2->component_count; i++) {
                        object_header *ohdr = (object_header *)object_data->data + (sw2->unit_index[i] & 0xffff);
                        object *uobj = ohdr->data;
                        if (uobj->cluster_stamp != stamp) {
                            uobj->cluster_stamp = stamp;
                        }
                    }
                }
            }
            {
                object_header *ohdr = (object_header *)object_data->data + (p->object_index & 0xffff);
                object *tobj = ohdr->data;
                if (tobj->cluster_stamp != stamp) {
                    tobj->cluster_stamp = stamp;
                }
            }

            if (accept) {
                ai_target_candidate_list *list = p->is_unit ? &list_a : &list_b;
                if (unit_bucket) {
                    if (list->entry_count < 0x80) {
                        list->entries[list->entry_count].object_index = p->object_index;
                        list->entries[list->entry_count].prop_index = current;
                        list->entries[list->entry_count].distance = dist_sq * 0.6944444f;
                        list->entry_count++;
                    }
                } else if (!p->is_vault) {
                    list->seen_count++;
                }
            } else {
                if ((p->kind < 4 || 5 < p->kind) && p->pair_index != k_datum_index_none) {
                    actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(p->pair_index)); // ESI -1, EDI the prop
                    actor_unlink_prop(actor_index, p->pair_index); // EAX actor, EDI the prop
                    datum_delete(prop_data, p->pair_index);
                }
                actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(current)); // ESI -1, EDI the prop
                actor_unlink_prop(actor_index, current); // EAX actor, EDI the prop
                datum_delete(prop_data, current);
            }
        }
    }

    // Push visibility into every object referenced from each cluster this actor's PVS row
    // marks visible: actor_target_evaluate_squad_link (0x41e320) folds each one into whichever
    // of the two candidate lists it belongs in.
    if (pvs_bitmap != 0 && global_structure_bsp->clusters.count > 0) {
        int32_t cluster;
        for (cluster = 0; cluster < global_structure_bsp->clusters.count; cluster++) {
            if (pvs_bitmap[cluster >> 5] & (1u << (cluster & 0x1f))) {
                // UNSURE, see file header: collideable/noncollideable_cluster_first chains,
                // exactly as object_get_root_parent_placement.c already names both globals.
                datum_index head;
                int32_t owner_cluster_ref, chain_object;

                head = collideable_cluster_first[cluster];
                if (head == k_datum_index_none) {
                    owner_cluster_ref = -1; chain_object = -1;
                } else {
                    object_cluster_reference *node =
                        (object_cluster_reference *)collideable_object_references->data + (head & 0xffff);
                    owner_cluster_ref = node->next_reference;
                    chain_object = node->object_index;
                }
                while (chain_object != -1) {
                    actor_target_evaluate_squad_link(actor_index, chain_object,
                                                     (int16_t *)&list_a, (int16_t *)&list_b);
                    if (owner_cluster_ref == -1) {
                        chain_object = -1;
                    } else {
                        object_cluster_reference *node =
                            (object_cluster_reference *)collideable_object_references->data +
                            (owner_cluster_ref & 0xffff);
                        owner_cluster_ref = node->next_reference;
                        chain_object = node->object_index;
                    }
                }

                head = noncollideable_cluster_first[cluster];
                if (head == k_datum_index_none) {
                    owner_cluster_ref = -1; chain_object = -1;
                } else {
                    object_cluster_reference *node =
                        (object_cluster_reference *)noncollideable_object_references->data + (head & 0xffff);
                    owner_cluster_ref = node->next_reference;
                    chain_object = node->object_index;
                }
                while (chain_object != -1) {
                    actor_target_evaluate_squad_link(actor_index, chain_object,
                                                     (int16_t *)&list_a, (int16_t *)&list_b);
                    if (owner_cluster_ref == -1) {
                        chain_object = -1;
                    } else {
                        object_cluster_reference *node =
                            (object_cluster_reference *)noncollideable_object_references->data +
                            (owner_cluster_ref & 0xffff);
                        owner_cluster_ref = node->next_reference;
                        chain_object = node->object_index;
                    }
                }
            }
        }
    }

    // Finalize list_a: nearest-first, instantiate/refresh up to 4, evict the rest.
    // (list_b below repeats the same shape with a looser cap and actor_find_or_allocate_prop's flag=0.)
    if (list_a.entry_count > 0) {
        int16_t i = 0;
        int16_t bound = list_a.entry_count;

        if (list_a.seen_count < 4) {
            _qsort(list_a.entries, list_a.entry_count, sizeof(ai_target_candidate),
                   (int32_t (*)(const void *, const void *))ai_target_distance_qsort_compare);
            if (list_a.entry_count > 0) {
                do {
                    if (list_a.entries[i].prop_index == k_datum_index_none) {
                        datum_index new_prop = actor_find_or_allocate_prop(actor_index, list_a.entries[i].object_index, 1);
                        if (new_prop != k_datum_index_none) {
                            actor_target_data_refresh(actor_index, new_prop, swarm_pvs, 0, 0);
                            goto list_a_counted;
                        }
                    } else {
list_a_counted:
                        list_a.seen_count++;
                        bound = list_a.entry_count;
                        if (3 < list_a.seen_count) goto list_a_evict;
                    }
                    i++;
                } while (i < list_a.entry_count);
            }
        } else {
list_a_evict:
            if (i < bound) {
                do {
                    if (list_a.entries[i].prop_index != k_datum_index_none) {
                        prop *existing = &props[list_a.entries[i].prop_index & 0xffff];
                        if ((existing->kind < 4 || 5 < existing->kind) &&
                            existing->pair_index != k_datum_index_none) {
                            actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(existing->pair_index)); // ESI -1, EDI the prop
                            actor_unlink_prop(actor_index, existing->pair_index); // EAX actor, EDI the prop
                            datum_delete(prop_data, existing->pair_index);
                        }
                        actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(list_a.entries[i].prop_index)); // ESI -1, EDI the prop
                        actor_unlink_prop(actor_index, list_a.entries[i].prop_index); // EAX actor, EDI the prop
                        datum_delete(prop_data, list_a.entries[i].prop_index);
                    }
                    i++;
                } while (i < list_a.entry_count);
            }
        }
    }

    // Finalize list_b: same shape as list_a, but the acceptance budget is shared with list_a
    // (Ghidra's `local_604 + local_c08`, whose low 16 bits are list_a.seen_count +
    // list_b.seen_count) against a cap derived from list_a's own seen_count (`local_c08 + 2`,
    // floored at 4), not list_b's.
    if (list_b.entry_count > 0) {
        int16_t i = 0;
        int16_t combined = (int16_t)(list_a.seen_count + list_b.seen_count);
        int16_t cap = list_a.seen_count + 2;

        if (cap < 5) cap = 4;

        if (combined < cap) {
            _qsort(list_b.entries, list_b.entry_count, sizeof(ai_target_candidate),
                   (int32_t (*)(const void *, const void *))ai_target_distance_qsort_compare);
            if (list_b.entry_count > 0) {
                do {
                    if (list_b.entries[i].prop_index == k_datum_index_none) {
                        datum_index new_prop = actor_find_or_allocate_prop(actor_index, list_b.entries[i].object_index, 0);
                        if (new_prop != k_datum_index_none) {
                            actor_target_data_refresh(actor_index, new_prop, swarm_pvs, 0, 0);
                            goto list_b_counted;
                        }
                    } else {
list_b_counted:
                        list_b.seen_count++;
                        combined++;
                        if (cap <= combined) goto list_b_evict;
                    }
                    i++;
                } while (i < list_b.entry_count);
                object_globals_pointer->collecting_in_clusters = 0;
                return;
            }
        } else {
list_b_evict:
            if (list_b.entry_count <= i) {
                object_globals_pointer->collecting_in_clusters = 0;
                return;
            }
            do {
                if (list_b.entries[i].prop_index != k_datum_index_none) {
                    prop *existing = &props[list_b.entries[i].prop_index & 0xffff];
                    if ((existing->kind < 4 || 5 < existing->kind) &&
                        existing->pair_index != k_datum_index_none) {
                        actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(existing->pair_index)); // ESI -1, EDI the prop
                        actor_unlink_prop(actor_index, existing->pair_index); // EAX actor, EDI the prop
                        datum_delete(prop_data, existing->pair_index);
                    }
                    actor_replace_object_reference(actor_index, 0xffffffff, (uint32_t)(list_b.entries[i].prop_index)); // ESI -1, EDI the prop
                    actor_unlink_prop(actor_index, list_b.entries[i].prop_index); // EAX actor, EDI the prop
                    datum_delete(prop_data, list_b.entries[i].prop_index);
                }
                i++;
            } while (i < list_b.entry_count);
        }
    }

    object_globals_pointer->collecting_in_clusters = 0;
}

#if 0
Original Ghidra decompilation (0x41d7e0):

void FUN_0041d7e0(uint param_1)

{
  bool bVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  bool bVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  uint uVar10;
  undefined4 uVar11;
  uint uVar12;
  int iVar13;
  int iVar14;
  short *psVar15;
  int iVar16;
  int iVar17;
  uint *puVar18;
  int iVar19;
  uint *puVar20;
  int iVar21;
  int iVar22;
  ushort uVar23;
  uint auStackY_60c04 [385];
  uint auStackY_60600 [97882];
  int local_c70;
  uint local_c6c;
  uint *local_c68;
  uint local_c58;
  int local_c54;
  uint local_c50;
  uint local_c48 [16];
  undefined4 local_c08;
  uint local_c04 [384];
  undefined4 local_604;
  uint local_600 [384];

  iVar21 = DAT_00880360;
  iVar16 = DAT_00746f9c;
  iVar22 = (param_1 & 0xffff) * 0x724;
  iVar9 = *(int *)(DAT_00880360 + 0x34) + iVar22;
  local_c08 = 0;
  local_604 = 0;
  local_c68 = (uint *)0x0;
  if (*(char *)(iVar9 + 6) == '\0') {
    if (*(short *)(iVar9 + 0x148) != -1) {
      local_c68 = (uint *)(*(int *)(DAT_00746f9c + 0x14c) +
                          (*(int *)(DAT_00746f9c + 0x134) + 0x1f >> 5) *
                          (int)*(short *)(iVar9 + 0x148) * 4);
    }
  }
  else {
    iVar19 = (*(uint *)(iVar9 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    puVar20 = local_c48;
    for (iVar17 = 0x10; iVar17 != 0; iVar17 = iVar17 + -1) {
      *puVar20 = 0;
      puVar20 = puVar20 + 1;
    }
    uVar23 = *(ushort *)(iVar19 + 2);
    bVar5 = false;
    if (0 < (short)uVar23) {
      iVar17 = *(int *)(DAT_008603b0 + 0x34);
      local_c6c = (uint)uVar23;
      puVar20 = (uint *)(iVar19 + 0x18);
      do {
        sVar7 = *(short *)(*(int *)(iVar17 + 8 + (*puVar20 & 0xffff) * 0xc) + 0x9c);
        if (sVar7 != -1) {
          iVar19 = *(int *)(DAT_00746f9c + 0x134);
          iVar13 = *(int *)(DAT_00746f9c + 0x14c);
          uVar10 = *(short *)(DAT_00746f9c + 0x134) + 0x1f >> 5;
          sVar6 = (short)uVar10 + -1;
          if (-1 < sVar6) {
            puVar18 = local_c48 + sVar6;
            uVar10 = uVar10 & 0xffff;
            do {
              *puVar18 = *puVar18 |
                         *(uint *)(((iVar13 + (iVar19 + 0x1f >> 5) * (int)sVar7 * 4) -
                                   (int)local_c48) + (int)puVar18);
              puVar18 = puVar18 + -1;
              uVar10 = uVar10 - 1;
            } while (uVar10 != 0);
          }
          bVar5 = true;
        }
        puVar20 = puVar20 + 1;
        local_c6c = local_c6c - 1;
      } while (local_c6c != 0);
      if (bVar5) {
        local_c68 = local_c48;
      }
    }
  }
  DAT_008603cc = DAT_008603cc + 1;
  *(undefined1 *)(DAT_006b8cbc + 1) = 1;
  uVar10 = *(uint *)(iVar22 + 0x50 + *(int *)(iVar21 + 0x34));
  while (local_c58 = uVar10, local_c58 != 0xffffffff) {
    iVar21 = (local_c58 & 0xffff) * 0x138;
    sVar7 = *(short *)(iVar21 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
    iVar21 = iVar21 + *(int *)(DAT_008802c0 + 0x34);
    uVar10 = *(uint *)(iVar21 + 8);
    if ((sVar7 < 4) || (5 < sVar7)) {
      fVar2 = *(float *)(iVar21 + 0x11c) * *(float *)(iVar21 + 0x11c);
      iVar17 = *(int *)(DAT_00880360 + 0x34);
      if (*(uint *)(iVar21 + 0x1c) == 0xffffffff) {
        iVar19 = 0;
      }
      else {
        iVar19 = (*(uint *)(iVar21 + 0x1c) & 0xffff) * 0x724 + iVar17;
      }
      bVar5 = false;
      if (*(char *)(iVar21 + 0x12e) == '\0') {
        if ((iVar19 == 0) || ((*(char *)(iVar19 + 8) != '\0' && (*(char *)(iVar19 + 0x13) == '\0')))
           ) {
          if ((*(char *)(iVar21 + 99) == '\0') && (*(short *)(iVar21 + 0x6a) < 1)) {
            if (fVar2 <= 1600.0) {
              if (*(char *)(iVar21 + 0x127) == '\0') {
                if (*(char *)(iVar21 + 0x60) == '\0') {
                  bVar1 = fVar2 < 225.0;
                  if (3 < *(short *)(iVar22 + 0x6e + iVar17)) {
                    bVar5 = true;
                    goto LAB_0041dbdd;
                  }
                  if (*(char *)(iVar22 + 0x1cc + iVar17) == '\0') {
                    uVar23 = (ushort)(fVar2 < 16.0) << 8 | (ushort)(fVar2 == 16.0) << 0xe;
                    goto LAB_0041dbcc;
                  }
                }
                else {
                  uVar23 = (ushort)(fVar2 < 36.0) << 8 | (ushort)(fVar2 == 36.0) << 0xe;
                  bVar1 = true;
LAB_0041dbcc:
                  bVar5 = true;
                  if (uVar23 == 0) goto LAB_0041dbdd;
                }
                bVar5 = false;
              }
              else {
                uVar12 = *(uint *)(iVar22 + 0x34 + iVar17);
                bVar1 = true;
                if (uVar12 != 0xffffffff) {
                  iVar14 = (uVar12 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
                  iVar19 = *(int *)(iVar22 + 0x3a0 + iVar17);
                  iVar13 = *(int *)(iVar14 + 0x58);
                  if (*(int *)(iVar14 + 0x58) <= iVar19) {
                    iVar13 = iVar19;
                  }
                  if ((iVar13 != -1) &&
                     ((iVar19 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                 (*(uint *)(iVar21 + 0x18) & 0xffff) * 0xc) + 0x41c)
                      , iVar19 == -1 || (iVar19 < iVar13)))) {
                    bVar1 = false;
                  }
                  if (((*(char *)(iVar14 + 0x45) == '\0') && (*(char *)(iVar14 + 0x44) == '\0')) &&
                     (*(char *)(iVar14 + 0x42) == '\0')) {
                    bVar4 = true;
                  }
                  else {
                    bVar4 = false;
                  }
                  if (!bVar1) goto LAB_0041dbdd;
                  if (!bVar4) goto LAB_0041db01;
                  if (fVar2 < 225.0) {
                    bVar1 = true;
                    goto LAB_0041dbdd;
                  }
                  goto LAB_0041db41;
                }
LAB_0041db01:
                if (*(float *)(iVar21 + 0x20) <= 0.0) {
                  if ((*(char *)(iVar21 + 0x60) == '\0') || (*(short *)(iVar21 + 0x76) < 0x97)) {
                    sVar7 = FUN_0040e760();
                    if (sVar7 < 2) {
                      fVar3 = 16.0;
                      if ((*(char *)(iVar21 + 0x60) == '\0') &&
                         (*(short *)(iVar22 + 0x6a + iVar17) < 3)) {
                        fVar3 = 64.0;
                      }
                      if (fVar2 < fVar3) {
                        bVar1 = true;
                        goto LAB_0041dbdd;
                      }
                    }
LAB_0041db41:
                    bVar1 = false;
                  }
                  else {
                    bVar1 = false;
                  }
                }
                else {
                  bVar1 = true;
                }
              }
            }
            else {
              bVar1 = false;
            }
          }
          else {
            bVar1 = true;
          }
        }
        else {
          bVar1 = false;
        }
      }
      else {
        bVar1 = true;
      }
LAB_0041dbdd:
      if ((bVar1) && (local_c68 != (uint *)0x0)) {
        bVar1 = false;
        uVar11 = object_get_root_parent_placement();
        sVar7 = (short)uVar11;
        while (sVar7 != -1) {
          if ((local_c68[(int)(short)uVar11 >> 5] & 1 << ((byte)uVar11 & 0x1f)) != 0) {
            bVar1 = true;
            break;
          }
          if (local_c50 == 0xffffffff) {
            uVar11 = 0xffffffff;
          }
          else {
            uVar12 = local_c50 & 0xffff;
            iVar17 = *(int *)(*(int *)(local_c54 + 8) + 0x34);
            local_c50 = *(uint *)(iVar17 + 8 + uVar12 * 0xc);
            uVar11 = *(undefined4 *)(iVar17 + uVar12 * 0xc + 4);
          }
          sVar7 = (short)uVar11;
        }
      }
      iVar19 = DAT_008603cc;
      iVar17 = DAT_008603b0;
      if ((*(char *)(iVar21 + 0x14) != '\0') && (*(uint *)(iVar21 + 0x1c) != 0xffffffff)) {
        uVar12 = *(uint *)((*(uint *)(iVar21 + 0x1c) & 0xffff) * 0x724 +
                           *(int *)(DAT_00880360 + 0x34) + 0x28);
        if (uVar12 == 0xffffffff) {
          uVar12 = *(uint *)(iVar9 + 0x24);
          while (uVar12 != 0xffffffff) {
            iVar14 = (uVar12 & 0xffff) * 0xc;
            iVar13 = *(int *)(*(int *)(iVar17 + 0x34) + 8 + iVar14);
            if (*(int *)(iVar13 + 0x14) != iVar19) {
              *(int *)(*(int *)(*(int *)(iVar17 + 0x34) + 8 + iVar14) + 0x14) = iVar19;
            }
            uVar12 = *(uint *)(iVar13 + 0x1fc);
          }
        }
        else {
          iVar13 = (uVar12 & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
          sVar7 = 0;
          if (0 < *(short *)(iVar13 + 2)) {
            do {
              iVar14 = *(int *)(*(int *)(iVar17 + 0x34) + 8 +
                               (*(uint *)(iVar13 + 0x18 + sVar7 * 4) & 0xffff) * 0xc);
              if (*(int *)(iVar14 + 0x14) != iVar19) {
                *(int *)(iVar14 + 0x14) = iVar19;
              }
              sVar7 = sVar7 + 1;
            } while (sVar7 < *(short *)(iVar13 + 2));
          }
        }
      }
      iVar17 = *(int *)(*(int *)(iVar17 + 0x34) + 8 + (*(uint *)(iVar21 + 0x18) & 0xffff) * 0xc);
      if (*(int *)(iVar17 + 0x14) != iVar19) {
        *(int *)(iVar17 + 0x14) = iVar19;
      }
      if (bVar1) {
        psVar15 = (short *)&local_c08;
        if (*(char *)(iVar21 + 0x60) == '\0') {
          psVar15 = (short *)&local_604;
        }
        if (bVar5) {
          if (psVar15[1] < 0x80) {
            *(undefined4 *)(psVar15 + psVar15[1] * 6 + 2) = *(undefined4 *)(iVar21 + 0x18);
            *(uint *)(psVar15 + psVar15[1] * 6 + 4) = local_c58;
            *(float *)(psVar15 + (psVar15[1] + 1) * 6) = fVar2 * 0.6944444;
            psVar15[1] = psVar15[1] + 1;
          }
        }
        else if (*(char *)(iVar21 + 0x127) == '\0') {
          *psVar15 = *psVar15 + 1;
        }
      }
      else {
        if (((*(short *)(iVar21 + 0x24) < 4) || (5 < *(short *)(iVar21 + 0x24))) &&
           (*(int *)(iVar21 + 0xc) != -1)) {
          actor_replace_object_reference(param_1);
          FUN_0043ea20();
          datum_delete();
        }
        actor_replace_object_reference(param_1);
        FUN_0043ea20();
        datum_delete();
      }
    }
  }
  if ((local_c68 != (uint *)0x0) && (sVar7 = 0, 0 < *(int *)(iVar16 + 0x134))) {
    iVar21 = 0;
    do {
      if ((local_c68[iVar21 >> 5] & 1 << ((byte)iVar21 & 0x1f)) != 0) {
        uVar10 = *(uint *)(DAT_008603d0 + iVar21 * 4);
        if (uVar10 == 0xffffffff) {
          iVar9 = -1;
          uVar12 = 0xffffffff;
        }
        else {
          uVar10 = uVar10 & 0xffff;
          uVar12 = *(uint *)(*(int *)(DAT_008603d4 + 0x34) + 8 + uVar10 * 0xc);
          iVar9 = *(int *)(*(int *)(DAT_008603d4 + 0x34) + uVar10 * 0xc + 4);
        }
        while (iVar9 != -1) {
          FUN_0041e320(param_1,iVar9,&local_c08,&local_604);
          if (uVar12 == 0xffffffff) {
            iVar9 = -1;
            uVar12 = 0xffffffff;
          }
          else {
            uVar10 = uVar12 & 0xffff;
            uVar12 = *(uint *)(*(int *)(DAT_008603d4 + 0x34) + 8 + uVar10 * 0xc);
            iVar9 = *(int *)(*(int *)(DAT_008603d4 + 0x34) + uVar10 * 0xc + 4);
          }
        }
        uVar10 = *(uint *)(DAT_008603c0 + iVar21 * 4);
        if (uVar10 == 0xffffffff) {
          iVar21 = -1;
          uVar12 = 0xffffffff;
        }
        else {
          uVar10 = uVar10 & 0xffff;
          uVar12 = *(uint *)(*(int *)(DAT_008603c4 + 0x34) + 8 + uVar10 * 0xc);
          iVar21 = *(int *)(*(int *)(DAT_008603c4 + 0x34) + uVar10 * 0xc + 4);
        }
        while (iVar21 != -1) {
          FUN_0041e320(param_1,iVar21,&local_c08,&local_604);
          if (uVar12 == 0xffffffff) {
            iVar21 = -1;
            uVar12 = 0xffffffff;
          }
          else {
            uVar10 = uVar12 & 0xffff;
            uVar12 = *(uint *)(*(int *)(DAT_008603c4 + 0x34) + 8 + uVar10 * 0xc);
            iVar21 = *(int *)(*(int *)(DAT_008603c4 + 0x34) + uVar10 * 0xc + 4);
          }
        }
      }
      sVar7 = sVar7 + 1;
      iVar21 = (int)sVar7;
    } while (iVar21 < *(int *)(iVar16 + 0x134));
  }
  if (0 < local_c08._2_2_) {
    sVar7 = 0;
    sVar6 = local_c08._2_2_;
    if ((short)local_c08 < 4) {
      _qsort(local_c04,(int)local_c08._2_2_,0xc,ai_target_distance_qsort_compare);
      if (0 < local_c08._2_2_) {
        do {
          if (local_c04[sVar7 * 3 + 1] == 0xffffffff) {
            iVar16 = FUN_0043e270(param_1,local_c04[sVar7 * 3],1);
            if (iVar16 != -1) {
              FUN_0041c4b0(param_1,iVar16,local_c48,0,0);
              goto LAB_0041e03e;
            }
          }
          else {
LAB_0041e03e:
            sVar8 = (short)local_c08 + 1;
            local_c08 = CONCAT22(local_c08._2_2_,sVar8);
            sVar6 = local_c08._2_2_;
            if (3 < sVar8) goto LAB_0041e066;
          }
          sVar7 = sVar7 + 1;
        } while (sVar7 < local_c08._2_2_);
      }
    }
    else {
LAB_0041e066:
      if (sVar7 < sVar6) {
        do {
          if (local_c04[sVar7 * 3 + 1] != 0xffffffff) {
            iVar16 = (local_c04[sVar7 * 3 + 1] & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
            sVar6 = *(short *)(iVar16 + 0x24);
            if (((sVar6 < 4) || (5 < sVar6)) && (*(int *)(iVar16 + 0xc) != -1)) {
              actor_replace_object_reference(param_1);
              FUN_0043ea20();
              datum_delete();
            }
            actor_replace_object_reference(param_1);
            FUN_0043ea20();
            datum_delete();
          }
          sVar7 = sVar7 + 1;
        } while (sVar7 < local_c08._2_2_);
      }
    }
  }
  if (local_604._2_2_ < 1) {
LAB_0041e201:
    *(undefined1 *)(DAT_006b8cbc + 1) = 0;
    return;
  }
  iVar16 = local_604 + local_c08;
  local_c70 = (short)local_c08 + 2;
  sVar7 = 0;
  if (local_c70 < 5) {
    local_c70 = 4;
  }
  sVar6 = local_604._2_2_;
  if ((short)iVar16 < (short)local_c70) {
    _qsort(local_600,(int)local_604._2_2_,0xc,ai_target_distance_qsort_compare);
    if (0 < local_604._2_2_) {
      do {
        if (local_600[sVar7 * 3 + 1] == 0xffffffff) {
          iVar21 = FUN_0043e270(param_1,local_600[sVar7 * 3],0);
          if (iVar21 != -1) {
            FUN_0041c4b0(param_1,iVar21,local_c48,0,0);
            goto LAB_0041e1e2;
          }
        }
        else {
LAB_0041e1e2:
          local_604 = CONCAT22(local_604._2_2_,(short)local_604 + 1);
          iVar16 = iVar16 + 1;
          sVar6 = local_604._2_2_;
          if ((short)local_c70 <= (short)iVar16) goto LAB_0041e21e;
        }
        sVar7 = sVar7 + 1;
      } while (sVar7 < local_604._2_2_);
      goto LAB_0041e201;
    }
  }
  else {
LAB_0041e21e:
    if (sVar6 <= sVar7) {
      *(undefined1 *)(DAT_006b8cbc + 1) = 0;
      return;
    }
    do {
      if (local_600[sVar7 * 3 + 1] != 0xffffffff) {
        iVar16 = (local_600[sVar7 * 3 + 1] & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
        sVar6 = *(short *)(iVar16 + 0x24);
        if (((sVar6 < 4) || (5 < sVar6)) && (*(int *)(iVar16 + 0xc) != -1)) {
          actor_replace_object_reference(param_1);
          FUN_0043ea20();
          datum_delete();
        }
        actor_replace_object_reference(param_1);
        FUN_0043ea20();
        datum_delete();
      }
      sVar7 = sVar7 + 1;
    } while (sVar7 < local_604._2_2_);
  }
  *(undefined1 *)(DAT_006b8cbc + 1) = 0;
  return;
}
#endif
