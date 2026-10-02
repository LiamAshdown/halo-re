// control_profile_finalize_slot  (Ghidra: FUN_0053b500, renamed)
// address 0x53b500, size 149 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_functions.md summary "Finalizes a newly written control-
// profile slot by validating its associated device identity." The four dwords at
// profile+0x1314..0x1320 are gamepads[slot].device_key[0..3] (the input_guid, per
// types/interface.h's note that input_device_default_profile_tag_find takes one by value);
// looked up into a fresh local profile-sized buffer, then merged into the real profile via
// control_profile_copy_gamepad_bindings_by_key (0x53b700, this session's sibling file).
// Phase 4 review: matched objdump 0x53b500..0x53b594; the tag lookup takes the 16-byte device
// guid by value (input_guid, as in src/interface).
// register convention: profile in EDI; gamepad_index is the recognized stack parameter.
// UNSURE: control_profile_copy_gamepad_bindings_by_key's exact arguments here (Ghidra shows the
// call with none of its 3 arguments); modeled as key = the slot's own device_key record,
// dest = profile, source = the just-looked-up template buffer, which is the only reading
// consistent with "validating its associated device identity" and with the template buffer
// otherwise going unused.
// reconciled: R20 controls_gamepad_record.device_key[5] -> input_guid product_guid (+0x20c, device_key[0..3]) and int32_t product_instance (+0x21c, device_key[4])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t input_device_default_profile_tag_find(input_guid guid, uint8_t *out_profile); // 0x490110, not in this module; guid by value
extern uint8_t control_profile_copy_gamepad_bindings_by_key(controls_gamepad_record *key,
    saved_player_profile *dest, saved_player_profile *source); // 0x53b700, this module

// blam-cc: profile in EDI, then the recognized stack parameter (gamepad_index)
uint8_t control_profile_finalize_slot(saved_player_profile *profile, int32_t gamepad_index)
{
    controls_gamepad_record *slot;
    saved_player_profile template_profile;
    input_guid guid;

    if (profile == 0 || gamepad_index < 0 || 4 <= gamepad_index) {
        return 0;
    }
    slot = &profile->gamepads[gamepad_index];
    if (slot->name[0] == 0) {
        return 0;
    }

    guid.words[0] = slot->product_guid.words[0];
    guid.words[1] = slot->product_guid.words[1];
    guid.words[2] = slot->product_guid.words[2];
    guid.words[3] = slot->product_guid.words[3];
    if (input_device_default_profile_tag_find(guid, (uint8_t *)&template_profile) != -1) {
        return (uint8_t)control_profile_copy_gamepad_bindings_by_key(slot, profile, &template_profile);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53b500):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_0053b500(uint param_1)

{
  uint uVar1;
  int unaff_EDI;
  undefined1 local_1ffc [8184];
  undefined4 uStack_4;

  uStack_4 = 0x53b50a;
  if ((((unaff_EDI != 0) && (-1 < (int)param_1)) && ((int)param_1 < 4)) &&
     (param_1 = param_1 * 0x220 + unaff_EDI, *(short *)(param_1 + 0x1108) != 0)) {
    param_1 = input_device_default_profile_tag_find
                        (*(undefined4 *)(param_1 + 0x1314),*(undefined4 *)(param_1 + 0x1318),
                         *(undefined4 *)(param_1 + 0x131c),*(undefined4 *)(param_1 + 0x1320),
                         local_1ffc);
    if (param_1 != 0xffffffff) {
      uVar1 = FUN_0053b700();
      return uVar1;
    }
  }
  return param_1 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
