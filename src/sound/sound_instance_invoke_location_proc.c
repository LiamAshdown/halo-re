// sound_instance_invoke_location_proc  (Ghidra: FUN_0054bcd0, still unnamed there)
// address 0x54bcd0, size 133 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Invokes a pending predicted-resource load callback
// for a pitch-range slot once its scheduled time has arrived, clearing it on failure."; matches
// types/sound.h sound.location_proc (0x10, sound_location_proc typedef: owner, callback_data,
// location) and sound_class_definition.dialog (0x08, base 0x0069eae0 + 8 == 0x0069eae8, stride
// 0x2c). instance->definition_index (0x08) is a Sound tag id (tag_instances[.].data).
// register convention: EAX -> sound_handle.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern int32_t sound_time;          // 0x0072520c
extern tag_instance *tag_instances; // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

// blam-cc: EAX -> sound_handle
// If `sound_handle`'s start time has arrived and it has not been flagged as delayed-start, runs
// its location_proc once. On failure: non-impulse sounds and dialog-class sounds are left
// pending (returns 0, tried again next update); other classes give up and clear location_proc
// (returns 1, as if the call had never been due). Returns 1 whenever nothing was due to run.
uint32_t sound_instance_invoke_location_proc(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    uint8_t location_proc_result;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));

    if (!(instance->flags & _sound_delayed_start_bit) && instance->location_proc != 0 &&
        instance->start_time < sound_time) {
        location_proc_result = instance->location_proc(instance->owner_index, instance->callback_data,
            &instance->location);
        if (location_proc_result == 0) {
            definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;
            if (instance->play_state != 0 || sound_class_definitions[definition->sound_class].dialog != 0) {
                return 0;
            }
            instance->location_proc = 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x54bcd0):

undefined4 FUN_0054bcd0(void)

{
  char cVar1;
  uint in_EAX;
  int iVar2;

  iVar2 = (in_EAX & 0xffff) * 0xb0 + *(int *)(DAT_007252c0 + 0x34);
  if ((((*(byte *)(iVar2 + 4) & 1) == 0) && (*(code **)(iVar2 + 0x10) != (code *)0x0)) &&
     (*(int *)(iVar2 + 0x84) < DAT_0072520c)) {
    cVar1 = (**(code **)(iVar2 + 0x10))(*(undefined4 *)(iVar2 + 0xc),iVar2 + 0x54,iVar2 + 0x14);
    if (cVar1 == '\0') {
      if ((*(short *)(iVar2 + 2) != 0) ||
         ((&DAT_0069eae8)
          [*(short *)(*(int *)((*(uint *)(iVar2 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4) *
           0x2c] != '\0')) {
        return 0;
      }
      *(undefined4 *)(iVar2 + 0x10) = 0;
    }
  }
  return 1;
}

Disassembly (0x54bcd0..0x54bd55, capstone; phase-4 review):

0x54bcd0: mov ecx, dword ptr [0x7252c0]
0x54bcd6: mov edx, dword ptr [ecx + 0x34]
0x54bcd9: and eax, 0xffff
0x54bcde: imul eax, eax, 0xb0
0x54bce4: add eax, edx
0x54bce6: push esi
0x54bce7: mov esi, eax
0x54bce9: test byte ptr [esi + 4], 1
0x54bced: jne 0x54bd4d
0x54bcef: mov eax, dword ptr [esi + 0x10]
0x54bcf2: test eax, eax
0x54bcf4: je 0x54bd4d
0x54bcf6: mov edx, dword ptr [esi + 0x84]
0x54bcfc: cmp edx, dword ptr [0x72520c]
0x54bd02: jge 0x54bd4d
0x54bd04: lea ecx, [esi + 0x14]
0x54bd07: push ecx
0x54bd08: mov ecx, dword ptr [esi + 0xc]
0x54bd0b: lea edx, [esi + 0x54]
0x54bd0e: push edx
0x54bd0f: push ecx
0x54bd10: call eax
0x54bd12: add esp, 0xc
0x54bd15: test al, al
0x54bd17: jne 0x54bd4d
0x54bd19: cmp word ptr [esi + 2], 0
0x54bd1e: jne 0x54bd51
0x54bd20: mov edx, dword ptr [esi + 8]
0x54bd23: mov eax, dword ptr [0x87bc14]
0x54bd28: and edx, 0xffff
0x54bd2e: shl edx, 5
0x54bd31: mov ecx, dword ptr [edx + eax + 0x14]
0x54bd35: movsx edx, word ptr [ecx + 4]
0x54bd39: imul edx, edx, 0x2c
0x54bd3c: mov al, byte ptr [edx + 0x69eae8]
0x54bd42: test al, al
0x54bd44: jne 0x54bd51
0x54bd46: mov dword ptr [esi + 0x10], 0
0x54bd4d: mov al, 1
0x54bd4f: pop esi
0x54bd50: ret 
0x54bd51: xor al, al
0x54bd53: pop esi
0x54bd54: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
