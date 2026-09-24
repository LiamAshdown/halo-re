// network_game_state_update_receive  (Ghidra: FUN_004d9d20; renamed, no prior name)
// address 0x4d9d20, size 261 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Processes an incoming sequenced
// game-state update packet, growing the per-connection reassembly buffer as needed and handing
// the payload off for application"). client+0xecc/+0xed0 match types/networking.h's
// network_client_globals::unknown_ecc/unknown_ed0 exactly.
// register convention: __cdecl-shaped stack parameters (client, the incoming record) per
// Ghidra's own recovery. // blam-cc: stack -> client, record
// UNSURE: client+0xcb4 lands at session-relative offset 0x1a0, exactly
// network_game_session::player_count. Used here as a reassembly-buffer growth target, which is
// surprising for a field named "player count"; accessed via `client->session.player_count`
// as declared rather than invented as a separate field, per the task's rule against redefining
// header fields.
// UNSURE: `record`'s own layout (a dword sequence number at +0x00, two more dwords at +0x04/+0x08,
// a 16-bit capacity at +0x0e, and an 8-dword-stride payload array from +0x10) has no declared
// type anywhere; accessed via raw offsets on a `uint32_t *`/`uint8_t *` view.
// UNSURE: `DAT_006f1d6c` sits 0x4c bytes into types/networking.h's documented "network game
// engine callback block" (0x006f1d20), but the header does not resolve interior offsets of that
// block; declared here as its own opaque global at its literal address instead.
// UNSURE: the growth loop's `(target - current) & 0x7ffffff` mask and the two back-to-back
// zero-fill loops (the second of which never executes, since its own counter is initialized to
// 0) are preserved exactly, including the dead second loop.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern uint32_t network_engine_callback_block_field_0c; // 0x006f1d6c + 0xc, UNSURE name/owner
extern random_seed random_seed_global; // 0x00719cd0
extern int32_t QueryPerformanceCounter(large_integer *counter);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern void network_disconnect_notify_dropped_machines(network_client_globals *client); // 0x4d9340
extern void update_client_advance_read_cursor(void *payload); // 0x4734b0, UNSURE argument; not in this batch

// blam-cc: stack -> client, record
int32_t network_game_state_update_receive(network_client_globals *client, uint8_t *record)
{
    int16_t current_capacity;
    int16_t target_capacity;
    uint32_t *fill;
    int32_t i;
    uint32_t local_buffer[194];
    uint32_t *src, *dst;
    int16_t copy_units;
    large_integer counter;
    int32_t now_ms;

    current_capacity = *(int16_t *)(record + 0xe);
    target_capacity = client->session.player_count; // UNSURE: see file header

    if (current_capacity < target_capacity) {
        fill = (uint32_t *)(record + 0x10) + (uint32_t)current_capacity * 8;
        for (i = ((int32_t)target_capacity - (int32_t)current_capacity & 0x7ffffff) << 3; i != 0; i = i - 1) {
            *fill = 0;
            fill = fill + 1;
        }
        for (i = 0; i != 0; i = i - 1) { // UNSURE: dead loop, preserved verbatim
            *(uint8_t *)fill = 0;
            fill = (uint32_t *)((uint8_t *)fill + 1);
        }
        *(int16_t *)(record + 0xe) = target_capacity;
    }

    if (*(uint32_t *)record <= (uint32_t)client->unknown_ecc ||
        (network_server == 0 && network_engine_callback_block_field_0c == *(uint32_t *)(record + 8) &&
         *(uint32_t *)(record + 4) != random_seed_global)) {
        network_disconnect_notify_dropped_machines(client);
    }

    copy_units = *(int16_t *)(record + 0xe);
    src = (uint32_t *)(record + 0x10);
    dst = local_buffer;
    for (i = (uint32_t)(uint16_t)copy_units << 3; i != 0; i = i - 1) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    }
    for (i = 0; i != 0; i = i - 1) { // UNSURE: dead loop, preserved verbatim
        *(uint8_t *)dst = (uint8_t)*src;
        src = (uint32_t *)((uint8_t *)src + 1);
        dst = (uint32_t *)((uint8_t *)dst + 1);
    }

    update_client_advance_read_cursor(local_buffer); // UNSURE argument
    client->unknown_ecc = *(uint32_t *)record;

    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    client->unknown_ed0 = now_ms;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4d9d20):

undefined4 FUN_004d9d20(int param_1,uint *param_2)

{
  short sVar1;
  undefined4 uVar2;
  int iVar3;
  uint *puVar4;
  uint *puVar5;
  undefined8 uVar6;
  LARGE_INTEGER local_318;
  ushort local_310;
  uint local_30c [194];

  sVar1 = *(short *)((int)param_2 + 0xe);
  if (sVar1 < *(short *)(param_1 + 0xcb4)) {
    puVar4 = param_2 + sVar1 * 8 + 4;
    for (iVar3 = ((int)*(short *)(param_1 + 0xcb4) - (int)sVar1 & 0x7ffffffU) << 3; iVar3 != 0;
        iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
      *(undefined1 *)puVar4 = 0;
      puVar4 = (uint *)((int)puVar4 + 1);
    }
    *(undefined2 *)((int)param_2 + 0xe) = *(undefined2 *)(param_1 + 0xcb4);
  }
  if ((*param_2 <= *(uint *)(param_1 + 0xecc)) ||
     (((DAT_0071c2d4 == 0 && (*(uint *)(DAT_006f1d6c + 0xc) == param_2[2])) &&
      (param_2[1] != random_seed_global)))) {
    FUN_004d9340();
  }
  local_310 = *(ushort *)((int)param_2 + 0xe);
  puVar4 = param_2 + 4;
  puVar5 = local_30c;
  for (iVar3 = (uint)local_310 << 3; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(char *)puVar5 = (char)*puVar4;
    puVar4 = (uint *)((int)puVar4 + 1);
    puVar5 = (uint *)((int)puVar5 + 1);
  }
  FUN_004734b0();
  *(uint *)(param_1 + 0xecc) = *param_2;
  QueryPerformanceCounter(&local_318);
  uVar6 = __allmul(local_318.s.LowPart,local_318.s.HighPart,1000,0);
  uVar2 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(param_1 + 0xed0) = uVar2;
  return 1;
}
#endif
