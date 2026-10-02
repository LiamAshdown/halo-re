// network_server_password_is_set  (Ghidra: FUN_004e08e0, unnamed)
// address 0x4e08e0, size 34 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Returns true if the session's wide-character
// password field at offset 0x9fc is non-empty." Matches types/networking.h's
// network_server_globals::password exactly.
// register convention: EAX = server (network_server_globals *).
// blam-cc: EAX -> server

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>


// True if `server`'s join password is not the empty string.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t network_server_password_is_set(network_server_globals *server)
{
    return wcsncmp((wchar_t *)server->password, L"", 8) != 0;
}

#if 0
Original Ghidra decompilation (0x4e08e0):

bool FUN_004e08e0(void)

{
  int in_EAX;
  int iVar1;

  iVar1 = _wcsncmp((wchar_t *)(in_EAX + 0x9fc),L"",8);
  return iVar1 != 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
