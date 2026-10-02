// player_update_client_local_player_vehicle_update_from_network  (Ghidra:
// player_update_client_local_player_vehicle_update_from_network, already named)
// address 0x4e5490, size 396 bytes
// name confidence: 0.85   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md; out/phase4/networking_types_notes.md's "message
// delta protocol" section (unresolved field-type descriptor/callbacks, see
// player_update_client_local_player_update_from_network.c for the fuller explanation this file
// leans on); types/game.h player; disassembly (objdump -d -M intel) for the register convention,
// the real vector3d_cross_product/vector3d_normalize_with_length argument wiring, and the real
// player_update_history_play argument list (all of which Ghidra's decompile dropped or
// misattributed).
// register convention: EAX -> decode_context, same shape as
// player_update_client_local_player_update_from_network.c.
//   // blam-cc: EAX -> decode_context
// UNSURE (load-bearing): exactly as in player_update_client_local_player_update_from_network.c,
// every "decoded" scratch value this function reads (the seat lookup index, the two update-id
// bytes, the vehicle position/orientation floats, and both cross-product input vectors) is never
// written by FUN_004ec590 or anything it calls -- confirmed again here by disassembly. They are
// preserved as genuinely-uninitialized locals, not invented.
// UNSURE: the two vector3d_cross_product results and their vector3d_normalize_with_length calls
// are never read again afterward (their outputs are dead), and players_find_local_owned_unclear in this file returns
// a datum_index handle in EAX, unlike its very different-looking use in
// player_update_queue_flush_by_name.c (this batch) -- both are transcribed literally per the
// task's "no invented behaviour" rule rather than reconciled or dropped.
// UNSURE: player_update_history_play's EAX (always loaded with 1 immediately before the call)
// and ECX (candidate->unknown_ec) look like two more register-passed arguments by blam-cc
// convention, but this batch does not have the callee's own disassembly to confirm it actually
// reads them.

// REVIEW PASS 2026-09-20: FUN_004ec590 takes the decode context in EAX and the DESTINATION in
// ECX (0x4e54a0 `lea ecx,[esp+0x10]`), so the block that earlier rewrites modelled as a pile of
// uninitialized locals is a real decoded record: types/networking.h
// local_player_vehicle_update_ack, a vehicle_update_body at +0x04 behind the two id bytes. That
// also identifies the four vector calls -- they are the standard forward/up orthonormalization
// on that body (operands at [esp+0x3c] and [esp+0x48], i.e. body+0x28 and body+0x34) -- and the
// three dwords latched into player+0x0f0..0x0f8 as the body position at [esp+0x18]..[esp+0x20].

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;              // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick
extern void *object_network_id_table;      // 0x00687130, UNSURE: name and shape both guessed
extern network_client_globals *network_client; // 0x0071c2d8

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *a, real_vector3d *b);
    // blam-cc: EAX -> out, ECX -> a, stack -> b; foreign (< this batch), 0x4052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // blam-cc: ECX -> v; foreign (< this batch), 0x401990
extern datum_index players_find_local_owned_unclear(void); // foreign (< this module), 0x477280; returns a handle here
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX -> handle, ESI -> array
extern uint8_t is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id);
    // foreign (< this batch), 0x4e69b0, blam-cc: EAX -> current_update_id, ECX -> new_update_id
extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...); // this module, 0x4e5ea0
extern void player_update_history_play(uint8_t flag, uint32_t control_ec, void *update_history,
    datum_index unit, float x, float y, float z, void *control_ptr);
    // foreign (< this batch), 0x4e6ff0, blam-cc: EAX -> flag, ECX -> control_ec,
    // stack -> update_history, unit, x, y, z, control_ptr

// Handles an incoming acknowledgement of the local player's own vehicle update: baseline-decodes
// it, resolves the local player's vehicle object via a directly-decoded handle (rather than by
// scanning, as the plain position-update sibling does), checks the new id is in order, logs it,
// latches the decoded control/position fields into the player object, and replays pending
// updates on top of it.
void player_update_client_local_player_vehicle_update_from_network(int32_t *decode_context) // blam-cc: EAX -> decode_context
{
    int32_t mode;
    int32_t *record_ctx;
    local_player_vehicle_update_ack ack; // [esp+0x10], the ECX destination FUN_004ec590 fills
    real_vector3d temp;                  // [esp+0x08], the cross-product scratch below it
    datum_index vehicle_handle;
    player *candidate;

    record_ctx = (int32_t *)(uintptr_t)decode_context[0];
    mode = record_ctx[0];
    if (mode != 0) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }
    if (message_delta_decode_compound_field(decode_context, &ack) != 1) {
        return;
    }
    if (ack.vehicle.parent_or_tag != 0) {
        int32_t *table_base = *(int32_t **)((uint8_t *)object_network_id_table + 0x28);
        ack.vehicle.parent_or_tag = table_base[ack.vehicle.parent_or_tag];
    } else {
        ack.vehicle.parent_or_tag = -1;
    }

    // The same orthonormalization the three remote-player vehicle handlers perform:
    //   temp = forward x up ;  up = temp x forward ;  normalize forward and up
    vector3d_cross_product(&temp, &ack.vehicle.up, &ack.vehicle.forward);
    vector3d_cross_product(&ack.vehicle.up, &ack.vehicle.forward, &temp);
    vector3d_normalize_with_length(&ack.vehicle.forward);
    vector3d_normalize_with_length(&ack.vehicle.up);

    vehicle_handle = players_find_local_owned_unclear();
    candidate = (player *)datum_get(vehicle_handle, player_data);
    if (candidate == 0) {
        return;
    }
    if (is_local_player_update_in_order(candidate->last_update_id, ack.update_id) == 1) {
        player_update_history_log_write(1, 0, "[%d]: Received vehicle ack for update [%d].\n",
            game_time->game_time, ack.baseline_id);
        candidate->last_update_id = ack.update_id;
        candidate->baseline_update_id = ack.baseline_id;
        candidate->unknown_f0 = *(int32_t *)&ack.vehicle.position.x;
        candidate->unknown_f4 = *(int32_t *)&ack.vehicle.position.y;
        candidate->unknown_f8 = *(int32_t *)&ack.vehicle.position.z;
        player_update_history_play(1, candidate->baseline_update_id,
            network_client->update_history, candidate->unit,
            ack.vehicle.position.x, ack.vehicle.position.y, ack.vehicle.position.z, &ack);
        return;
    }
    player_update_history_log_write(1, 0,
        "[%d]: Threw away local player vehicle ack [%d] (%d), previous ack [%d] (%d).\n",
        game_time->game_time, ack.baseline_id,
        candidate->baseline_update_id, ack.update_id, candidate->last_update_id);
}

