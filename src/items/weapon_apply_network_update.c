// weapon_apply_network_update  (Ghidra: FUN_004c6070; named from
// out/phase4/items_functions.md, "Applies an incoming network state update to a weapon object
// if it is newer than the item's current state, refreshing its transform and ammo fields")
// address 0x4c6070, size 537 bytes
// name confidence: 0.35   rewrite confidence: 0.75 (raised by the phase-4 verification pass, which
//   re-derived this function against objdump disassembly rather than the decompilation; see the
//   notes in the body)
// evidence: types/items.h weapon_data (network_state at 0x2e4, network_baseline_index 0x2e1,
//   network_sequence 0x2e2, last_update_valid 0x310, last_update_state 0x314,
//   magazines[].rounds_unloaded, age); types/objects.h object (position 0x05c, velocity 0x068,
//   forward 0x074, up 0x080); the 11-dword block copy (0x2e4..0x310) proves weapon_network_state
//   is exactly 0x2c bytes, exactly as the struct comment in types/items.h states.
// register convention: item index in the first parameter; the incoming update record pointer in
// the second. Both are already Ghidra-recognized __cdecl parameters.
// blam-cc: stack -> (item_index, update_record)
// UNSURE: `update_record` is a generic message-delta-system record belonging to the networking
// module. Its header sub-record at +0x44 is now weapon_network_update_header in types/items.h;
// the dword at +0x00 is a *pointer* to the record's mode word, not the mode word itself --
// objdump -d -M intel bin/halo.exe shows
//     4c61d3: mov ecx,DWORD PTR [esp+0x44]     ; update_record
//     4c61d7: mov edx,DWORD PTR [ecx]          ; update_record[0]
//     4c61d9: cmp DWORD PTR [edx],0x0          ; *(int *)update_record[0]
// so the mode test is a double indirection (Ghidra spells it `*(int *)*param_2`). Mode 1 is the
// incremental update, mode 0 the full/creation one. The object flags bit 0x08000000 tested and
// set here does not appear in types/objects.h's object_flags enum and is used as decompiled.
// NOTE on the distance test: Ghidra writes the subtraction back into local_2c, which would make
// last_update_state carry a delta instead of a position. It does not -- 0x4c61e8..0x4c6203 is
// pure x87 (`fld [esp+0x10]; fsub [ebp+0x5c]`, three times) and never stores, and both
// `lea esi,[esp+0x10]` before the 0x4f52c0 call and the closing `rep movs` read the unmodified
// snapshot. The dx/dy/dz temporaries below reproduce the real behaviour, not Ghidra's.
// reconciled: R26 object +0x18/+0x1c/+0x44/+0x48 raw writes -> network_position_valid/network_position/network_velocity_valid/network_velocity

// FIXED 2026-09-28 (networking call audit): message_delta_decode_compound_field / _forced / _staged take the
// decode context first (EAX) and the destination second (ECX); the calls here had the context missing or the two
// swapped.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0
extern real weapon_network_update_position_tolerance; // 0x00696550

// sqrt is a single x87/SSE instruction sequence in the original code (Ghidra's SQRT());
// declared locally instead of via <math.h> because -I types shadows that header name with
// types/math.h.
extern double sqrt(double x);

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern uint8_t message_delta_decode_compound_field_forced(void *decode_context, void *destination,
    int32_t changed_offset, uint8_t force); // 0x4ec600, EAX context, ECX destination, EDX baseline, stack force
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
    // (see src/items/weapon_predict_ammo.c for the resolved convention). UNSURE: this call
    // site sets up no ECX of its own, so it is passed as 0 here.
extern void object_set_position_and_recalculate(real_point3d *position, datum_index object_index); // 0x4f52c0

// Applies an incoming network state update to a weapon object, rejecting it if it is not newer
// than the item's own recorded baseline/sequence, then refreshes the object's position/velocity
// (relinking into the world when it moved far enough or a flag demands it) and the ammo/age
// fields carried in the update.
void weapon_apply_network_update(datum_index item_index, uint32_t *update_record)
{
    object *item_obj;
    weapon_data *wd;
    weapon_network_update_header *header;
    weapon_network_state snapshot;
    uint8_t accept;
    real dx, dy, dz;

    item_obj = object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0) {
        message_delta_decode_compound_field_staged(update_record);
        return;
    }
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    header = (weapon_network_update_header *)update_record[0x11];

    if ((item_obj->flags & 0x8000000) != 0 && *(int32_t *)update_record[0] == 1 &&
        (header->baseline_index != wd->network_baseline_index ||
         (header->sequence <= wd->network_sequence &&
          (int)((uint32_t)(header->sequence - wd->network_sequence) + 0xff) > 0x1d))) {
        message_delta_decode_compound_field_staged(update_record);
        return;
    }

    snapshot = wd->network_state; // 11-dword block copy, see file header

    if (*(int32_t *)update_record[0] == 1) {
        accept = message_delta_decode_compound_field_forced(update_record, &snapshot, (int32_t)&wd->network_state, 0);
    } else {
        accept = message_delta_decode_compound_field(update_record, &snapshot);
    }

    if (accept != 0) {
        wd->network_sequence = header->sequence;
        item_obj->flags = item_obj->flags | 0x8000000;

        if (header->force_baseline != 0) {
            wd->network_baseline_index = header->baseline_index;
            wd->network_state = snapshot;
        }

        item_obj->velocity = snapshot.velocity;
        // the network interpolation block (object 0x018..0x054, objects.h R26)
        item_obj->network_position = snapshot.position;
        item_obj->network_velocity = snapshot.velocity;
        item_obj->network_position_valid = 1;
        item_obj->network_velocity_valid = 1;

        if (wd->magazines[0].state != 1) {
            wd->magazines[0].rounds_unloaded = snapshot.rounds_unloaded[0];
        }
        if (wd->magazines[1].state != 1) {
            wd->magazines[1].rounds_unloaded = snapshot.rounds_unloaded[1];
        }
        if (*(int32_t *)update_record[0] == 0) {
            wd->age = snapshot.age;
        }

        dx = snapshot.position.x - item_obj->position.x;
        dy = snapshot.position.y - item_obj->position.y;
        dz = snapshot.position.z - item_obj->position.z;
        if ((item_obj->flags & 0x800) != 0 &&
            (weapon_network_update_position_tolerance < (real)sqrt((double)(dy * dy + dx * dx + dz * dz)) ||
             (item_obj->flags & 0x20) != 0 || header->force_baseline != 0)) {
            object_set_position_and_recalculate(&snapshot.position, item_index);
        }

        wd->last_update_valid = 1;
        wd->last_update_state = snapshot;
    }
}

