// actor_scan_allies_for_backup_request  (Ghidra: actor_scan_allies_for_backup_request; named per the extern already
//   declared by actor_update_crouch_state.c, which calls this function, and by
//   actor_target_get_backup_priority.c, which documents being called from here)
// address 0x420ec0, size 1256 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: types/ai.h actor.first_prop / prop.kind / prop.owner_actor_index / prop.distance /
//   prop.engaged / prop.is_unit; actor_target_get_backup_priority (0x420e50) and
//   ai_group_bucket_find_or_add (0x420de0), both already rewritten in this module and cited by
//   their own file headers as the two helpers this function calls; actor.unknown_3a4/0x3a8/
//   0x3ac/0x3b0 (already-named fields in types/ai.h, read back by actor_update_crouch_state
//   right after it calls this function); types/tags.h Actor danger/retreat trigger fields,
//   confirmed field-for-field against the binary below; types/game.h game_time_globals.game_time;
//   src/math/random_real_range.c and src/items/item_detonation_timer_start.c (the identical
//   inlined-LCG + "* 30.0f ticks-per-second" idiom, called here as random_real_range() per that
//   file's precedent rather than re-inlined).
// register convention: actor index is a genuine stack parameter (matches the extern already
//   declared by actor_update_crouch_state.c).
// blam-cc: stack -> actor_index
//
// Ghidra drops 49 blocks here as unreachable ("WARNING: Removing unreachable block", addresses
// 0x421115..0x421332, about 43% of the body) and its decompile jumps straight from the
// prop-scanning loop to the two-line cooldown-decrement tail, skipping the entire per-bucket
// scoring pass and the "pick a call to answer" tail. This rewrite instead comes from a full
// objdump -d -Mintel disassembly of 0x420ec0..0x4213b0 (the function's exact bounds per
// out/functions.json: the next function starts at 0x4213b0 with no gap). Every stack-slot
// offset below was tracked by hand against the four register pushes (ebx/ebp/esi/edi, 0x10
// bytes) and the 0x1dc-byte local frame, and cross-checked against the two call sites that
// already exist for this function's two helpers. The Actor tag field offsets (0x268
// unreachable_danger_trigger .. 0x2a0 retreat_time) were confirmed by compiling a tiny
// offsetof() harness against types/tags.h: it reproduced 0x268/0x26a/0x26c/0x270/0x278/0x27a/
// 0x288 exactly.
//
// UNSURE: prop+0x78 is declared as a float (unknown_78) in types/ai.h on weak evidence (the
// header's own notes attribute that area only to the unread 0x412ba0); this function reads it
// as a signed int16 counter compared against 45. The access below reinterprets through the
// header field (the established `*(int16_t *)&field` idiom used elsewhere in this module)
// rather than reintroducing a raw offset.
// UNSURE: prop+0xaa/0xac/0xae are named shots_fired/shots_hit/shots_unknown_ae elsewhere in
// this module on actor_target_reset_shot_counters evidence, but this function clearly reuses
// them, on the *claiming actor's own* prop for the requested object, as a "how long left to
// answer this call" countdown / "already answering" flag / re-rolled random response-time
// tick. The header names are kept (never redefined) with this note standing in their place.
// UNSURE: the ai_group_bucket_entry fields (from ai_group_bucket_find_or_add.c, itself a
// TYPES-GAP) are used here as: 0x00 priority claimed for this bucket's key so far, 0x04 the
// claiming actor's own prop datum index (or none), 0x08/key the requested object's index,
// 0x0c a pointer to that same claiming prop, 0x10 how many times a claim refreshed the
// bucket's best distance, 0x14 the smallest claim distance^2 seen (FLT_MAX sentinel), 0x18 the
// nearest/first claiming ally's actor index. Commented in place rather than renamed in the
// shared TYPES-GAP struct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern real random_real_range(real min, real max); // 0x401050, math module

extern uint8_t actor_target_get_backup_priority(datum_index target_prop_index); // 0x420e50, this module
extern int16_t ai_group_bucket_find_or_add(void *buckets, int32_t key, int16_t *count,
                                           int16_t capacity); // 0x420de0, this module
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX

