// network_prepare_challenge_packet  (Ghidra: network_prepare_challenge_packet, already named)
// address 0x4deaf0, size 85 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Encodes the queued packet group into a freshly
// allocated 0x600-byte network buffer, matching the chimera-identified 'prepare challenge
// packet' code path."
// FIXED in the review pass. Ghidra shows this as `int network_prepare_challenge_packet(void)`
// with both calls argument-less, and the first draft followed that literally, calling
// network_message_block_build(0x600) -- i.e. it passed the buffer *capacity* where the encoded
// *length* belongs and dropped the destination and flag arguments entirely. The disassembly
// recovers all of it:
//   4deaf0  sub esp,0x604                  ; uint8_t buffer[0x600] plus the capacity dword
//   4deaf7  push 1 / push eax / push ecx / push edx
//   4deb00  lea eax,[esp+0x18]             ; EAX = buffer
//   4deb04  mov ebx,0x6994f8               ; EBX = network_game_messages_group
//   4deb09  mov [esp+0x14],0x600           ; capacity = 0x600
//   4deb11  call 0x4d0ae0                  ; data_packet_group_encode_packet
//   4deb1e  mov eax,[esp]                  ; the encoded byte count written back over capacity
//   4deb21  push eax                       ; -> length
//   4deb22  mov eax,0x6b7f98               ; -> existing block to reuse
//   4deb27  lea ecx,[esp+0x8]              ; -> source = buffer
//   4deb2b  mov dl,3                       ; -> flags = 3
//   4deb2d  call 0x440350                  ; network_message_block_build, result returned in EAX
// So this function takes two register arguments of its own (EAX and EDX), forwards them to the
// encoder, and returns the built message block -- which is exactly how all nine of its callers
// use it (`movzx ecx,WORD PTR [eax]` on the result, e.g. 0x4d9488).
// register convention: message type/class in EAX, payload pointer in EDX.
// blam-cc: EAX -> message_type, EDX -> payload
// UNSURE: data_packet_group_encode_packet's full parameter list is still only known from this
// one call site; the group arrives in EBX and the four stack arguments are given below in the
// order the pushes imply.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, data_packet_group *group,
    void *payload, int32_t *capacity, int32_t message_type, int32_t flag); // 0x4d0ae0; UNSURE, see header
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length); // 0x440350, this module
extern data_packet_group network_game_messages_group; // 0x006994f8
extern uint16_t network_challenge_packet_block[]; // 0x006b7f98, the reused message block

// blam-cc: EAX -> message_type, EDX -> payload
uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload)
{
    uint8_t buffer[0x600];
    int32_t length;

    length = 0x600;
    if (data_packet_group_encode_packet(buffer, &network_game_messages_group, payload, &length,
                                        message_type, 1) != 0) {
        return network_message_block_build(network_challenge_packet_block, (uint32_t *)buffer, 3,
                                           (uint32_t)length);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4deaf0):

int __cdecl network_prepare_challenge_packet(void)

{
  char cVar1;
  int iVar2;

  cVar1 = data_packet_group_encode_packet();
  if (cVar1 != '\0') {
    iVar2 = FUN_00440350(0x600);
    return iVar2;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
