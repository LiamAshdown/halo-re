// actor_should_throw_grenade  (Ghidra: actor_should_throw_grenade, already named)
// address 0x40b840, size 224 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/ai.h actor.unknown_1ca/combat_status/unknown_26c/unknown_1c0/unknown_378/
//   order_committed; types/tags.h Actor.hide_target_not_visible_time (0x2d8)/
//   cover_damage_threshold (0x324), already-named fields; phase-4 summary "returns whether
//   the actor is currently eligible to throw a grenade, based on cooldowns, combat status,
//   and vitality thresholds".
// register convention: actor index in EAX, a "force" flag in the recognized stack parameter.
//   // blam-cc: EAX -> actor_index, stack -> force
// UNSURE: the original packs its bool result together with unrelated NaN/sign/zero
// classification bits from two float comparisons (the CONCAT2x patterns); simplified here to
// a plain bool, matching the treatment used elsewhere in this session for the same pattern.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "game.h"

extern data_array *actor_data;       // 0x00880360
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

// The result is produced in AL only (the Ghidra listing returns CONCAT31(<garbage>, uVar12)),
// so the return type is a byte, not the int Ghidra recovered.
uint8_t actor_should_throw_grenade(uint32_t actor_index, char force)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
    uint8_t eligible = 1;

    if (force == 0 && a->playfight == 0) {
        if (actor_def->hide_target_not_visible_time > 0.0f) {
            if (a->combat_status < 7) {
                if (a->target_last_seen_time != (datum_index)k_datum_index_none) {
                    int16_t delay = (int16_t)(actor_def->hide_target_not_visible_time * 30.0f);
                    if (game_time->game_time < delay + (int32_t)a->target_last_seen_time) {
                        eligible = 0;
                    }
                }
            } else {
                eligible = 0;
            }
        }
        if (!(actor_def->cover_damage_threshold < 0.0f) && !(actor_def->cover_damage_threshold == 0.0f)) {
            if (a->recent_body_damage < actor_def->cover_damage_threshold) {
                eligible = 0;
            }
        }
    }

    if (a->berserking != 0) {
        eligible = 0;
    }
    if (a->order_committed != 0) {
        return 0;
    }
    return eligible;
}

#if 0
Original Ghidra decompilation (0x40b840):

int actor_should_throw_grenade(char param_1)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  uint in_EAX;
  int iVar6;
  undefined4 uVar7;
  uint uVar8;
  uint3 uVar10;
  undefined1 uVar12;
  undefined3 uVar9;
  undefined2 uVar11;

  iVar6 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar3 = *(int *)((*(uint *)(iVar6 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar9 = (undefined3)(DAT_0087bc14 >> 8);
  uVar7 = CONCAT31(uVar9,param_1);
  uVar12 = 1;
  if ((param_1 == '\0') &&
     (uVar7 = CONCAT31(uVar9,*(char *)(iVar6 + 0x1ca)), *(char *)(iVar6 + 0x1ca) == '\0')) {
    uVar8 = DAT_0087bc14 & 0xffff0000;
    uVar11 = (undefined2)(DAT_0087bc14 >> 0x10);
    if (0.0 < *(float *)(iVar3 + 0x2d8)) {
      if (*(short *)(iVar6 + 0x6e) < 7) {
        iVar4 = *(int *)(iVar6 + 0x26c);
        if (iVar4 != -1) {
          sVar5 = __ftol();
          uVar8 = *(uint *)(DAT_006f1d6c + 0xc);
          if ((int)uVar8 < sVar5 + iVar4) {
            uVar12 = 0;
          }
        }
        uVar11 = (undefined2)(uVar8 >> 0x10);
      }
      else {
        uVar12 = 0;
      }
    }
    fVar1 = *(float *)(iVar3 + 0x324);
    uVar7 = CONCAT22(uVar11,(ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                            (ushort)(fVar1 == 0.0) << 0xe);
    if (fVar1 < 0.0 == 0 && (fVar1 == 0.0) == 0) {
      fVar1 = *(float *)(iVar6 + 0x1c0);
      fVar2 = *(float *)(iVar3 + 0x324);
      uVar7 = CONCAT22(uVar11,(ushort)(fVar1 < fVar2) << 8 |
                              (ushort)(NAN(fVar1) || NAN(fVar2)) << 10 |
                              (ushort)(fVar1 == fVar2) << 0xe);
      if (fVar1 < fVar2) {
        uVar12 = 0;
      }
    }
  }
  uVar10 = (uint3)((uint)uVar7 >> 8);
  if (*(char *)(iVar6 + 0x378) != '\0') {
    uVar12 = 0;
  }
  if (*(char *)(iVar6 + 0x160) != '\0') {
    return (uint)uVar10 << 8;
  }
  return CONCAT31(uVar10,uVar12);
}
#endif