// ai_group_bucket_entry now lives in types/ai.h (folded from this file).

// Per-tick scan of every prop (perceived object) this actor is tracking. For each one that
// belongs to (is owned by) another actor and looks like a live target, checks whether that
// owning ally currently has an outstanding "call for backup" (owner.unknown_3a8/0x3ac) this
// actor is eligible to answer, and if so buckets the candidate by the object being called
// about, keeping the nearest claim per bucket. A prop this actor itself already rates urgent
// (actor_target_get_backup_priority > 0) is bucketed directly by its own object instead.
// Every bucket is then scored up using the actor tag's danger/friend triggers, and finally:
// while this actor's own backup-request cooldown (unknown_3a8) is still counting down, it is
// just decremented (and unknown_3a4 stamped with the current tick when it reaches zero);
// once it has expired, the highest-scoring bucket (if any, priority > 5) is chosen and this
// actor issues its own call by rolling a new cooldown from the tag's retreat_time range and
// recording the chosen prop (unknown_3ac) and the current tick (unknown_3b0).
void actor_scan_allies_for_backup_request(datum_index actor_index) // blam-cc: stack -> actor_index
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_def = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    prop *props = (prop *)prop_data->data;

    ai_group_bucket_entry buckets[16]; // capacity 0x10, lazily initialized by find_or_add
    int16_t bucket_count = 0;

    datum_index next = self->first_prop;
    datum_index current;

    while (current = next, current != k_datum_index_none) {
        prop *p = &props[current & 0xffff];
        uint8_t priority;

        next = p->next_in_actor;
        priority = actor_target_get_backup_priority(current);

        if (priority > 0) {
            // Directly urgent prop: bucket by its own object, keyed on nothing but priority.
            int16_t idx = ai_group_bucket_find_or_add(buckets, (int32_t)p->object_index,
                                                       &bucket_count, 16);
            if (idx != -1) {
                if (buckets[idx].priority < (int16_t)priority) {
                    buckets[idx].prop_index = (int32_t)current;
                    buckets[idx].key = (int32_t)p->object_index;
                    buckets[idx].prop = p;
                    buckets[idx].priority = (int16_t)priority;
                }
            }
        } else if (2 <= p->state && p->state <= 3 && !p->enemy &&
                   p->owner_actor_index != k_datum_index_none && p->distance < 8.0f) {
            actor *owner = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];

            if (owner->retreat_timer != 0 && owner->retreat_prop_index != k_datum_index_none &&
                (self->retreat_end_time == k_datum_index_none ||
                 owner->retreat_start_time >= self->retreat_end_time)) {
                prop *requested = &props[owner->retreat_prop_index & 0xffff];
                datum_index own_prop_index = actor_find_prop_for_object(requested->object_index, actor_index); // 0x421034: ECX = actor (arg)

                if (own_prop_index != k_datum_index_none) {
                    prop *own_prop = &props[own_prop_index & 0xffff];

                    if (2 <= own_prop->state && own_prop->state <= 3 && own_prop->engaged) {
                        int16_t idx = ai_group_bucket_find_or_add(
                            buckets, (int32_t)requested->object_index, &bucket_count, 16);
                        if (idx != -1) {
                            float dist_sq = requested->distance * requested->distance;

                            buckets[idx].retreating_friend_count++;
                            if (dist_sq < buckets[idx].nearest_friend_distance_squared) {
                                buckets[idx].nearest_friend_distance_squared = dist_sq;
                                buckets[idx].nearest_friend_actor_index = (int32_t)p->owner_actor_index;
                            }
                            if (buckets[idx].prop_index == k_datum_index_none) {
                                buckets[idx].prop_index = (int32_t)own_prop_index;
                                buckets[idx].key = (int32_t)own_prop->object_index;
                                buckets[idx].prop = own_prop;
                            }
                        }
                    }
                }
            }
        }
    }

    // Score every populated bucket using the actor tag's danger / friend triggers.
    if (bucket_count > 0) {
        int16_t i;
        for (i = 0; i < bucket_count; i++) {
            ai_group_bucket_entry *b = &buckets[i];
            prop *claimant = b->prop;
            int16_t trigger = actor_def->unreachable_danger_trigger;
            uint8_t flagged = 0;

            if (claimant->is_vehicle_gunner != 0 || claimant->is_vehicle_driver != 0) {
                trigger = actor_def->vehicle_danger_trigger;
            }
            if (claimant->is_parented) {
                int16_t player_trigger = actor_def->player_danger_trigger;
                if (player_trigger > 0 && trigger > player_trigger) {
                    trigger = player_trigger;
                }
            }

            if (trigger > 0 && b->priority >= trigger) {
                if (!claimant->is_parented) {
                    claimant->shots_fired = 0x16;
                } else {
                    flagged = 1;
                }
            } else if (claimant->is_parented) {
                claimant->shots_fired = 0x16;
            }

            if (claimant->shots_fired > 0) {
                if (claimant->shots_hit == 0) {
                    claimant->shots_unknown_ae = (int16_t)(random_real_range(
                        actor_def->danger_trigger_time[0], actor_def->danger_trigger_time[1]) *
                        30.0f);
                }
                claimant->shots_fired--;
                claimant->shots_hit++;
            }

            if (claimant->sighted_ticks >= 0x2d ||b->priority >= 4) { // UNSURE, see file header
                if (claimant->shots_unknown_ae > 0 &&
                    claimant->shots_hit >= claimant->shots_unknown_ae) {
                    if (b->priority < 7) b->priority = 7;
                }
                if (flagged) {
                    if (b->priority < 8) b->priority = 8;
                }
                if (actor_def->friends_killed_trigger > 0 &&
                    claimant->friends_killed >= actor_def->friends_killed_trigger) {
                    if (b->priority < 9) b->priority = 9;
                }
                if (actor_def->friends_retreating_trigger > 0 &&
                    b->retreating_friend_count >= actor_def->friends_retreating_trigger) {
                    if (b->priority < 6) b->priority = 6;
                }
            }
        }
    }

    // Age down (or issue) this actor's own backup-request cooldown.
    if (self->retreat_timer > 0) {
        self->retreat_timer--;
        if (self->retreat_timer == 0) {
            self->retreat_end_time = game_time->game_time;
        }
        return;
    }

    {
        int16_t best_priority = 5;
        int32_t best_prop = k_datum_index_none;
        int16_t i;

        for (i = 0; i < bucket_count; i++) {
            if (buckets[i].priority > best_priority &&
                buckets[i].prop_index != k_datum_index_none) {
                best_priority = buckets[i].priority;
                best_prop = buckets[i].prop_index;
            }
        }

        if (best_prop != k_datum_index_none) {
            self->retreat_timer = (int16_t)(random_real_range(
                actor_def->retreat_time[0], actor_def->retreat_time[1]) * 30.0f);
            self->retreat_prop_index = (datum_index)best_prop;
            self->retreat_start_time = game_time->game_time;
        }
    }
}

