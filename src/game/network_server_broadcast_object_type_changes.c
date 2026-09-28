// network_server_broadcast_object_type_changes  (Ghidra: FUN_0045b680; named per
// out/phase4/game_functions.md, "Server-side routine that gathers qualifying object data into
// a large buffer and sends it as a network update, likely a full-state sync.")
// address 0x45b680, size 256 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: types/objects.h object (network_role 0x004, type 0x0b4), object_iterator,
//   object_type_definition; src/objects/object_iterator_next.c,
//   object_type_override_call_0x68/_0x6c/_0x74/_0x7c.c and
//   object_datum_consume_pending_flag.c (all pre-existing, objects module); the shared
//   network message scratch buffer already established at 0x00871de0 in
//   src/items/weapon_notify_ammo_pickup.c and friends.
// register convention: __cdecl, no arguments.
//
// UNSURE: object_type_override_call_0x6c's already-written signature (objects module) takes
// only an object index, but Ghidra's own decompile of THIS call site shows three apparent
// stack arguments (`&DAT_00871de0, 0x7ff8, changed`) being pushed immediately before the call.
// object_type_override_call_0x6c (0x4f45b0) is itself a thin trampoline that tail-calls
// whatever per-object-type function pointer it finds with no arguments of its own, so those
// three values most plausibly belong to that deeper, per-type callback rather than to the
// trampoline -- but reproducing that pass-through in portable C without touching the
// already-written objects-module prototype is not possible, so only the index is passed here,
// matching that file's own established interface. This is flagged rather than silently
// dropped.
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "units.h"
#include "networking.h"

extern int16_t network_game_mode;   // 0x00719720
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0

extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc, objects module
extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module
extern uint8_t object_type_override_call_0x74(uint32_t object_index); // 0x4f4700, objects module
extern int object_type_override_call_0x6c(uint32_t object_index);     // 0x4f45b0, objects module
extern void object_type_override_call_0x68(uint32_t object_index);    // 0x4f4560, objects module
extern void object_type_override_call_0x7c(uint32_t object_index);    // 0x4f4760, objects module
extern uint8_t object_datum_consume_pending_flag(uint32_t object_index); // 0x4f46b0, objects module
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server

// While hosting (network_game_mode == 2) and network_server+0x04 == 1, walks every object
// that this machine controls (network_role == 0) whose type opts in
// (object_type_override_call_0x74) and has a non-default object_type_definition +0x10 field,
// consumes its "changed" flag, notifies the type's create/reset overrides, and -- if the
// type's encode override reports data to send -- broadcasts a network message for it.
void network_server_broadcast_object_type_changes(void)
{
    object_iterator iterator;
    object *obj;
    uint8_t changed;
    int encode_result;

    if (network_game_mode != 2 || network_server->unknown_004 != 1) {
        return;
    }

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        if (obj->network_role == 0 && object_type_override_call_0x74(iterator.handle) == 1 &&
            object_type_definitions[obj->type]->network_delta_message_type != -1) {
            changed = object_datum_consume_pending_flag(iterator.handle);
            if (changed != 0) {
                object_type_override_call_0x68(iterator.handle);
            }
            encode_result = object_type_override_call_0x6c(iterator.handle);
            if (0 < encode_result) {
                network_session_broadcast_to_flagged(network_server, 1, object_network_message_scratch, changed == 0, 0, 0, 3);
            }
            object_type_override_call_0x7c(iterator.handle);
        }
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x45b680), from tools/pack.py 0x45b680:

void FUN_0045b680(void)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  if ((DAT_00719720 == 2) && (*(short *)(DAT_0071c2d4 + 4) == 1)) {
    local_4 = 0x86868686;
    local_10 = 0xffffffff;
    local_c = 0;
    local_a = 0;
    local_8 = 0xffffffff;
    iVar3 = object_iterator_next(&local_10);
    while (iVar3 != 0) {
      if (((*(int *)(iVar3 + 4) == 0) && (cVar2 = object_type_override_call_0x74(), cVar2 == '\x01')
          ) && (*(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar3 + 0xb4)] + 0x10) != -1)) {
        cVar2 = object_datum_consume_pending_flag();
        if (cVar2 != '\0') {
          object_type_override_call_0x68();
        }
        bVar1 = cVar2 == '\0';
        iVar3 = object_type_override_call_0x6c(&DAT_00871de0,0x7ff8,bVar1);
        if (0 < iVar3) {
          FUN_004e1a80(1,&DAT_00871de0,!bVar1,0,0,3);
        }
        object_type_override_call_0x7c();
      }
      iVar3 = object_iterator_next(&local_10);
    }
  }
  return;
}
#endif
