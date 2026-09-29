// actor_set_combat_alert_flag  (Ghidra: actor_set_combat_alert_flag, already named)
// address 0x421a40, size 163 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase2/results/ai_02.json -- only acts when unaff_BL differs from
//   actor+0x378; stores the new flag, clears actor+0x379, and if the actor has no cluster
//   (actor.swarm==0) sets/clears bit 0x80 of the controlled unit's object flags (unit+0x204),
//   otherwise propagates the same bit across every actor sharing the swarm/cluster (walking
//   the object+0x1fc chain via actor.cluster_unit_index); also sets actor+0x375 when turning
//   the flag on. types/units.h names bit 0x80 of unit_flags `_unit_flag_disoriented`, on
//   different evidence (0x5705a0's stun timer) than this call site.
// register convention: EAX -> actor_index, BL -> new_flag (unaff_BL).
//   // blam-cc: EAX -> actor_index, EBX -> new_flag
//
// UNSURE: the cluster-propagation branch below sets a BYTE at unit+0x106 (vitality_flags' low
// byte) with the same 0x80 bit, not the dword unit_flags field the single-unit branch touches
// at unit+0x204. Preserved exactly; the two branches genuinely touch different fields.
// UNSURE: reusing _unit_flag_disoriented for a combat-alert toggle is only a byte-offset match,
// not a confirmed semantic one -- left for the hook-verification pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> actor_index, EBX -> new_flag
// Toggles the actor's (and, for grouped actors, its whole cluster's) combat-alert object flag
// when the alert state changes.
void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag)
{
    actor *self;
    object *unit_obj;
    datum_index cluster_unit;
    object *cluster_obj;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if ((char)new_flag == self->berserking) {
        return;
    }
    self->berserking = new_flag;
    self->berserk_announced = 0;

    if (self->swarm == 0) {
        unit_obj = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
        if (new_flag == 0) {
            ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->flags &= 0xffffff7f;
        } else {
            ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->flags |= 0x80;
        }
    } else {
        cluster_unit = self->cluster_unit_index;
        while (cluster_unit != k_datum_index_none) {
            cluster_obj = ((object_header *)object_data->data)[cluster_unit & 0xffff].data;
            cluster_obj->vitality_flags |= 0x80;
            cluster_unit = ((unit_data *)((uint8_t *)cluster_obj + k_unit_data_offset))->swarm_next_unit_index;
        }
    }

    if (new_flag != 0) {
        self->always_charge = 1;
    }
}

#if 0
Original Ghidra decompilation (0x421a40):

void actor_set_combat_alert_flag(void)

{
  byte *pbVar1;
  uint *puVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint in_EAX;
  int iVar6;
  char unaff_BL;

  iVar6 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (unaff_BL != *(char *)(iVar6 + 0x378)) {
    *(char *)(iVar6 + 0x378) = unaff_BL;
    *(undefined1 *)(iVar6 + 0x379) = 0;
    iVar5 = DAT_008603b0;
    if (*(char *)(iVar6 + 6) == '\0') {
      iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar6 + 0x18) & 0xffff) * 0xc)
      ;
      if (unaff_BL == '\0') {
        puVar2 = (uint *)(iVar5 + 0x204);
        *puVar2 = *puVar2 & 0xffffff7f;
        return;
      }
      puVar2 = (uint *)(iVar5 + 0x204);
      *puVar2 = *puVar2 | 0x80;
    }
    else {
      uVar3 = *(uint *)(iVar6 + 0x24);
      while (uVar3 != 0xffffffff) {
        iVar4 = *(int *)(*(int *)(iVar5 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
        pbVar1 = (byte *)(iVar4 + 0x106);
        *pbVar1 = *pbVar1 | 0x80;
        uVar3 = *(uint *)(iVar4 + 0x1fc);
      }
    }
    if (unaff_BL != '\0') {
      *(undefined1 *)(iVar6 + 0x375) = 1;
    }
  }
  return;
}
#endif