#if 0
Original Ghidra decompilation (0x4c6070):

void FUN_004c6070(uint param_1,undefined4 *param_2)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  float *pfVar5;
  float *pfVar6;
  float local_2c [4];
  undefined4 local_1c;
  undefined4 local_18;
  undefined2 local_8;
  undefined2 local_6;
  undefined4 local_4;

  iVar2 = object_try_and_get(4);
  if (iVar2 == 0) {
    FUN_004ec670();
    return;
  }
  iVar4 = param_2[0x11];
  if ((((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0x10) &
        0x8000000) != 0) && (*(int *)*param_2 == 1)) &&
     ((*(char *)(iVar4 + 4) != *(char *)(iVar2 + 0x2e1) ||
      (((uint)*(byte *)(iVar4 + 5) <= (uint)*(byte *)(iVar2 + 0x2e2) &&
       (0x1d < (int)(((uint)*(byte *)(iVar4 + 5) - (uint)*(byte *)(iVar2 + 0x2e2)) + 0xff))))))) {
    FUN_004ec670();
    return;
  }
  pfVar5 = (float *)(iVar2 + 0x2e4);
  pfVar6 = local_2c;
  for (iVar3 = 0xb; iVar3 != 0; iVar3 = iVar3 + -1) {
    *pfVar6 = *pfVar5;
    pfVar5 = pfVar5 + 1;
    pfVar6 = pfVar6 + 1;
  }
  if (*(int *)*param_2 == 1) {
    cVar1 = FUN_004ec600(0);
  }
  else {
    cVar1 = FUN_004ec590();
  }
  if (cVar1 != '\0') {
    *(undefined1 *)(iVar2 + 0x2e2) = *(undefined1 *)(iVar4 + 5);
    *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) | 0x8000000;
    if (*(char *)(iVar4 + 6) != '\0') {
      *(undefined1 *)(iVar2 + 0x2e1) = *(undefined1 *)(iVar4 + 4);
      pfVar5 = local_2c;
      pfVar6 = (float *)(iVar2 + 0x2e4);
      for (iVar3 = 0xb; iVar3 != 0; iVar3 = iVar3 + -1) {
        *pfVar6 = *pfVar5;
        pfVar5 = pfVar5 + 1;
        pfVar6 = pfVar6 + 1;
      }
    }
    *(float *)(iVar2 + 0x68) = local_2c[3];
    *(undefined4 *)(iVar2 + 0x6c) = local_1c;
    *(undefined4 *)(iVar2 + 0x70) = local_18;
    *(float *)(iVar2 + 0x1c) = local_2c[0];
    *(float *)(iVar2 + 0x20) = local_2c[1];
    *(float *)(iVar2 + 0x24) = local_2c[2];
    *(float *)(iVar2 + 0x48) = local_2c[3];
    *(undefined4 *)(iVar2 + 0x4c) = local_1c;
    *(undefined4 *)(iVar2 + 0x50) = local_18;
    *(undefined1 *)(iVar2 + 0x18) = 1;
    *(undefined1 *)(iVar2 + 0x44) = 1;
    if (*(short *)(iVar2 + 0x2b0) != 1) {
      *(undefined2 *)(iVar2 + 0x2b6) = local_8;
    }
    if (*(short *)(iVar2 + 700) != 1) {
      *(undefined2 *)(iVar2 + 0x2c2) = local_6;
    }
    if (*(int *)*param_2 == 0) {
      *(undefined4 *)(iVar2 + 0x240) = local_4;
    }
    local_2c[0] = local_2c[0] - *(float *)(iVar2 + 0x5c);
    local_2c[1] = local_2c[1] - *(float *)(iVar2 + 0x60);
    local_2c[2] = local_2c[2] - *(float *)(iVar2 + 100);
    if (((*(uint *)(iVar2 + 0x10) & 0x800) != 0) &&
       (((_DAT_00696550 <
          SQRT(local_2c[1] * local_2c[1] + local_2c[0] * local_2c[0] + local_2c[2] * local_2c[2]) ||
         ((*(uint *)(iVar2 + 0x10) & 0x20) != 0)) || (*(char *)(iVar4 + 6) != '\0')))) {
      object_set_position_and_recalculate();
    }
    *(undefined1 *)(iVar2 + 0x310) = 1;
    pfVar5 = local_2c;
    pfVar6 = (float *)(iVar2 + 0x314);
    for (iVar4 = 0xb; iVar4 != 0; iVar4 = iVar4 + -1) {
      *pfVar6 = *pfVar5;
      pfVar5 = pfVar5 + 1;
      pfVar6 = pfVar6 + 1;
    }
  }
  return;
}
#endif