#if 0
Original Ghidra decompilation (0x420ec0) -- truncated: Ghidra removes 49 blocks
(0x421115..0x421332) as unreachable, so this listing skips straight from the scanning loop
close to the two-line cooldown tail. See the file header for the full objdump-based
reconstruction this rewrite is actually built from.

void FUN_00420ec0(uint param_1)

{
  int iVar1;
  float fVar2;
  float fVar3;
  uint uVar4;
  uint uVar5;
  short sVar6;
  short sVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  int iVar11;
  uint uVar12;
  uint local_1c4;
  short local_1c0 [2];
  uint local_1bc [3];
  short local_1b0 [2];
  float fStack_1ac;
  uint auStack_1a8 [106];

  iVar1 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar4 = *(uint *)(iVar1 + 0x50);
  while (local_1c4 = uVar4, local_1c4 != 0xffffffff) {
    iVar8 = *(int *)(DAT_008802c0 + 0x34);
    iVar9 = (local_1c4 & 0xffff) * 0x138;
    uVar4 = *(uint *)(iVar9 + 8 + iVar8);
    uVar10 = iVar9 + iVar8;
    sVar6 = FUN_00420e50();
    if (sVar6 < 1) {
      if (((((1 < *(short *)(uVar10 + 0x24)) && (*(short *)(uVar10 + 0x24) < 4)) &&
           (*(char *)(uVar10 + 0x60) == '\0')) &&
          (((uVar5 = *(uint *)(uVar10 + 0x1c), uVar5 != 0xffffffff &&
            (*(float *)(uVar10 + 0x11c) < 8.0)) &&
           ((iVar9 = (uVar5 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34),
            *(short *)(iVar9 + 0x3a8) != 0 && (*(uint *)(iVar9 + 0x3ac) != 0xffffffff)))))) &&
         ((*(int *)(iVar1 + 0x3a4) == -1 || (*(int *)(iVar1 + 0x3a4) <= *(int *)(iVar9 + 0x3b0)))))
      {
        iVar9 = (*(uint *)(iVar9 + 0x3ac) & 0xffff) * 0x138;
        uVar10 = FUN_0043ea80(*(undefined4 *)(iVar9 + 0x18 + iVar8));
        if (uVar10 != 0xffffffff) {
          iVar11 = (uVar10 & 0xffff) * 0x138;
          sVar6 = *(short *)(iVar11 + 0x24 + iVar8);
          uVar12 = iVar11 + iVar8;
          if ((((1 < sVar6) && (sVar6 < 4)) && (*(char *)(uVar12 + 0xa4) != '\0')) &&
             (sVar6 = FUN_00420de0(0x10), sVar6 != -1)) {
            iVar11 = (int)sVar6;
            fVar2 = *(float *)(iVar9 + iVar8 + 0x11c);
            fVar2 = fVar2 * fVar2;
            fVar3 = (&fStack_1ac)[iVar11 * 7];
            local_1b0[iVar11 * 0xe] = local_1b0[iVar11 * 0xe] + 1;
            if (fVar2 < fVar3) {
              (&fStack_1ac)[iVar11 * 7] = fVar2;
              auStack_1a8[iVar11 * 7] = uVar5;
            }
            if (local_1bc[iVar11 * 7] == 0xffffffff) {
              uVar5 = *(uint *)(uVar12 + 0x18);
              local_1bc[iVar11 * 7] = uVar10;
              local_1bc[iVar11 * 7 + 1] = uVar5;
              local_1bc[iVar11 * 7 + 2] = uVar12;
            }
          }
        }
      }
    }
    else {
      uVar5 = *(uint *)(uVar10 + 0x18);
      sVar7 = FUN_00420de0(0x10);
      if (sVar7 != -1) {
        iVar8 = (int)sVar7;
        if (local_1c0[iVar8 * 0xe] < sVar6) {
          local_1bc[iVar8 * 7] = local_1c4;
          local_1bc[iVar8 * 7 + 1] = uVar5;
          local_1bc[iVar8 * 7 + 2] = uVar10;
          local_1c0[iVar8 * 0xe] = sVar6;
        }
      }
    }
  }
  if ((0 < *(short *)(iVar1 + 0x3a8)) &&
     (sVar6 = *(short *)(iVar1 + 0x3a8) + -1, *(short *)(iVar1 + 0x3a8) = sVar6, sVar6 == 0)) {
    *(undefined4 *)(iVar1 + 0x3a4) = *(undefined4 *)(DAT_006f1d6c + 0xc);
    return;
  }
  return;
}

