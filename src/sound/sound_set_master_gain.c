// sound_set_master_gain  (Ghidra: FUN_00548590)
// address 0x548590, size 225 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Sets the global master sound gain
//   (DAT_007252ac) with pop-avoidance hysteresis around silence, flushing cached sounds via
//   FUN_0054adb0/FUN_0054c900 when it changes."; sound_master_gain (0x007252ac) and sound_enabled
//   (0x00725201) match types/sound.h; reuses sound_stop_all (0x54adb0) and
//   sound_update_active_instances (0x54c900, already named).
// register convention: plain __cdecl, gain as the recognized stack parameter (param_1).
// blam-cc: stack -> gain
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern float sound_master_gain;  // 0x007252ac
extern uint8_t sound_enabled;    // 0x00725201


// blam-cc: stack -> gain
// Sets the master gain slider. Crossing from audible to silent stops every sound and disables
// sound_enabled first; crossing from silent to audible re-enables it and snaps to exactly 1.0
// unless the requested gain exceeds it (pop-avoidance hysteresis); otherwise the gain is set
// directly. Any actual change refreshes every active playing instance's gain.
void sound_set_master_gain(float gain)
{
    if (gain == sound_master_gain) {
        return;
    }

    if (sound_master_gain > 0.0f && gain <= 0.0f) {
        sound_stop_all();
        sound_enabled = 0;
        sound_master_gain = (gain < 0.0f) ? gain : 0.0f;
        sound_update_active_instances();
        return;
    }

    if (sound_master_gain == 0.0f && gain > 0.0f) {
        sound_enabled = 1;
        sound_master_gain = (gain > 1.0f) ? gain : 1.0f;
        sound_update_active_instances();
        return;
    }

    sound_master_gain = gain;
    sound_update_active_instances();
}

#if 0
Original Ghidra decompilation (0x548590):

void FUN_00548590(float param_1)

{
  if (param_1 == DAT_007252ac) {
    return;
  }
  if ((0.0 < DAT_007252ac) && (!NAN(param_1) && param_1 < 0.0 != (param_1 == 0.0))) {
    FUN_0054adb0();
    DAT_00725201 = 0;
    if (param_1 < 0.0) {
      DAT_007252ac = param_1;
      sound_update_active_instances();
      return;
    }
    DAT_007252ac = 0.0;
    sound_update_active_instances();
    return;
  }
  if ((DAT_007252ac == 0.0) && (0.0 < param_1)) {
    DAT_00725201 = 1;
    if (1.0 < param_1) {
      DAT_007252ac = param_1;
      sound_update_active_instances();
      return;
    }
    DAT_007252ac = 1.0;
    sound_update_active_instances();
    return;
  }
  DAT_007252ac = param_1;
  sound_update_active_instances();
  return;
}

Disassembly (0x548590..0x548671, capstone; phase-4 review):

0x548590: fld dword ptr [0x7252ac]
0x548596: fld dword ptr [esp + 4]
0x54859a: fucompp 
0x54859c: fnstsw ax
0x54859e: test ah, 0x44
0x5485a1: jnp 0x548670
0x5485a7: fld dword ptr [0x7252ac]
0x5485ad: fcomp dword ptr [0x672ac0]
0x5485b3: fnstsw ax
0x5485b5: test ah, 0x41
0x5485b8: jne 0x548605
0x5485ba: fld dword ptr [esp + 4]
0x5485be: fcomp dword ptr [0x672ac0]
0x5485c4: fnstsw ax
0x5485c6: test ah, 0x41
0x5485c9: jp 0x548605
0x5485cb: call 0x54adb0
0x5485d0: fld dword ptr [esp + 4]
0x5485d4: fcomp dword ptr [0x672ac0]
0x5485da: mov byte ptr [0x725201], 0
0x5485e1: fnstsw ax
0x5485e3: test ah, 5
0x5485e6: jp 0x5485f6
0x5485e8: mov eax, dword ptr [esp + 4]
0x5485ec: mov dword ptr [0x7252ac], eax
0x5485f1: jmp 0x54c900
0x5485f6: mov dword ptr [0x7252ac], 0
0x548600: jmp 0x54c900
0x548605: fld dword ptr [0x672ac0]
0x54860b: fld dword ptr [0x7252ac]
0x548611: fucompp 
0x548613: fnstsw ax
0x548615: test ah, 0x44
0x548618: jp 0x548661
0x54861a: fld dword ptr [esp + 4]
0x54861e: fcomp dword ptr [0x672ac0]
0x548624: fnstsw ax
0x548626: test ah, 0x41
0x548629: jne 0x548661
0x54862b: fld dword ptr [esp + 4]
0x54862f: mov byte ptr [0x725201], 1
0x548636: fcomp dword ptr [0x672ac4]
0x54863c: fnstsw ax
0x54863e: test ah, 0x41
0x548641: jne 0x548652
0x548643: mov ecx, dword ptr [esp + 4]
0x548647: mov dword ptr [0x7252ac], ecx
0x54864d: jmp 0x54c900
0x548652: mov dword ptr [0x7252ac], 0x3f800000
0x54865c: jmp 0x54c900
0x548661: mov edx, dword ptr [esp + 4]
0x548665: mov dword ptr [0x7252ac], edx
0x54866b: jmp 0x54c900
0x548670: ret 
#endif
