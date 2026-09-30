// test_input_device_defaults_find  (Ghidra: already named)
// address 0x490090, size 124 bytes
// name confidence: 0.9   rewrite confidence: 0.7
// evidence: out/phase4/input_functions.md summary "Debug/test routine that looks up a device's
// default control profile tag by device id and prints whether one was found."; cea-pdb naming
// hint via its two format strings. Confirmed unreachable in this build (0 callers per
// out/phase4/input_batch/490090.md). objdump of 0x490090..0x4900f3 shows `mov esi,ecx` right
// after the prologue, so the ANSI device-id string argument arrives in ECX, and that same ESI is
// reused as the extra printf argument for "deviceid %s has no default" (Ghidra's pseudo-C drops
// that vararg entirely, since it never shows register-carried varargs).
// register convention: device id ANSI string in ECX (mov esi,ecx at 0x49009f)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"


    // blam-cc: ansi in ESI
extern uint32_t input_device_default_profile_tag_find(input_guid device_guid, void *out_profile);
    // this module, 0x490110
extern void console_printf_verbose(ColorARGB *color, char *format, ...); // interface module, 0x496a80

// blam-cc: device id ANSI string in ECX
// Debug harness for input_device_default_profile_tag_find: parses device_id_ansi as a GUID
// string and reports (via the verbose console) whether a matching InputDeviceDefaults tag was
// found. Dead code in this build: nothing calls it.
void test_input_device_defaults_find(char *device_id_ansi)
{
    input_guid guid;
    uint8_t saved_profile[k_saved_player_profile_size];
    int32_t tag_id;

    input_guid_parse_ansi(&guid, device_id_ansi);
    tag_id = (int32_t)input_device_default_profile_tag_find(guid, saved_profile);
    if (tag_id == -1) {
        console_printf_verbose((ColorARGB *)0, "deviceid %s has no default", device_id_ansi);
        return;
    }
    console_printf_verbose((ColorARGB *)0, "Default profile in tag %d", tag_id);
}

#if 0
Original Ghidra decompilation (0x490090):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void test_input_device_defaults_find(void)

{
  int iVar1;
  undefined4 local_200c;
  undefined4 local_2008;
  undefined4 local_2004;
  undefined4 local_2000;
  undefined1 local_1ffc [8184];
  undefined4 uStack_4;

  uStack_4 = 0x49009a;
  input_guid_parse_ansi(&local_200c);
  iVar1 = input_device_default_profile_tag_find
                    (local_200c,local_2008,local_2004,local_2000,local_1ffc);
  if (iVar1 == -1) {
    FUN_00496a80("deviceid %s has no default");
    return;
  }
  FUN_00496a80("Default profile in tag %d",iVar1);
  return;
}

Disassembly (objdump -d, 0x490090..0x49010b), confirming the ECX device-id argument and the
esi-carried %s vararg that Ghidra's pseudo-C omits:

00490090 <test_input_device_defaults_find>:
  490090: mov    $0x200c,%eax
  490095: call   0x628240              ; __chkstk
  49009a: push   %esi
  49009b: lea    0x4(%esp),%eax        ; &local guid buffer
  49009f: mov    %ecx,%esi             ; esi = device_id_ansi (ECX argument)
  4900a1: push   %eax
  4900a2: call   0x491670              ; input_guid_parse_ansi(eax, esi)
  ...
  4900cf: call   0x490110              ; input_device_default_profile_tag_find
  4900d7: cmp    $0xffffffff,%eax
  4900da: jne    0x4900f4
  4900dc: push   %esi                  ; device_id_ansi, the %s vararg
  4900dd: push   $0x66945c             ; "deviceid %s has no default"
  4900e2: xor    %eax,%eax             ; color = NULL
  4900e4: call   0x496a80              ; console_printf_verbose
  ...
  4900f4: push   %eax                  ; iVar1, the %d vararg
  4900f5: push   $0x669440             ; "Default profile in tag %d"
  4900fa: xor    %eax,%eax
  4900fc: call   0x496a80
#endif
