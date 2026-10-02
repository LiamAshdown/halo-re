// network_dispatch_initialize  (Ghidra: FUN_004414c0, still unnamed -> renamed)
// address 0x4414c0, size 72 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("initializes networking (winsock etc.)
// and, on first run, registers the network game-message dispatch group used to route incoming
// gameplay packets"); types/networking.h's closing note that 0x006994f8 is the
// data_packet_group 0x4414c0 registers (39 types, max decoded size 0x600), reusing
// src/memory/struct_definition_table_compute_sizes.c's already-established prototype.
// register convention: __cdecl, no arguments.
// UNSURE: DAT_00718fa4, DAT_0071973c and the byte-sliced object at DAT_00719754 are not
// documented anywhere in networking_types_notes.md; network_initialize (0x4415c0, also in
// this module) shares DAT_007196ec with this function as a gate, so it is named here as a
// plausible "networking disabled" flag, but that is not independently confirmed either.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t network_disabled_flag; // 0x007196ec, UNSURE: see note above
extern int16_t network_join_error_code; // 0x00718fa4, UNSURE
extern int32_t network_join_error_reason; // 0x0071973c, UNSURE
extern uint8_t split_screen_quit_prompt_string[4]; // 0x00719754, UNSURE: byte 2 is never
                                                  // touched by this function
extern data_packet_group network_game_messages_group; // 0x006994f8

extern int16_t network_initialize(void); // 0x4415c0, this module
extern void struct_definition_table_compute_sizes(data_packet_group *group); // 0x4d0980

void network_dispatch_initialize(void)
{
    int16_t initialize_result;

    initialize_result = network_initialize();
    if (initialize_result != 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = 5;
        }
        *(uint16_t *)&split_screen_quit_prompt_string[0] = 0xffff;
        network_join_error_reason = 0;
        split_screen_quit_prompt_string[3] = 1;
    }
    if (network_disabled_flag == 0) {
        struct_definition_table_compute_sizes(&network_game_messages_group);
    }
}

#if 0
Original Ghidra decompilation (0x4414c0):

void FUN_004414c0(void)

{
  short sVar1;

  sVar1 = network_initialize();
  if (sVar1 != 0) {
    if (DAT_00718fa4 == -1) {
      DAT_00718fa4 = 5;
    }
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719754._3_1_ = 1;
  }
  if (DAT_007196ec == 0) {
    struct_definition_table_compute_sizes(&PTR_s_network_game_messages_group_006994f8);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
