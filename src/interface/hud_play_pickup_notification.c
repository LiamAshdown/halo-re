// hud_play_pickup_notification  (Ghidra: FUN_00492990, unnamed)
// address 0x492990, size 320 bytes
// name confidence: 0.3   rewrite confidence: 0.7
// evidence: out/phase4/interface_functions.md "Plays a HUD notification (sound/animation)
// reflecting a unit's weapon/equipment state, gated on the equipment item actually existing."
// Disassembled directly (objdump bin/halo.exe 0x492990..0x492ad0) since Ghidra loses every
// register argument at every call in this function; that disassembly is what pins the register
// convention and the exact field-offset chain below.
// register convention: object_or_slot_index in EBX, item_type_code in EAX.
// // blam-cc: object_or_slot_index=EBX, item_type_code=EAX
// UNSURE / TYPES-GAP: the object_or_slot_index argument is genuinely different in kind between
// this function's two callers -- weapon_action_notify_for_weapon.c passes a real weapon object
// index (datum_index), while weapon_action_notify_for_unit.c passes unit->current_weapon_index,
// a small 0..3 inventory *slot* number, straight through as if it were an object index. Verified
// by disassembly at both call sites (0x492778, 0x4927ae); preserved as-is, not "fixed", since
// this is what the retail binary does.
// TYPES-GAP: no weapon/equipment/weapon_hud_interface tag struct exists yet in types/tags.h
// (only TagDependency references to "weapon_hud_interface" by name, e.g. tags.h:7338), so the
// chain from the item tag's +0x478 field through the referenced hud_interface tag's +0x48/+0x4c/
// +0x58/+0x78 fields down to the final +0xc block entry is written as raw byte-offset pointer
// arithmetic rather than named fields. Each hop's likely role is noted inline.
// UNSURE: sound_start_at_object_marker (0x543ce0, foreign, not in this module) is called with EAX/ECX/ESI as
// register arguments plus four stack arguments; declared here with a best-effort prototype from
// the disassembly alone, not from that function's own analysis.

// Phase-4 review: verified against objdump 0x492990..0x492acf; object_try_and_get now receives
// the object index and the sound call uses the shared sound_start_at_object_marker prototype.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "cache.h"

extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances;  // 0x0087bc14, types/cache.h, stride 0x20, tag data at +0x14
extern data_array *player_data;      // 0x0087a480, "players"
extern void *sound_creation_origin;  // 0x006966f8, types/devices.h (points at global_origin3d 0x0065c230)
extern real_vector3d *global_forward3d_pointer;  // 0x00696718, types/math.h

extern int16_t item_type_to_message_stage(int16_t item_type_code);   // 0x4927c0, this module
extern int16_t item_type_to_animation_stage(int16_t message_stage);  // 0x492880, this module
extern void *datum_get(datum_index handle, data_array *array);       // 0x4d0680, memory module
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module; blam-cc: ECX -> object_index

extern int32_t sound_start_at_object_marker(datum_index object_index, void *position, void *forward,
                            datum_index sound, int32_t marker, float gain, uint8_t flag); // 0x543ce0
    // blam-cc: ESI -> object_index, ECX -> position, EAX -> forward, stack -> sound, marker, gain, flag
    // (same prototype as first_person_weapon_update.c; pushed as sound, -1, 1.0f, flag)

