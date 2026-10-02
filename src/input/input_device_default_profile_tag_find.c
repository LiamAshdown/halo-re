// input_device_default_profile_tag_find  (Ghidra: already named)
// address 0x490110, size 149 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: phase-4 review, body checked against `objdump -d` of 0x490110..0x4901ab. The tag
// iterator group filter is the immediate 0x64657663 ("devc", InputDeviceDefaults); tag_instance
// (types/cache.h) puts `.data` at +0x14, matching `*(short **)(index*0x20+0x14+tag_instances)`.
// InputDeviceDefaults (types/tags.h): device_type +0x00, device_id (TagDataOffset) +0x04 so its
// .pointer field lands at +0x10, profile (TagDataOffset) +0x18 so its .pointer lands at +0x24 --
// both confirmed directly against `mov edi,0x10(%edx)` / `mov esi,0x24(%edx)`. The compare is a
// plain `repz cmpsl` of ECX=4 dwords starting at the caller's guid argument (no off-by-one:
// Ghidra's do-while unrolling of the REP CMPS made the pseudo-C look staggered, but the machine
// code compares guid[0..3] against device_id.pointer[0..3] with no skipped or extra element).
// The copy is `rep movsl` ECX=0x7ff dwords (0x1ffc bytes, k_saved_player_profile_size) from
// profile.pointer to the caller's output buffer.
// register convention: the 16-byte GUID is passed as 4 contiguous cdecl stack dwords (received
// here as one struct-by-value, byte-identical to that layout); the output buffer pointer follows
// on the stack. tag_iterator_next's own argument (the on-stack iterator) is passed in ESI, as
// throughout this codebase (see model_load_vertex_buffers.c).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14
extern datum_index tag_iterator_next(tag_iterator *iterator); // blam-cc: ESI; cache module, 0x4425d0

// Scans every InputDeviceDefaults ("devc") tag for a mouse/keyboard or joystick/gamepad entry
// whose device_id matches device_guid, and copies its saved_player_profile-sized profile block
// out to out_profile. Returns the matching tag's datum index, or 0xffffffff if none match.
uint32_t input_device_default_profile_tag_find(input_guid device_guid, void *out_profile)
{
    tag_iterator iterator;
    datum_index tag_id;
    InputDeviceDefaults *defaults;

    iterator.next_index = -1;
    iterator.group_tag = (tag_group)0x64657663; // "devc"

    tag_id = tag_iterator_next(&iterator);
    while (tag_id != (datum_index)0xffffffff) {
        defaults = (InputDeviceDefaults *)tag_instances[(uint16_t)tag_id].data;
        if (defaults->device_type == inputdevicedefaultsdevicetype_mouse_and_keyboard ||
            defaults->device_type == inputdevicedefaultsdevicetype_joysticks_gamepads_etc) {
            if (memcmp(&device_guid, (void *)defaults->device_id.pointer, sizeof(input_guid)) == 0) {
                memcpy(out_profile, (void *)defaults->profile.pointer, k_saved_player_profile_size);
                return (uint32_t)tag_id;
            }
        }
        tag_id = tag_iterator_next(&iterator);
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x490110):

uint input_device_default_profile_tag_find(void)

{
  short *psVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  undefined4 *puVar6;
  int *piVar7;
  bool bVar8;
  undefined4 *in_stack_00000014;

  uVar2 = tag_iterator_next();
  iVar4 = DAT_0087bc14;
  if (uVar2 == 0xffffffff) {
    return 0xffffffff;
  }
  do {
    psVar1 = *(short **)((uVar2 & 0xffff) * 0x20 + 0x14 + iVar4);
    if ((*psVar1 == 1) || (*psVar1 == 0)) {
      iVar3 = 4;
      bVar8 = true;
      piVar7 = *(int **)(psVar1 + 8);
      piVar5 = (int *)register0x00000010;
      do {
        piVar5 = piVar5 + 1;
        if (iVar3 == 0) break;
        iVar3 = iVar3 + -1;
        bVar8 = *piVar5 == *piVar7;
        piVar7 = piVar7 + 1;
      } while (bVar8);
      if (bVar8) {
        puVar6 = *(undefined4 **)(psVar1 + 0x12);
        for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
          *in_stack_00000014 = *puVar6;
          puVar6 = puVar6 + 1;
          in_stack_00000014 = in_stack_00000014 + 1;
        }
        return uVar2;
      }
    }
    uVar2 = tag_iterator_next();
    if (uVar2 == 0xffffffff) {
      return 0xffffffff;
    }
  } while( true );
}

Disassembly (objdump -d, 0x490110..0x4901a4), confirming the field offsets and the plain 4-dword
compare / 0x7ff-dword copy used above:

00490110 <input_device_default_profile_tag_find>:
  490110: sub    $0x14,%esp
  490113: push   %esi
  490114: push   %edi
  490115: lea    0x8(%esp),%esi
  490119: or     $0xffffffff,%edi
  49011c: movw   $0x0,0xc(%esp)
  490123: movl   $0x64657663,0x18(%esp)      ; "devc"
  49012b: call   0x4425d0                    ; tag_iterator_next(esi=&iterator)
  490130: cmp    $0xffffffff,%eax
  490133: je     0x49019d
  490136: mov    0x87bc14,%ebx               ; tag_instances
  490140: mov    %eax,%ecx
  490142: and    $0xffff,%ecx
  490148: shl    $0x5,%ecx                    ; * sizeof(tag_instance)
  49014b: mov    0x14(%ecx,%ebx,1),%edx       ; edx = tag_instances[idx].data
  49014f: mov    (%edx),%cx                   ; device_type
  490152: cmp    $0x1,%cx
  490156: je     0x49015d
  490158: test   %cx,%cx
  49015b: jne    0x49016f
  49015d: mov    0x10(%edx),%edi              ; device_id.pointer
  490160: mov    $0x4,%ecx
  490165: lea    0x28(%esp),%esi              ; caller's guid argument
  49016b: repz cmpsl %es:(%edi),%ds:(%esi)
  49016d: je     0x490187
  490187: mov    0x24(%edx),%esi              ; profile.pointer
  49018a: mov    0x38(%esp),%edi              ; caller's out_profile argument
  49018f: mov    $0x7ff,%ecx
  490195: rep movsl %ds:(%esi),%es:(%edi)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
