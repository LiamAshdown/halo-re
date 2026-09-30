// projectile_detonation_message_apply  (Ghidra: FUN_004bdb40; renamed per
// out/phase4/projectiles_types_notes.md "Renames this pass establishes")
// address 0x4bdb40, size 185 bytes
// VERIFIED against disassembly 0x4bdb40..0x4bdbf9 (2026-09-30)
// name confidence: 0.75   rewrite confidence: 0.85 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: `objdump -d -M intel --start-address=0x4bdb40 --stop-address=0x4bdc00 bin/halo.exe`
//   resolves every register Ghidra elided (in_EAX, in_ECX-style locals with no top-level
//   parameter, and every argumentless callee below). types/projectiles.h
//   projectile_detonation_message (object_hash @0x00, position @0x04, size 0x10) and
//   k_message_projectile_detonation = 0x30; out/phase4/projectiles_types_notes.md "the
//   receiver is 0x4bdb40, which sets role 3, repositions the object, runs projectile_detonate,
//   raises the state and deletes it." The hash -> handle resolution through
//   object_network_id_table + 0x28, the object_header.flags delete-pending test and the
//   network_index_cache_remove(&network_object_index_cache, object_index) call are byte-for-byte the // FIXED: EAX is the container ADDRESS 0x6870d8 (mov eax,imm); its dword is 0xd, not a pointer
//   same sequence as src/objects/object_delete_by_pooled_node_id.c, which is where those two
//   externs and object_try_and_get's (index in ECX, mask on stack) convention come from.
//   object_set_position_and_recalculate's (position in ESI, object_index in EDI) convention is
//   established in src/objects/object_set_position_and_recalculate.c. projectile_detonate's
//   object-index-in-EBX entry is confirmed by disassembling its own prologue at 0x4c0670
//   (`mov eax,ebx` immediately after the frame is cut); it is outside this task's address range
//   and not rewritten here. projectile_request_state (0x4bf0f0, this batch) takes the index in
//   EAX and the requested state in CX.
// register convention: the incoming decoded-message record pointer in EAX (in_EAX), same shape
//   as weapon_create_from_creation_message's incoming_record.
// blam-cc: EAX -> incoming_record

// FIXED 2026-09-28 (networking call audit): message_delta_decode_compound_field / _forced / _staged take the
// decode context first (EAX) and the destination second (ECX); the calls here had the context missing or the two
// swapped.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern network_id_table *object_network_id_table; // 0x00687130
extern void *network_object_index_cache; // 0x006870d8, see
    // src/objects/object_delete_by_pooled_node_id.c

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
    // decodes the message body into out_state.
    // blam-cc: ECX -> out_state, EAX -> incoming_record (0x4ec590 opens with
    // `mov edi,[eax]` and passes ECX straight through to 0x4ed1d0). Returns nonzero (tested
    // here as != 0, not == 1) when a message was decoded.
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern void network_index_cache_remove(void *globals, uint32_t object_index); // 0x4e9d40, opaque, out of range
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_set_position_and_recalculate(real_point3d *position, datum_index object_index); // 0x4f52c0
extern void projectile_detonate(uint32_t object_index, char first_collision,
                                real remaining_tick_fraction); // 0x4c0670,
    // object index in EBX; out of range, not rewritten this pass. Both stack arguments are the
    // literal 0 at this call site
extern void projectile_request_state(datum_index projectile_index, int16_t requested_state); // 0x4bf0f0, this batch
extern void object_delete(uint32_t object_index); // 0x4f5bd0