// Plays a HUD pickup notification (sound/animation) for an equipment/weapon item, gated on: the
// object actually existing (object_try_and_get, type mask 4), its tag's "pickup notification"
// dependency (+0x478) being set, both stage remaps succeeding, and the referenced
// weapon_hud_interface tag's message table containing a valid entry for the remapped index.
void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code)
{
    object_header *header;
    uint8_t *object_base;
    uint32_t tag_index;
    uint8_t *item_tag_data;
    int32_t hud_tag_ref;
    uint32_t hud_tag_index;
    uint8_t *hud_tag_data;
    uint8_t *block_a_base; // *(hud_tag+0x48/0x4c): count/address pair, "message name table"?
    int32_t block_a_count;
    int16_t message_stage;
    int16_t animation_stage;
    int16_t table_entry;
    uint8_t *block_c_base; // *(hud_tag+0x78): a second table, stride 0xb4
    int16_t sub_entry;
    uint8_t *block_d_base; // *(hud_tag+0x58): a third table, stride 0x14
    int32_t message_index;
    datum_index carried_object;
    uint8_t has_carried_object;
    void *carried_record;

    if (object_or_slot_index == 0xffffffff || item_type_code == -1) {
        return;
    }
    if (object_try_and_get((datum_index)object_or_slot_index, 4) == (void *)0) {
        return;
    }

    header = &((object_header *)object_data->data)[object_or_slot_index & 0xffff];
    object_base = (uint8_t *)header->data;

    tag_index = *(uint32_t *)object_base & 0xffff;
    item_tag_data = *(uint8_t **)((uint8_t *)tag_instances + tag_index * 0x20 + 0x14);

    hud_tag_ref = *(int32_t *)(item_tag_data + 0x478);
    if (hud_tag_ref == -1) {
        return;
    }

    message_stage = item_type_to_message_stage(item_type_code);
    if (message_stage == -1) {
        return;
    }
    animation_stage = item_type_to_animation_stage(message_stage);
    if (animation_stage == -1) {
        return;
    }

    hud_tag_index = (uint32_t)hud_tag_ref & 0xffff;
    hud_tag_data = *(uint8_t **)((uint8_t *)tag_instances + hud_tag_index * 0x20 + 0x14);

    block_a_count = *(int32_t *)(hud_tag_data + 0x48);
    block_a_base = (block_a_count != 0) ? *(uint8_t **)(hud_tag_data + 0x4c) : (uint8_t *)0;

    if (animation_stage < 0) {
        return;
    }
    if (animation_stage >= *(int32_t *)(block_a_base + 0x10)) {
        return;
    }
    table_entry = *(int16_t *)(*(uint8_t **)(block_a_base + 0x14) + animation_stage * 2);
    if (table_entry == -1) {
        return;
    }

    block_c_base = *(uint8_t **)(hud_tag_data + 0x78);
    sub_entry = *(int16_t *)(block_c_base + (int32_t)table_entry * 0xb4 + 0x3c);
    if (sub_entry == -1) {
        return;
    }

    block_d_base = *(uint8_t **)(hud_tag_data + 0x58);
    message_index = *(int32_t *)(block_d_base + (int32_t)sub_entry * 0x14 + 0xc);
    if (message_index == -1) {
        return;
    }

    carried_object = *(datum_index *)(object_base + 0xc0);
    has_carried_object = 0;
    if (carried_object != (datum_index)0xffffffff) {
        carried_record = datum_get(carried_object, player_data);
        if (carried_record != 0 && *(int16_t *)((uint8_t *)carried_record + 2) != -1) {
            has_carried_object = 1;
        }
    }

    sound_start_at_object_marker((datum_index)object_or_slot_index, sound_creation_origin, global_forward3d_pointer,
                 (datum_index)message_index, -1, 1.0f, has_carried_object);
}

#if 0
Original Ghidra decompilation (0x492990):

void FUN_00492990(void)

{
  uint *puVar1;
  short in_AX;
  short sVar2;
  int iVar3;
  uint extraout_ECX;
  int extraout_EDX;
  int iVar4;
  uint unaff_EBX;
  undefined4 local_4;

  if (((((unaff_EBX != 0xffffffff) && (in_AX != -1)) && (iVar3 = object_try_and_get(4), iVar3 != 0))
      && ((puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc),
          *(int *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x478) != -1 &&
          (sVar2 = FUN_004927c0(), sVar2 != -1)))) && (sVar2 = FUN_00492880(), sVar2 != -1)) {
    iVar3 = *(int *)((extraout_ECX & 0xffff) * 0x20 + 0x14 + extraout_EDX);
    if (*(int *)(iVar3 + 0x48) == 0) {
      iVar4 = 0;
    }
    else {
      iVar4 = *(int *)(iVar3 + 0x4c);
    }
    if (((-1 < sVar2) && ((int)sVar2 < *(int *)(iVar4 + 0x10))) &&
       ((sVar2 = *(short *)(*(int *)(iVar4 + 0x14) + sVar2 * 2), sVar2 != -1 &&
        ((sVar2 = *(short *)(sVar2 * 0xb4 + *(int *)(iVar3 + 0x78) + 0x3c), sVar2 != -1 &&
         (iVar3 = *(int *)(*(int *)(iVar3 + 0x58) + 0xc + sVar2 * 0x14), iVar3 != -1)))))) {
      local_4 = 0;
      if ((puVar1[0x30] != 0xffffffff) &&
         ((iVar4 = datum_get(), iVar4 != 0 && (*(short *)(iVar4 + 2) != -1)))) {
        local_4 = 1;
      }
      FUN_00543ce0(iVar3,0xffffffff,0x3f800000,local_4);
    }
  }
  return;
}

Disassembly (objdump, 0x492990..0x492acf), which is what actually pins register convention and
the field-offset chain used above:

