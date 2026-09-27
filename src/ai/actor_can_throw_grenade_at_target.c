// actor_can_throw_grenade_at_target  (Ghidra: actor_can_throw_grenade_at_target, renamed)
// address 0x40d9c0, size 318 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x40d9c0..0x40dafd; callee conventions and offsets checked)
// evidence: phase-4 summary "determines whether the actor can currently throw a grenade at
// its target by validating a clear, safe trajectory"; calls the grenade-landing helpers
// actor_find_grenade_landing_spot / actor_score_blast_area_clear / actor_commit_grenade_toss
// (all rewritten in this module).
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index
// UNSURE: four ActorVariant offsets (0x186, 0x188, 0x19c, 0x1a8) are read here as raw
// floats/int16s; types/tags.h's ActorVariant layout does not land 0x186 on a field boundary
// for this use, so they are kept as raw offsets rather than asserting a field name the
// evidence contradicts.
// UNSURE: FUN_0046fe70 is called with no visible argument; treated as taking one float
// argument passed through the FPU stack (ST0), per the idiom used elsewhere in this module.
// Note: the local Ghidra calls "local_14" is the current game tick before the landing-spot
// search, then is silently reused afterward as actor_find_grenade_landing_spot's
// relationship_object_index out-value (its final use, into the third arg of the commit
// call). Modeled here as two distinct C locals for clarity: `now` and `relationship`.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "ai.h"

extern data_array *actor_data;         // 0x00880360
extern data_array *encounter_data;     // 0x008802c8
extern tag_instance *tag_instances;    // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX table, AX index: the difficulty scale

extern uint8_t actor_find_grenade_landing_spot(datum_index actor_index, real_point3d *out_point, datum_index *out_target_handle, int32_t *out_relationship); // 0x410c90, this module
extern uint8_t actor_score_blast_area_clear(datum_index actor_index, float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count); // this module
extern uint32_t actor_commit_grenade_toss(datum_index actor_index, real_point3d *point, uint32_t object_handle, uint32_t param_3); // 0x411180, this module

// blam-cc: EAX -> actor_index
uint8_t actor_can_throw_grenade_at_target(datum_index actor_index)
{
    actor *self;
    ActorVariant *variant;
    int32_t now;
    int16_t hostile_count;
    real_point3d point;
    datum_index target_handle;
    int32_t relationship;
    float random_wait;
    int16_t random_wait_ticks;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;
    now = game_time->game_time; // +0x0c

    if (self->active_unit_index != (datum_index)k_datum_index_none) {
        return 0;
    }

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = (encounter *)((uint8_t *)encounter_data->data +
                                        (self->encounter_index & 0xffff) * sizeof(encounter));
        int32_t squad_deadline = enc->unknown_5c;

        // 0x40da35..0x40da6f: the variant's wait (seconds) times difficulty scale 0x18 for the encounter's team
        // (encounter+2), doubled when actor+0x1ca is set, then converted to ticks (x30, __ftol)
        random_wait = *(float *)((uint8_t *)variant + 0x1a8) *
                      weapon_get_zoom_fov_resolved(0x18, *(int16_t *)((uint8_t *)enc + 2));
        if (self->unknown_1ca != 0) {
            random_wait = random_wait + random_wait;
        }
        if (squad_deadline != -1) {
            random_wait_ticks = (int16_t)(int32_t)(random_wait * 30.0f); // __ftol, then movsx ax
            if (now < random_wait_ticks + squad_deadline) {
                return 0;
            }
        }
    }

    if (actor_find_grenade_landing_spot(actor_index, &point, &target_handle, &relationship) != 0 &&
        actor_score_blast_area_clear(actor_index, *(float *)((uint8_t *)variant + 0x188),
                     *(float *)((uint8_t *)variant + 0x19c), &point, &hostile_count) != 0 &&
        *(int16_t *)((uint8_t *)variant + 0x186) <= (int16_t)hostile_count &&
        actor_commit_grenade_toss(actor_index, &point, target_handle, (uint32_t)relationship) != 0) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40d9c0):

undefined4 FUN_0040d9c0(void)

{
  uint uVar1;
  char cVar2;
  short sVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  undefined1 local_c [12];

  iVar4 = (in_EAX & 0xffff) * 0x724;
  iVar5 = iVar4 + *(int *)(DAT_00880360 + 0x34);
  iVar4 = *(int *)((*(uint *)(iVar4 + 0x5c + *(int *)(DAT_00880360 + 0x34)) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  local_14 = *(int *)(DAT_006f1d6c + 0xc);
  if (*(int *)(iVar5 + 0x158) != -1) {
    return 0;
  }
  uVar1 = *(uint *)(iVar5 + 0x34);
  if (uVar1 != 0xffffffff) {
    iVar5 = *(int *)(DAT_008802c8 + 0x34);
    local_18 = *(undefined4 *)(iVar4 + 0x1a8);
    FUN_0046fe70();
    iVar5 = *(int *)((uVar1 & 0xffff) * 0x6c + iVar5 + 0x5c);
    if ((iVar5 != -1) && (sVar3 = __ftol(), local_14 < sVar3 + iVar5)) {
      return 0;
    }
  }
  cVar2 = FUN_00410c90(&local_10,&local_14);
  if ((((cVar2 != '\0') &&
       (cVar2 = actor_score_blast_area_clear(*(undefined4 *)(iVar4 + 0x188),*(undefined4 *)(iVar4 + 0x19c),local_c,
                             &local_18), cVar2 != '\0')) &&
      (*(short *)(iVar4 + 0x186) <= (short)local_18)) &&
     (cVar2 = FUN_00411180(local_c,local_10,local_14), cVar2 != '\0')) {
    return 1;
  }
  return 0;
}
#endif
