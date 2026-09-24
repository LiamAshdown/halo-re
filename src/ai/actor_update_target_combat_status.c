// actor_update_target_combat_status  (Ghidra: actor_update_target_combat_status, already named)
// address 0x4200d0, size 415 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase2/results/ai_02.json -- looks up the actor's current target prop
//   (actor.target_unit_index, resolved with the module's 0x138 prop stride -- further
//   confirmation that field holds a prop index, not a raw unit handle), switches on the prop's
//   kind (0-5) to derive a status code stored into actor.target_combat_status, and updates an
//   obscured/aim flag at actor.unknown_27c from the tracked object's vitality_flags bit 2;
//   called immediately after every place that changes actor.target_unit_index.
// register convention: EAX -> actor_index; no other register operands are read. Ghidra's
//   "in_CX" is not actually a register read -- every switch case assigns it before use -- so
//   it is a plain local here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *prop_data;   // 0x008802c0
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> actor_index
// Recomputes the actor's cached combat-status code and aim/visibility flag for its currently
// selected target prop.
void actor_update_target_combat_status(datum_index actor_index)
{
    actor *self;
    prop *target;
    object *target_obj;
    int16_t status;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->target_unit_index == k_datum_index_none) {
        self->target_combat_status = 0;
        self->unknown_26c = k_datum_index_none;
        self->unknown_27c = 0;
        return;
    }

    target = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
    target_obj = ((object_header *)object_data->data)[target->object_index & 0xffff].data;

    switch (target->kind) {
    case 0:
        status = 0;
        self->target_unit_index = k_datum_index_none;
        self->unknown_26c = k_datum_index_none;
        break;
    case 1:
        status = 1;
        break;
    case 2:
    case 3:
        if (target->is_vault != 0) {
            status = 2;
        } else if (target->seen != 0) {
            status = 0xb;
        } else if (2 <= target->unknown_32) {
            status = 10;
        } else if (target->unknown_38 != 0 && target->unknown_38 != 1) {
            status = 7;
        } else if (2 < (int8_t)target->unknown_122 || 6.0f <= target->distance) {
            status = 8;
        } else {
            status = 9;
        }
        break;
    case 4:
        status = (int16_t)((target->unknown_b8 != 0) + 5);
        break;
    case 5:
        if (target->is_vault != 0) {
            status = 2;
        } else {
            status = (int16_t)(4 - (target->noticed_c != 0));
        }
        break;
    // UNSURE: Ghidra's switch has no default arm and `status` is left unset for any other
    // kind (types/ai.h documents kind up to 6, "the parented kind", which this switch does not
    // cover) -- preserved exactly rather than inventing a fallback, since the original leaves
    // whatever was in the CX register at that point.
    }

    self->target_combat_status = status;

    if (target->kind < 2 || 3 < target->kind) {
        self->unknown_27c = (uint8_t)(~(target_obj->vitality_flags >> 2) & 1);
    } else {
        self->unknown_27c = (uint8_t)(target->is_vault == 0);
        if (0 < target->unknown_32) {
            self->unknown_26c = target->unknown_8c;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4200d0):

void actor_update_target_combat_status(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  short in_CX;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(uint *)(iVar2 + 0x270) == 0xffffffff) {
    *(undefined2 *)(iVar2 + 0x268) = 0;
    *(undefined4 *)(iVar2 + 0x26c) = 0xffffffff;
    *(undefined1 *)(iVar2 + 0x27c) = 0;
    return;
  }
  iVar3 = (*(uint *)(iVar2 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar3 + 0x18) & 0xffff) * 0xc);
  switch(*(undefined2 *)(iVar3 + 0x24)) {
  case 0:
    in_CX = 0;
    *(undefined4 *)(iVar2 + 0x270) = 0xffffffff;
    *(undefined4 *)(iVar2 + 0x26c) = 0xffffffff;
    break;
  case 1:
    in_CX = 1;
    break;
  case 2:
  case 3:
    if (*(char *)(iVar3 + 0x127) == '\0') {
      if (*(char *)(iVar3 + 0x74) == '\0') {
        if (*(short *)(iVar3 + 0x32) < 2) {
          if ((*(short *)(iVar3 + 0x38) == 0) || (*(short *)(iVar3 + 0x38) == 1)) {
            if (('\x02' < *(char *)(iVar3 + 0x122)) || (6.0 <= *(float *)(iVar3 + 0x11c))) {
              in_CX = 8;
            }
            else {
              in_CX = 9;
            }
          }
          else {
            in_CX = 7;
          }
        }
        else {
          in_CX = 10;
        }
      }
      else {
        in_CX = 0xb;
      }
    }
    else {
      in_CX = 2;
    }
    break;
  case 4:
    in_CX = (*(char *)(iVar3 + 0xb8) != '\0') + 5;
    break;
  case 5:
    if (*(char *)(iVar3 + 0x127) == '\0') {
      in_CX = 4 - (ushort)(*(char *)(iVar3 + 0xbb) != '\0');
    }
    else {
      in_CX = 2;
    }
  }
  *(short *)(iVar2 + 0x268) = in_CX;
  if ((*(short *)(iVar3 + 0x24) < 2) || (3 < *(short *)(iVar3 + 0x24))) {
    *(byte *)(iVar2 + 0x27c) = ~(*(byte *)(iVar1 + 0x106) >> 2) & 1;
  }
  else {
    *(bool *)(iVar2 + 0x27c) = *(char *)(iVar3 + 0x127) == '\0';
    if (0 < *(short *)(iVar3 + 0x32)) {
      *(undefined4 *)(iVar2 + 0x26c) = *(undefined4 *)(iVar3 + 0x8c);
      return;
    }
  }
  return;
}
#endif
