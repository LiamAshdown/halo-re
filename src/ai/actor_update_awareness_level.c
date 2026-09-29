// actor_update_awareness_level  (Ghidra: actor_update_awareness_level, already named)
// address 0x420290, size 258 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump (branch-for-branch; offsets 0x6a/0x6c/0x72/0x74/0x78/0x268/0x34a, ai_globals +3/+4/+6).)
// evidence: out/phase2/results/ai_02.json -- consumes the pending perception event recorded by
//   0x422070 (actor.perception_event / perception_event_data), merges it with a per-target
//   -status minimum threshold table indexed by actor.target_combat_status, and advances
//   actor.combat_status (awareness/vitality grade) plus streak counters at unknown_7c/0x80/0x84,
//   setting unknown_8c once the grade exceeds 6.
// register convention: EAX -> actor_index; no other register operands are read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// Per-target-combat-status minimum grade table, indexed by actor.target_combat_status (0-0xb
// per the switch in actor_update_target_combat_status). No name is attributed elsewhere.
extern int16_t actor_combat_status_min_grade[]; // 0x00655880

// blam-cc: EAX -> actor_index
// Advances the actor's alertness/awareness state machine each tick from the highest-priority
// perception event and the current target's combat status.
void actor_update_awareness_level(datum_index actor_index)
{
    actor *self;
    int16_t event;
    int16_t burst_counter;
    int16_t status_min_grade;
    int16_t event_floor;
    int16_t old_grade;
    int16_t new_grade;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    event = self->perception_event;
    if (0 < event) {
        if (self->suspicion_status < event) {
            self->suspicion_status = event;
            self->suspicion_timer = self->perception_event_data;
        } else if (self->suspicion_status == event) {
            if (self->suspicion_timer <= self->perception_event_data) {
                self->suspicion_timer = self->perception_event_data;
            }
        }
        self->perception_event = 0;
    }

    burst_counter = self->minimum_combat_status;
    status_min_grade = actor_combat_status_min_grade[self->target_combat_status];
    event_floor = (burst_counter <= status_min_grade) ? status_min_grade : burst_counter;

    old_grade = self->suspicion_status;
    new_grade = old_grade;
    if (old_grade <= event_floor) {
        new_grade = (burst_counter <= status_min_grade) ? status_min_grade : burst_counter;
    }
    self->combat_status = new_grade;

    if (old_grade < new_grade) {
        self->suspicion_status = 0;
    }

    if (self->awareness_level < 3) {
        self->ticks_in_combat = 0;
    } else {
        self->ticks_in_combat = self->ticks_in_combat + 1;
    }

    if (new_grade == 0) {
        self->ticks_alerted = 0;
    } else {
        self->ticks_alerted = self->ticks_alerted + 1;
        if (3 < new_grade) {
            self->ticks_threatened = self->ticks_threatened + 1;
            self->ticks_since_threatened = 0;
            goto have_streaks;
        }
    }
    self->ticks_threatened = 0;
    if (self->ticks_since_threatened != -1) {
        self->ticks_since_threatened = self->ticks_since_threatened + 1;
    }

have_streaks:
    if (6 < new_grade) {
        self->has_engaged = 1;
    }
}

#if 0
Original Ghidra decompilation (0x420290):

void actor_update_awareness_level(void)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short sVar4;
  uint in_EAX;
  int iVar5;
  short sVar6;
  int iVar7;

  iVar5 = (in_EAX & 0xffff) * 0x724;
  sVar1 = *(short *)(iVar5 + 0x34a + *(int *)(DAT_00880360 + 0x34));
  iVar5 = iVar5 + *(int *)(DAT_00880360 + 0x34);
  if (0 < sVar1) {
    if (*(short *)(iVar5 + 0x74) < sVar1) {
      *(short *)(iVar5 + 0x74) = sVar1;
      *(undefined4 *)(iVar5 + 0x78) = *(undefined4 *)(iVar5 + 0x34c);
    }
    else if (*(short *)(iVar5 + 0x74) == sVar1) {
      iVar7 = *(int *)(iVar5 + 0x78);
      if (*(int *)(iVar5 + 0x78) <= *(int *)(iVar5 + 0x34c)) {
        iVar7 = *(int *)(iVar5 + 0x34c);
      }
      *(int *)(iVar5 + 0x78) = iVar7;
    }
    *(undefined2 *)(iVar5 + 0x34a) = 0;
  }
  sVar1 = *(short *)(iVar5 + 0x72);
  sVar2 = *(short *)(&DAT_00655880 + *(short *)(iVar5 + 0x268) * 2);
  sVar4 = sVar1;
  if (sVar1 <= sVar2) {
    sVar4 = sVar2;
  }
  sVar3 = *(short *)(iVar5 + 0x74);
  sVar6 = sVar3;
  if ((sVar3 <= sVar4) && (sVar6 = sVar1, sVar1 <= sVar2)) {
    sVar6 = sVar2;
  }
  *(short *)(iVar5 + 0x6e) = sVar6;
  if (sVar3 < sVar6) {
    *(undefined2 *)(iVar5 + 0x74) = 0;
  }
  if (*(short *)(iVar5 + 0x6a) < 3) {
    *(undefined4 *)(iVar5 + 0x7c) = 0;
  }
  else {
    *(int *)(iVar5 + 0x7c) = *(int *)(iVar5 + 0x7c) + 1;
  }
  if (sVar6 == 0) {
    *(undefined4 *)(iVar5 + 0x80) = 0;
  }
  else {
    *(int *)(iVar5 + 0x80) = *(int *)(iVar5 + 0x80) + 1;
    if (3 < sVar6) {
      *(int *)(iVar5 + 0x84) = *(int *)(iVar5 + 0x84) + 1;
      *(undefined4 *)(iVar5 + 0x88) = 0;
      goto LAB_00420361;
    }
  }
  *(undefined4 *)(iVar5 + 0x84) = 0;
  if (*(int *)(iVar5 + 0x88) != -1) {
    *(int *)(iVar5 + 0x88) = *(int *)(iVar5 + 0x88) + 1;
  }
LAB_00420361:
  if (6 < sVar6) {
    *(undefined1 *)(iVar5 + 0x8c) = 1;
  }
  return;
}
#endif