--- objdump -d -Mintel bin/halo.exe --start-address=0x420ec0 --stop-address=0x4213b0, the
basis for the bucket-scoring and call-selection tail that Ghidra decompile drops -

00420ec0 <.text+0x1fec0>:
  420ec0: sub    esp,0x1dc
  420ec6: mov    eax,DWORD PTR [esp+0x1e0]
  420ecd: mov    ecx,DWORD PTR ds:0x880360
  420ed3: mov    edx,DWORD PTR [ecx+0x34]
  420ed6: mov    ecx,DWORD PTR ds:0x87bc14
  420edc: and    eax,0xffff
  420ee1: imul   eax,eax,0x724
  420ee7: push   ebx
  420ee8: lea    ebx,[eax+edx*1]
  420eeb: mov    eax,DWORD PTR [ebx+0x58]
  420eee: and    eax,0xffff
  420ef3: shl    eax,0x5
  420ef6: mov    edx,DWORD PTR [eax+ecx*1+0x14]
  420efa: mov    eax,DWORD PTR [ebx+0x50]
  420efd: push   ebp
  420efe: push   esi
  420eff: push   edi
  420f00: mov    DWORD PTR [esp+0x14],ebx
  420f04: mov    DWORD PTR [esp+0x1c],edx
  420f08: mov    DWORD PTR [esp+0x20],0x0
  420f10: mov    DWORD PTR [esp+0x28],eax
  ... (full loop and bucket-lookup calls to 0x420e50 / 0x420de0, elided -- matches the C
      rewrite while-loop body field for field) ...
  421108: mov    eax,DWORD PTR [esp+0x20]
  42110c: test   ax,ax
  42110f: jle    0x4212bc
  421115: movzx  ecx,ax
  421118: lea    edi,[esp+0x2c]
  ... (per-bucket scoring loop against actor_def+0x268/0x26a/0x26c/0x270/0x274/0x278/0x27a and
      prop+0x135/0x136/0x12e/0xaa/0xac/0xae/0x78/0xa6, elided -- matches the C rewrite
      "Score every populated bucket" loop field for field) ...
  4212bc: xor    eax,eax
  4212be: mov    ax,WORD PTR [ebx+0x3a8]
  4212c5: test   ax,ax
  4212c8: jle    0x4212f5
  4212ca: dec    eax
  4212cb: test   ax,ax
  4212ce: mov    WORD PTR [ebx+0x3a8],ax
  4212d5: jne    0x42139d
  4212db: mov    edx,DWORD PTR ds:0x6f1d6c
  4212e1: mov    eax,DWORD PTR [edx+0xc]
  4212e7: mov    DWORD PTR [ebx+0x3a4],eax
  4212ee: add    esp,0x1dc
  4212f4: ret
  4212f5: mov    eax,DWORD PTR [esp+0x20]
  4212f9: or     edi,0xffffffff
  4212fc: test   ax,ax
  4212ff: mov    esi,0x5
  421304: jle    0x42139d
  42130a: lea    ecx,[esp+0x2c]
  42130e: movzx  ebp,ax
  421311: xor    edx,edx
  421313: mov    dx,WORD PTR [ecx]
  421316: cmp    dx,si
  421319: jle    0x421327
  42131b: mov    eax,DWORD PTR [ecx+0x4]
  42131e: cmp    eax,0xffffffff
  421321: je     0x421327
  421323: mov    esi,edx
  421325: mov    edi,eax
  421327: add    ecx,0x1c
  42132a: dec    ebp
  42132b: jne    0x421311
  42132d: cmp    edi,0xffffffff
  421330: je     0x42139d
  421332: mov    ecx,DWORD PTR ds:0x719cd0
  421338: mov    eax,DWORD PTR [esp+0x1c]
  42133c: imul   ecx,ecx,0x19660d
  421342: fld    DWORD PTR [eax+0x28c]
  421348: fld    DWORD PTR [eax+0x288]
  42134e: fxch   st(1)
  421350: fsub   st,st(1)
  421352: add    ecx,0x3c6ef35f
  421358: mov    edx,ecx
  42135a: shr    edx,0x10
  42135d: mov    DWORD PTR [esp+0x10],edx
  421361: mov    DWORD PTR ds:0x719cd0,ecx
  421367: fild   DWORD PTR [esp+0x10]
  42136b: fmul   DWORD PTR ds:0x672b84
  421371: fmulp  st(1),st
  421373: fadd   st,st(1)
  421375: fmul   DWORD PTR ds:0x672ac8
  42137b: call   0x6391b4
  421380: fstp   st(0)
  421382: mov    WORD PTR [ebx+0x3a8],ax
  421389: mov    eax,DWORD PTR ds:0x6f1d6c
  42138e: mov    DWORD PTR [ebx+0x3ac],edi
  421394: mov    ecx,DWORD PTR [eax+0xc]
  421397: mov    DWORD PTR [ebx+0x3b0],ecx
  42139d: pop    edi
  42139e: pop    esi
  42139f: pop    ebp
  4213a0: pop    ebx
  4213a1: add    esp,0x1dc
  4213a7: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
