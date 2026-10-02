// network_game_server_host_create  (Ghidra: network_game_server_host_create, already named)
// address 0x4ddd40, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Allocates and installs the network host globals
// (DAT_0071c2d4) via network_game_server_host_new and seeds its randomisation salt field from the global PRNG
// state, mirroring the salt into the network-game globals when present." network_server
// (0x0071c2d4) and network_client (0x0071c2d8) match types/networking.h.
// UNSURE: the salt field this function writes (host+0x3ac, i.e. session-relative +0x3a4) lands
// two bytes into types/networking.h's network_game_session.unknown_3a2[10] and would spill into
// unknown_3ac/pad_3ad for a 4-byte write; the header does not carve out a dedicated dword there,
// so this rewrite keeps the raw offset rather than asserting a field boundary the header doesn't
// support.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern random_seed effect_random_seed; // 0x00719cd4, types/math.h
extern network_client_globals *network_client; // 0x0071c2d8
extern network_server_globals *network_server; // 0x0071c2d4

extern network_server_globals *network_game_server_host_new(void); // 0x4dec40, this batch

int32_t network_game_server_host_create(void)
{
    network_server_globals *host;
    uint32_t salt;

    host = network_game_server_host_new();
    network_server = host;
    if (host != 0) {
        effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
        salt = effect_random_seed >> 0x10;
        *(uint32_t *)((uint8_t *)host + 0x3ac) = salt; // UNSURE: see header
        if (network_client != 0) {
            *(uint32_t *)((uint8_t *)network_client + 0xeb8) = salt; // UNSURE: see header
        }
    }
    return host != 0;
}

#if 0
Original Ghidra decompilation (0x4ddd40):

int __cdecl network_game_server_host_create(void)

{
  int iVar1;
  void *pvVar2;
  uint uVar3;
  bool bVar4;

  pvVar2 = network_game_server_host_new();
  iVar1 = DAT_0071c2d8;
  DAT_0071c2d4 = pvVar2;
  if (pvVar2 != (void *)0x0) {
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar3 = DAT_00719cd4 >> 0x10;
    bVar4 = DAT_0071c2d8 != 0;
    *(uint *)((int)pvVar2 + 0x3ac) = uVar3;
    if (bVar4) {
      *(uint *)(iVar1 + 0xeb8) = uVar3;
    }
  }
  return CONCAT31((int3)((uint)pvVar2 >> 8),pvVar2 != (void *)0x0);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
