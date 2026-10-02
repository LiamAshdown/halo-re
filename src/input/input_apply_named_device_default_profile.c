// input_apply_named_device_default_profile  (Ghidra: already named)
// address 0x4901b0, size 200 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: phase-4 review, body checked against `objdump -d` of 0x4901b0..0x490277. Finds the
// "full_profile_definition" (device_type 2) InputDeviceDefaults tag whose profile name matches
// device_name, creates a new saved profile slot with that name, copies the tag's controls
// (category 2, see input_profile_copy_bindings_by_device.c) into it, and saves it -- but only
// once every earlier step succeeds (tag found, slot created, profile round-tripped through
// player_profile_get, and the copy itself reports success).
// Second review (phase 4 input consistency pass): the create call is 0x539ab0, not a sibling of
// 0x53a1c0, and the binary sets flags bits 1|2 (or byte [profile+0x11c],6 at 0x49025a) before
// saving; both were missing and are fixed here.
// register convention: device_name (wide string) in EDI (unaff_EDI)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <wchar.h>

extern tag_instance *tag_instances; // 0x0087bc14
extern datum_index tag_iterator_next(tag_iterator *iterator); // blam-cc: ESI; cache module, 0x4425d0

extern uint32_t saved_game_create_default_profile(uint16_t *name); // saved_games module, 0x00539ab0;
    // blam-cc: ECX -> name (0x490226 also pushes a 0 that the callee's rewrite does not read)
extern uint8_t player_profile_get(int32_t index, saved_player_profile *out_buffer); // saved_games module,
    // 0x0053a770; blam-cc: ECX -> out_buffer
extern uint8_t input_profile_copy_bindings_by_device(int32_t category, saved_player_profile *dst,
    saved_player_profile *src); // this module, 0x490280
extern void player_profile_save_539bf0(int32_t handle, saved_player_profile *profile); // saved_games
    // module, 0x539bf0

// blam-cc: device_name in EDI
// Looks up the named full-profile-definition InputDeviceDefaults tag ("devc") and, if found,
// creates and saves a new player profile from it.
void input_apply_named_device_default_profile(uint16_t *device_name)
{
    tag_iterator iterator;
    datum_index tag_id;
    InputDeviceDefaults *defaults;
    uint16_t *tag_profile_name;
    uint32_t profile_handle;
    saved_player_profile profile;

    iterator.next_index = -1;
    iterator.group_tag = (tag_group)0x64657663; // "devc"

    tag_id = tag_iterator_next(&iterator);
    if (tag_id == (datum_index)0xffffffff) {
        return;
    }

    for (;;) {
        defaults = (InputDeviceDefaults *)tag_instances[(uint16_t)tag_id].data;
        tag_profile_name = (uint16_t *)(defaults->profile.pointer + 2); // saved_player_profile::name
        if (defaults->device_type == inputdevicedefaultsdevicetype_full_profile_definition &&
            _wcsicmp((const wchar_t *)device_name, (const wchar_t *)tag_profile_name) == 0) {
            break;
        }
        tag_id = tag_iterator_next(&iterator);
        if (tag_id == (datum_index)0xffffffff) {
            return;
        }
    }

    profile_handle = saved_game_create_default_profile(tag_profile_name);
    if (profile_handle != 0xffffffff) {
        if (player_profile_get((int32_t)profile_handle, &profile) != 0) {
            if (input_profile_copy_bindings_by_device(2, &profile,
                    (saved_player_profile *)defaults->profile.pointer) != 0) {
                // 0x49025a: or byte [profile + 0x11c], 6 -- flag bits 1 and 2 on the new profile
                profile.flags |= 0x0006;
                player_profile_save_539bf0((int32_t)profile_handle, &profile);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4901b0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void input_apply_named_device_default_profile(void)

{
  short *psVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  undefined4 extraout_EDX;
  wchar_t *unaff_EDI;

  uVar3 = tag_iterator_next();
  if (uVar3 != 0xffffffff) {
    while ((psVar1 = *(short **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), *psVar1 != 2 ||
           (iVar4 = __wcsicmp(unaff_EDI,(wchar_t *)(*(int *)(psVar1 + 0x12) + 2)), iVar4 != 0))) {
      uVar3 = tag_iterator_next();
      if (uVar3 == 0xffffffff) {
        return;
      }
    }
    iVar4 = FUN_00539ab0(0);
    if ((iVar4 != -1) &&
       ((cVar2 = player_profile_get(iVar4), cVar2 != '\0' && (cVar2 = FUN_00490280(), cVar2 != '\0')
        ))) {
      player_profile_save_539bf0(extraout_EDX);
    }
  }
  return;
}

Disassembly (objdump -d, 0x4901b0..0x490277), confirming the ECX/EAX/EDX arguments to
saved_game_create_default_profile / input_profile_copy_bindings_by_device that Ghidra's
pseudo-C loses:

004901b0 <input_apply_named_device_default_profile>:
  ...
  4901ee: mov    0x14(%eax,%ecx,1),%eax   ; defaults = tag_instances[idx].data
  4901f2: cmpw   $0x2,(%eax)              ; device_type == full_profile_definition
  4901f8: mov    0x24(%eax),%ebx          ; ebx = defaults->profile.pointer
  4901fb: lea    0x2(%ebx),%edx           ; edx = tag_profile_name
  490200: call   0x6277ed                 ; _wcsicmp(edi, edx)
  ...
  490223: lea    0x2(%ebx),%ecx           ; ecx = tag_profile_name
  490228: call   0x539ab0                 ; saved_game_create_default_profile(ecx)
  49022d: mov    %eax,%esi                ; esi = profile_handle
  490237: push   %esi                     ; slot argument
  490238: lea    0x20(%esp),%ecx          ; ecx = &profile (out)
  49023c: call   0x53a770                 ; player_profile_get(esi, ecx)
  490248: lea    0x1c(%esp),%edx          ; edx = &profile (dst)
  49024c: mov    $0x2,%eax                ; eax = category 2
  490251: call   0x490280                 ; input_profile_copy_bindings_by_device(eax, edx, ebx)
  490262: mov    %edx,%eax
  490264: push   %eax                     ; profile pointer
  490265: mov    %esi,%eax                ; handle
  490267: call   0x539bf0                 ; player_profile_save_539bf0(eax, [pushed])
#endif
