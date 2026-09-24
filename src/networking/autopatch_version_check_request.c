// autopatch_version_check_request  (Ghidra: autopatch_version_check_request, already named)
// address 0x5771e0, size 96 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary; reads the installed version/dist id via
// this module's own registry_get_halo_version/registry_get_dist_id, then waits for
// autopatch_proxy_initialize to finish before sending the check.
// register convention: no register-passed arguments.
// UNSURE: FUN_0061c260's full argument list (a foreign HTTP-request-shaped helper; the constant
// 0x281b and the callback &LAB_005777d0 are transcribed as-is).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t autopatch_proxy_ready;       // 0x007228d0
extern int32_t autopatch_update_check_state; // 0x0069fe04, -1 not started, 0 no update thread failed, 1 running

extern char *registry_get_halo_version(void);  // 0x5776d0, this module
extern uint32_t registry_get_dist_id(void);    // 0x577760, this module
extern int32_t FUN_0061c260(int32_t request_type, char *version, uint32_t dist_id,
                             void *callback, int32_t a5, int32_t a6); // foreign, UNSURE
extern void LAB_005777d0(void); // UNSURE: the request-completion callback
extern void Sleep(uint32_t milliseconds);

// Thread that waits for proxy setup and then sends the current game version and distribution id
// to check for an update; clears the "check succeeded" flag if the version string is empty or
// the request could not be sent.
uint32_t autopatch_version_check_request(void)
{
    char *version = registry_get_halo_version();
    uint32_t dist_id = registry_get_dist_id();

    while (autopatch_proxy_ready != 1) {
        Sleep(0);
    }

    if (version[0] == 0 ||
        FUN_0061c260(0x281b, version, dist_id, (void *)LAB_005777d0, 1, 0) == 0) {
        autopatch_update_check_state = 0;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x5771e0):

undefined4 autopatch_version_check_request(void)

{
  char *pcVar1;
  undefined4 uVar2;
  int iVar3;

  pcVar1 = (char *)registry_get_halo_version();
  uVar2 = registry_get_dist_id();
  while (DAT_007228d0 != '\x01') {
    Sleep(0);
  }
  if ((*pcVar1 == '\0') || (iVar3 = FUN_0061c260(0x281b,pcVar1,uVar2,&LAB_005777d0,1,0), iVar3 == 0)
     ) {
    DAT_0069fe04 = 0;
  }
  return 0;
}
#endif