00492990:
  51                   push   %ecx
  83 fb ff             cmp    $0xffffffff,%ebx
  57                   push   %edi
  8b f8                mov    %eax,%edi              ; edi = saved item_type_code
  0f 84 30 01 00 00    je     0x492acd
  66 83 ff ff          cmp    $0xffff,%di
  0f 84 26 01 00 00    je     0x492acd
  6a 04                push   $0x4
  8b cb                mov    %ebx,%ecx
  e8 10 45 06 00       call   0x4f6ec0                ; object_try_and_get(4), ecx=object_or_slot_index
  83 c4 04             add    $0x4,%esp
  85 c0                test   %eax,%eax
  0f 84 12 01 00 00    je     0x492acd
  8b 0d b0 03 86 00    mov    0x8603b0,%ecx
  8b 51 34             mov    0x34(%ecx),%edx
  8b c3                mov    %ebx,%eax
  25 ff ff 00 00       and    $0xffff,%eax
  8d 04 40             lea    (%eax,%eax,2),%eax
  56                   push   %esi
  8b 74 82 08          mov    0x8(%edx,%eax,4),%esi   ; esi = object_header[idx].data
  8b 06                mov    (%esi),%eax             ; eax = object.tag_index
  8b 15 14 bc 87 00    mov    0x87bc14,%edx           ; edx = tag_instances
  25 ff ff 00 00       and    $0xffff,%eax
  c1 e0 05             shl    $0x5,%eax
  8b 4c 10 14          mov    0x14(%eax,%edx,1),%ecx  ; ecx = item tag data
  8b 89 78 04 00 00    mov    0x478(%ecx),%ecx        ; ecx = item tag +0x478 (hud_interface dependency's tag id)
  83 f9 ff             cmp    $0xffffffff,%ecx
  0f 84 d6 00 00 00    je     0x492acc
  8b c7                mov    %edi,%eax
  e8 c3 fd ff ff       call   0x4927c0                ; item_type_to_message_stage(item_type_code)
  66 3d ff ff          cmp    $0xffff,%ax
  0f 84 c5 00 00 00    je     0x492acc
  e8 74 fe ff ff       call   0x492880                ; item_type_to_animation_stage(ax = previous result)
  66 3d ff ff          cmp    $0xffff,%ax
  0f 84 b6 00 00 00    je     0x492acc
  81 e1 ff ff 00 00    and    $0xffff,%ecx
  c1 e1 05             shl    $0x5,%ecx
  8b 4c 11 14          mov    0x14(%ecx,%edx,1),%ecx  ; ecx = weapon_hud_interface tag data
  8b 51 48             mov    0x48(%ecx),%edx          ; edx = *(hud_tag+0x48) (block count)
  85 d2                test   %edx,%edx
  75 04                jne    0x492a2e
  33 d2                xor    %edx,%edx
  eb 03                jmp    0x492a31
  8b 51 4c             mov    0x4c(%ecx),%edx           ; edx = *(hud_tag+0x4c) (block address)
  66 85 c0             test   %ax,%ax
  0f 8c 92 00 00 00    jl     0x492acc
  8b 7a 10             mov    0x10(%edx),%edi            ; edi = block_a.count
  0f bf c0             movswl %ax,%eax
  3b c7                cmp    %edi,%eax
  0f 8d 84 00 00 00    jge    0x492acc
  8b 52 14             mov    0x14(%edx),%edx             ; edx = block_a.address
  66 8b 04 42          mov    (%edx,%eax,2),%ax
  66 3d ff ff          cmp    $0xffff,%ax
  74 77                je     0x492acc
  8b 79 78             mov    0x78(%ecx),%edi              ; edi = *(hud_tag+0x78)
  0f bf c0             movswl %ax,%eax
  69 c0 b4 00 00 00    imul   $0xb4,%eax,%eax
  03 c7                add    %edi,%eax
  66 8b 40 3c          mov    0x3c(%eax),%ax
  66 3d ff ff          cmp    $0xffff,%ax
  74 5f                je     0x492acc
  8b 49 58             mov    0x58(%ecx),%ecx              ; ecx = *(hud_tag+0x58)
  0f bf c0             movswl %ax,%eax
  8d 04 80             lea    (%eax,%eax,4),%eax
  8b 7c 81 0c          mov    0xc(%ecx,%eax,4),%edi
  83 ff ff             cmp    $0xffffffff,%edi
  74 4d                je     0x492acc
  8b 96 c0 00 00 00    mov    0xc0(%esi),%edx              ; edx = *(object_base+0xc0)
  83 fa ff             cmp    $0xffffffff,%edx
  c6 44 24 08 00       movb   $0x0,0x8(%esp)
  74 1b                je     0x492aaa
  8b 35 80 a4 87 00    mov    0x87a480,%esi                ; esi = player_data
  e8 e6 db 03 00       call   0x4d0680                      ; datum_get(edx, esi)
  85 c0                test   %eax,%eax
  74 0c                je     0x492aaa
  66 83 78 02 ff       cmpw   $0xffff,0x2(%eax)
  74 05                je     0x492aaa
  c6 44 24 08 01       movb   $0x1,0x8(%esp)
  8b 54 24 08          mov    0x8(%esp),%edx
  a1 18 67 69 00       mov    0x696718,%eax
  8b 0d f8 66 69 00    mov    0x6966f8,%ecx
  52                   push   %edx
  68 00 00 80 3f       push   $0x3f800000
  6a ff                push   $0xffffffff
  57                   push   %edi
  8b f3                mov    %ebx,%esi
  e8 17 12 0b 00       call   0x543ce0
  83 c4 10             add    $0x10,%esp
  5e/5f/59/c3           pop esi; pop edi; pop ecx; ret
#endif
