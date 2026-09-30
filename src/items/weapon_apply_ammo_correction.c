// weapon_apply_ammo_correction  (Ghidra: FUN_004c3870; named from
// out/phase4/items_functions.md, "Applies a server-confirmed ammo correction to a weapon
// trigger and clears the pending-prediction dirty flag")
// address 0x4c3870, size 138 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: network message dispatch case 0x2d (k_message_weapon_reload_end); types/items.h
//   weapon_magazine_ammo_message (the 0xc-byte stack buffer), weapon_magazine_state
//   (state/state_ticks/rounds_unloaded/rounds_loaded at 0x2b0 + i*0x0c),
//   _weapon_magazine_chamber_pending (2) and
//   weapon_flags._weapon_ammo_prediction_pending_bit.
// register convention: the message record in EAX.
// blam-cc: EAX -> message_record
// The four "incoming ammo message" handlers in this module (0x4c25a0, 0x4c3530, 0x4c3870,
// 0x4c4ac0) share one prologue that Ghidra cannot recover, because every argument travels in a
// register or in an un-typed stack buffer. objdump -d -M intel bin/halo.exe resolves it -- this
// is 0x4c3530, and the other three are the same shape:
//     4c3530: mov ecx,DWORD PTR [eax]      ; eax = the record, in EAX
//     4c3532: mov edx,DWORD PTR [ecx]      ; the mode word is behind TWO indirections
//     4c3537: test edx,edx / jne <reject>  ; nonzero -> message_delta_decode_compound_field_staged, drop the message
//     4c353b: lea ecx,[esp]                ; ECX = a stack buffer for the decoded message
//     4c353e: call 0x4ec590                ; decodes into it, returns success in AL
//     4c3547: mov eax,DWORD PTR [esp]      ; decoded.object_hash
//     4c354a: or  ecx,0xffffffff           ; datum_index defaults to -1
//     4c3551: mov edx,DWORD PTR ds:0x687130
//     4c3557: mov ecx,DWORD PTR [edx+0x28] ; the hash -> datum_index array
//     4c355a: mov ecx,DWORD PTR [ecx+eax*4]
//     4c355d: push 0x4 / call 0x4f6ec0     ; object_try_and_get(ECX = index, mask = weapon)
// So: message_delta_decode_compound_field takes the destination record in ECX (it is not the no-argument predicate the
// decompilation suggests), the hash is resolved through object_network_id_table + 0x28
// exactly as src/objects/object_apply_linked_impulse.c already spells it, and the stack buffer
// is a message record types/items.h already describes. Ghidra's un-assigned `local_*` shorts are
// that buffer's fields, so they are named here instead of left as zero placeholders.
// NOTE: the magazine index is sign-extended (`movsx ecx,WORD PTR [esp+0x4]`).
// NOTE: 0x4c38b9 is `cmp WORD PTR [eax+edx*4+0x2b0],0x1` -- a compare of the magazine's current
// state against _weapon_magazine_reloading whose flags are then never tested; no branch follows
// it. It is a dead compare in the shipped code (the reload state is overwritten unconditionally
// two instructions later), so it is not reproduced as a condition here.

// FIXED 2026-09-28 (networking call audit): message_delta_decode_compound_field / _forced / _staged take the
// decode context first (EAX) and the destination second (ECX); the calls here had the context missing or the two
// swapped.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"

extern network_id_table *object_network_id_table; // 0x00687130
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// Applies a host-confirmed ammo correction to one magazine, forces it into the chamber-pending
// state with a fresh (zero) state timer, and clears the prediction-pending flag.
void weapon_apply_ammo_correction(void **message_record)
{
    weapon_magazine_ammo_message decoded;
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_state *magazine;
    datum_index item_index;

    if (*(int32_t *)*message_record != 0) {
        message_delta_decode_compound_field_staged(message_record);
        return;
    }
    if ((int8_t)message_delta_decode_compound_field(message_record, &decoded) == 0) {
        return;
    }

    item_index = (datum_index)0xffffffff;
    if (decoded.object_hash != 0) {
        item_index = object_network_id_table->handles[decoded.object_hash];
    }

    item_obj = object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0 || item_obj->network_role != 1) {
        return;
    }

    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    magazine = &wd->magazines[decoded.magazine_index];
    magazine->rounds_unloaded = decoded.rounds_unloaded;
    magazine->rounds_loaded = decoded.rounds_loaded;
    magazine->state_ticks = 0;
    magazine->state = _weapon_magazine_chamber_pending;
    wd->flags = wd->flags & ~(uint32_t)_weapon_ammo_prediction_pending_bit;
}

#if 0
Original Ghidra decompilation (0x4c3870):

void FUN_004c3870(void)

{
  undefined2 *puVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  short local_8;
  undefined2 local_6;
  undefined2 local_4;

  if (*(int *)*in_EAX == 0) {
    cVar2 = FUN_004ec590();
    if (((cVar2 != '\0') && (iVar3 = object_try_and_get(4), iVar3 != 0)) &&
       (*(int *)(iVar3 + 4) == 1)) {
      puVar1 = (undefined2 *)(iVar3 + 0x2b0 + local_8 * 0xc);
      puVar1[3] = local_6;
      puVar1[4] = local_4;
      puVar1[1] = 0;
      *puVar1 = 2;
      *(uint *)(iVar3 + 0x22c) = *(uint *)(iVar3 + 0x22c) & 0xfffffff7;
      return;
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
