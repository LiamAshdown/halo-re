// autopatch_proxy_initialize  (Ghidra: autopatch_proxy_initialize, already named)
// address 0x5771c0, size 26 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary; installs the proxy string
// autopatch_get_proxy_settings determines into the foreign WinInet-shaped setup thunk, then
// signals readiness.
// register convention: no register-passed arguments.
// UNSURE: autopatch_get_proxy_settings (0x576f40) is out of scope for this rewrite (WinInet/
// WinHTTP glue with nothing Blam-shaped, per out/phase4/networking_types_notes.md's
// "misattributed" list), so its return type here is treated opaquely.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t autopatch_proxy_ready; // 0x007228d0

extern void *autopatch_get_proxy_settings(void); // 0x576f40, out of this rewrite's scope
extern void ghttpSetProxy(void *proxy_settings); // foreign WinInet/WinHTTP setup, UNSURE

// Detects and installs the proxy configuration used by the autopatch HTTP client, then signals
// it is ready.
// FIXED 2026-09-28: a CreateThread routine (network_initialize), __stdcall with the unused thread parameter --
// the original ends ret 4 (0x5771d7).
uint32_t __stdcall autopatch_proxy_initialize(void *parameter)
{
    (void)parameter;
    void *settings = autopatch_get_proxy_settings();
    ghttpSetProxy(settings);
    autopatch_proxy_ready = 1;
    return 0;
}

#if 0
Original Ghidra decompilation (0x5771c0):

undefined4 autopatch_proxy_initialize(void)

{
  undefined4 uVar1;

  uVar1 = autopatch_get_proxy_settings();
  thunk_FUN_00622050(uVar1);
  DAT_007228d0 = 1;
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