#if 0
Original Ghidra decompilation (0x4e5490), from tools/pack.py 0x4e5490:

void player_update_client_local_player_vehicle_update_from_network(void)

{
  char cVar1;
  undefined4 *in_EAX;
  undefined4 uVar2;
  int iVar3;
  undefined1 local_54 [12];
  byte local_48;
  byte local_47;
  int local_44;
  float local_40;
  float local_3c;
  float local_38;
  undefined1 local_1c [28];

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 == '\x01') {
      uVar2 = 0xffffffff;
      if (local_44 != 0) {
        uVar2 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_44 * 4);
      }
      local_44 = uVar2;
      vector3d_cross_product(local_1c);
      vector3d_cross_product(local_54);
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      FUN_00477280();
      iVar3 = datum_get();
      if (iVar3 != 0) {
        cVar1 = is_local_player_update_in_order();
        if (cVar1 == '\x01') {
          player_update_history_log_write
                    ("[%d]: Received vehicle ack for update [%d].\n",
                     *(undefined4 *)(DAT_006f1d6c + 0xc),local_47);
          *(uint *)(iVar3 + 0xe8) = (uint)local_48;
          *(uint *)(iVar3 + 0xec) = (uint)local_47;
          *(float *)(iVar3 + 0xf0) = local_40;
          *(float *)(iVar3 + 0xf4) = local_3c;
          *(float *)(iVar3 + 0xf8) = local_38;
          player_update_history_play
                    (*(int *)(DAT_0071c2d8 + 0xf48),*(uint *)(iVar3 + 0x34),local_40,local_3c,
                     local_38,(int)&local_48);
          return;
        }
        player_update_history_log_write
                  ("[%d]: Threw away local player vehicle ack [%d] (%d), previous ack [%d] (%d).\n",
                   *(undefined4 *)(DAT_006f1d6c + 0xc),local_47,*(undefined4 *)(iVar3 + 0xec),
                   local_48,*(undefined4 *)(iVar3 + 0xe8));
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) for the real vector3d_cross_product wiring
(ESP_BASE = esp right after this function `sub esp,0x54; push esi` prologue):
  4e54a0: lea ecx,[esp+0x10] / call 0x4ec590            ; scratch base = ESP_BASE+0x10
  4e54c8: lea ecx,[esp+0x3c]      ; = ESP_BASE+0x3c, cross1 stack arg (cross_operand_b)
  4e54d0: push ecx
  4e54d1: lea eax,[esp+0x8]       ; = ESP_BASE+0x4, cross1 out (cross1_out)
  4e54d5: lea ecx,[esp+0x4c]      ; = ESP_BASE+0x48, cross1 ECX arg (cross_operand_a)
  4e54d9: call 0x4052c0           ; vector3d_cross_product(out=cross1_out, a=cross_operand_a, b=cross_operand_b)
  4e54de: lea edx,[esp+0x8]       ; = ESP_BASE+0x4 = cross1_out, reused as cross2 stack arg
  4e54e2: push edx
  4e54e3: lea eax,[esp+0x50]      ; = ESP_BASE+0x48, cross2's out (cross2_out; aliases cross_operand_a's memory)
  4e54e7: lea ecx,[esp+0x44]      ; = ESP_BASE+0x3c = cross_operand_b, reused as cross2 ECX arg
  4e54eb: call 0x4052c0           ; vector3d_cross_product(out=cross2_out, a=cross_operand_b, b=cross1_out)
  4e54f3: lea ecx,[esp+0x3c] -> ESP_BASE+0x3c = cross_operand_b / call 0x401990
  4e54fe: lea ecx,[esp+0x48] -> ESP_BASE+0x48 = cross2_out / call 0x401990
  (player_update_history_play argument order, deferred-cleanup `add esp,0x18` after the call,
   confirms: update_history, unit, x, y, z, &control_ptr, with EAX=1 and ECX=candidate->unknown_ec
   loaded immediately before the call as two more register-passed values)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