// Receiver of network message 0x30 (k_message_projectile_detonation), sent only by
// projectile_send_detonation for a thrown-grenade projectile detonating on the authority.
// Resolves the message's object hash back to a local handle, repositions the projectile to the
// position it detonated at on the sender, runs the normal detonation effects, requests
// _projectile_state_disappearing (so nothing else tries to detonate it a second time) and
// deletes it outright.
void projectile_detonation_message_apply(void *incoming_record)
{
    projectile_detonation_message decoded;
    datum_index projectile_index;
    object *obj;

    // The mode word sits behind two indirections; nonzero means "not for us".
    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        message_delta_decode_compound_field_staged(incoming_record);
        return;
    }
    if (message_delta_decode_compound_field(incoming_record, &decoded) == 0 || decoded.object_hash == 0) {
        return;
    }

    projectile_index = object_network_id_table->handles[decoded.object_hash];
    if (projectile_index == (datum_index)0xffffffff) {
        return;
    }

    if ((((object_header *)object_data->data)[projectile_index & 0xffff].flags & _object_header_delete_pending_bit) == 0) {
        network_index_cache_remove(&network_object_index_cache, projectile_index); // FIXED: EAX is the container ADDRESS 0x6870d8 (mov eax,imm); its dword is 0xd, not a pointer
    }

    obj = object_try_and_get(projectile_index, _object_mask_projectile);
    if (obj == 0) {
        return;
    }
    obj->network_role = 3;
    object_set_position_and_recalculate(&decoded.position, projectile_index);
    projectile_detonate(projectile_index, 0, 0);
    projectile_request_state(projectile_index, _projectile_state_disappearing);
    object_delete(projectile_index);
}

#if 0
Original Ghidra decompilation (0x4bdb40):

void FUN_004bdb40(void)

{
  uint uVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int local_14;

  if (*(int *)*in_EAX == 0) {
    cVar2 = FUN_004ec590();
    if (((cVar2 != '\0') && (local_14 != 0)) &&
       (uVar1 = *(uint *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_14 * 4), uVar1 != 0xffffffff))
    {
      if ((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + (uVar1 & 0xffff) * 0xc) & 8) == 0) {
        FUN_004e9d40();
      }
      iVar3 = object_try_and_get(0x20);
      if (iVar3 != 0) {
        *(undefined4 *)(iVar3 + 4) = 3;
        object_set_position_and_recalculate();
        item_detonate(0,0);
        item_update_max_permutation_reached();
        object_delete();
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}

Disassembly (objdump -d -M intel, 0x4bdb40..0x4bdbff), which is what the rewrite above actually
follows (Ghidra elided every register argument shown here):

004bdb40:
  mov    ecx,DWORD PTR [eax]
  mov    edx,DWORD PTR [ecx]
  sub    esp,0x18
  test   edx,edx
  push   ebx
  push   esi
  push   edi
  jne    0x4bdbed
  lea    ecx,[esp+0x10]
  call   0x4ec590
  test   al,al
  je     0x4bdbf2
  mov    eax,DWORD PTR [esp+0x10]
  test   eax,eax
  je     0x4bdbf2
  mov    edx,DWORD PTR ds:0x687130
  mov    ecx,DWORD PTR [edx+0x28]
  mov    edi,DWORD PTR [ecx+eax*4]
  cmp    edi,0xffffffff
  je     0x4bdbf2
  mov    eax,edi
  and    eax,0xffff
  lea    edx,[eax+eax*2]
  mov    eax,ds:0x8603b0
  mov    ecx,DWORD PTR [eax+0x34]
  test   BYTE PTR [ecx+edx*4+0x2],0x8
  jne    0x4bdba5
  mov    esi,edi
  mov    eax,0x6870d8
  call   0x4e9d40
  push   0x20
  mov    ecx,edi
  call   0x4f6ec0
  add    esp,0x4
  test   eax,eax
  je     0x4bdbf2
  lea    esi,[esp+0x14]
  mov    DWORD PTR [eax+0x4],0x3
  call   0x4f52c0
  push   0x0
  push   0x0
  mov    ebx,edi
  call   0x4c0670
  add    esp,0x8
  mov    ecx,0x2
  mov    eax,edi
  call   0x4bf0f0
  mov    eax,edi
  call   0x4f5bd0
  pop    edi
  pop    esi
  pop    ebx
  add    esp,0x18
  ret
  call   0x4ec670  (from jne 0x4bdbed)
#endif
