// weapon_add_ammunition  (Ghidra: item_add_ammunition, which is also the name in
// symbols/functions.txt; kept as weapon_add_ammunition per out/phase4/items_types_notes.md
// because object_try_and_get's mask is 4 = weapon, not the item mask)
// address 0x4c25a0, size 106 bytes
// VERIFIED against disassembly 0x4c25a0..0x4c260a (2026-09-30)
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: called only from the network message dispatcher's case 0x2c
//   (k_message_weapon_ammo_pickup); types/items.h weapon_ammo_pickup_message (the 8-byte stack
//   buffer this function decodes into -- `sub esp,0x8` at 0x4c25a4 fixes the size),
//   weapon_magazine_state.rounds_unloaded (0x2b6) and magazines[] base (0x2b0) with the 0x0c
//   stride that `lea edx,[ecx+ecx*2]` + `[eax+edx*4+0x2b6]` spells as 3*i*4.
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
// NOTE: the magazine index is sign-extended (`movsx ecx,WORD PTR [esp+0x4]`), so it is a signed
// int16_t here and not an unsigned one.
// NOTE: this function does NOT clamp the result against the tag magazine
// rounds_reserved_maximum; weapon_transfer_ammunition (0x4c2610) is the path that clamps.
// NOTE: on every early-out the original leaves whatever is in EAX as the return value (0 from
// object_try_and_get, or the object pointer when network_role != 1); that is reproduced below
// rather than normalised to a single value, because one caller may be reading it.

// FIXED 2026-09-28 (networking call audit): message_delta_decode_compound_field / _forced / _staged take the
// decode context first (EAX) and the destination second (ECX); the calls here had the context missing or the two
// swapped.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_id_table *object_network_id_table; // 0x00687130
    // variable's value is the table root, and +0x28 off it is the hash -> datum_index array
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0,
    // object_index in ECX, type_mask on the stack

// Applies a received ammo-pickup network event to one of a weapon's magazines, adding the
// signed round count it carries to that magazine's reserve, and returns a pointer to the
// magazine state it touched.
int32_t weapon_add_ammunition(void **message_record)
{
    weapon_ammo_pickup_message decoded;
    object *item_obj;
    weapon_data *wd;
    datum_index item_index;
    int16_t *rounds_unloaded;

    if (*(int32_t *)*message_record != 0) {
        return message_delta_decode_compound_field_staged(message_record);
    }
    if ((int8_t)message_delta_decode_compound_field(message_record, &decoded) == 0) {
        return 0; // NOTE: the original leaves message_delta_decode_compound_field's own EAX here
    }

    item_index = (datum_index)0xffffffff;
    if (decoded.object_hash != 0) {
        item_index = object_network_id_table->handles[decoded.object_hash];
    }

    item_obj = object_try_and_get(item_index, _object_mask_weapon);
    if (item_obj == 0 || item_obj->network_role != 1) {
        return (int32_t)(long)item_obj;
    }

    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    rounds_unloaded = &wd->magazines[decoded.magazine_index].rounds_unloaded;
    *rounds_unloaded = (int16_t)(*rounds_unloaded + decoded.rounds);
    return (int32_t)(long)&wd->magazines[decoded.magazine_index];
}

#if 0
Original Ghidra decompilation (0x4c25a0):

int item_add_ammunition(void)

{
  short *psVar1;
  undefined4 *in_EAX;
  int iVar2;
  short local_4;
  short local_2;

  if (*(int *)*in_EAX == 0) {
    iVar2 = FUN_004ec590();
    if ((((char)iVar2 != '\0') && (iVar2 = object_try_and_get(4), iVar2 != 0)) &&
       (*(int *)(iVar2 + 4) == 1)) {
      psVar1 = (short *)(iVar2 + 0x2b6 + local_4 * 0xc);
      *psVar1 = *psVar1 + local_2;
      return iVar2 + 0x2b0 + local_4 * 0xc;
    }
  }
  else {
    iVar2 = FUN_004ec670();
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
