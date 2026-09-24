// weapon_apply_ammo_correction_and_resync  (Ghidra: FUN_004c4ac0; named from
// out/phase4/items_functions.md, "Applies a server ammo correction for a weapon trigger, then
// resynchronizes any in-progress reload state")
// address 0x4c4ac0, size 139 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: network message dispatch case 0x2e (k_message_weapon_reload_cancel); types/items.h
//   weapon_magazine_ammo_message, weapon_magazine_state rounds fields at 0x2b0 + i*0x0c + 6/8,
//   weapon_flags._weapon_ammo_prediction_pending_bit; tail-calls weapon_reset_triggers
//   (0x4c4b50) with the resolved item datum_index pushed on the stack (0x4c4b2e).
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
// decompilation suggests), the hash is resolved through object_pooled_node_globals + 0x28
// exactly as src/objects/object_apply_linked_impulse.c already spells it, and the stack buffer
// is a message record types/items.h already describes. Ghidra's un-assigned `local_*` shorts are
// that buffer's fields, so they are named here instead of left as zero placeholders.
// NOTE: this variant saves the resolved datum_index in ESI (`or esi,0xffffffff` at 0x4c4adb)
// because it needs it again for the weapon_reset_triggers call; the `push esi` at 0x4c4ada also
// shifts the decoded record's stack slots by 4, which is why the disassembly reads it at
// [esp+0x8]/[esp+0xa]/[esp+0xc] rather than [esp+0x4]/[esp+0x6]/[esp+0x8].
// NOTE: unlike weapon_apply_ammo_correction (0x4c3870) this one does NOT write
// magazine->state or magazine->state_ticks; weapon_reset_triggers re-derives them.
// NOTE: the magazine index is sign-extended (`movsx ecx,WORD PTR [esp+0x8]`).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern uint8_t *object_pooled_node_globals; // 0x00687130, see weapon_add_ammunition.c
extern uint8_t message_delta_decode_compound_field(void *out_record); // 0x4ec590, networking; out_record in ECX
extern int32_t message_delta_decode_compound_field_staged(void);                // 0x4ec670, networking; drops the message
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void weapon_reset_triggers(datum_index item_index); // 0x4c4b50

// Applies a host-confirmed ammo correction to one magazine, clears the prediction-pending flag,
// and then re-derives the weapon's trigger and magazine state from scratch.
void weapon_apply_ammo_correction_and_resync(void **message_record)
{
    weapon_magazine_ammo_message decoded;
    object *item_obj;
    weapon_data *wd;
    weapon_magazine_state *magazine;
    datum_index item_index;

    if (*(int32_t *)*message_record != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }
    if ((int8_t)message_delta_decode_compound_field(&decoded) == 0) {
        return;
    }

    item_index = (datum_index)0xffffffff;
    if (decoded.object_hash != 0) {
        item_index = (*(datum_index **)(object_pooled_node_globals + 0x28))[decoded.object_hash];
    }

    item_obj = object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0 || item_obj->network_role != 1) {
        return;
    }

    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    magazine = &wd->magazines[decoded.magazine_index];
    magazine->rounds_unloaded = decoded.rounds_unloaded;
    magazine->rounds_loaded = decoded.rounds_loaded;
    wd->flags = wd->flags & ~(uint32_t)_weapon_ammo_prediction_pending_bit;
    weapon_reset_triggers(item_index);
}

#if 0
Original Ghidra decompilation (0x4c4ac0):

void FUN_004c4ac0(void)

{
  int iVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  undefined4 uVar4;
  int local_c;
  short local_8;
  undefined2 local_6;
  undefined2 local_4;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return;
  }
  cVar2 = FUN_004ec590();
  if (cVar2 != '\0') {
    uVar4 = 0xffffffff;
    if (local_c != 0) {
      uVar4 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_c * 4);
    }
    iVar3 = object_try_and_get(4);
    if ((iVar3 != 0) && (*(int *)(iVar3 + 4) == 1)) {
      iVar1 = iVar3 + 0x2b0 + local_8 * 0xc;
      *(undefined2 *)(iVar1 + 6) = local_6;
      *(undefined2 *)(iVar1 + 8) = local_4;
      *(uint *)(iVar3 + 0x22c) = *(uint *)(iVar3 + 0x22c) & 0xfffffff7;
      FUN_004c4b50(uVar4);
    }
  }
  return;
}
#endif
