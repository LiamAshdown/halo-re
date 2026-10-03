#pragma once
// Blam ai module (halo.exe 1.0.10 retail, 0x401090..0x43ecf0, 519 functions).
// The actor layer: one actor record per AI-controlled unit, the runtime encounter /
// squad / platoon bookkeeping built from the scenario encounter blocks, the prop records
// that hold an actor perception of one object, the swarm aggregation, the A*-style
// navigation-mesh search and the separate obstacle-graph point search, and the AI
// conversation and communication state.
//
// Offsets in comments are byte offsets from the struct base. Where the binary carries the
// layout it is preferred over the decompiler and said so:
//   - The five data arrays are created by name and capacity, which fixes their element
//     strides through the (index & 0xffff) * stride arithmetic every accessor uses:
//       actors_initialize @0x426710      "actor" 0x100            stride 0x724
//                                        "swarm" 0x20             stride 0x98
//                                        "swarm component" 0x100  stride 0x40
//       ai_initialize_for_new_map        "prop" 0x300             stride 0x138
//         @0x42a7c0                      (the name is the literal at 0x0065f008)
//       encounters_initialize @0x435c00  "encounter" 0x80         stride 0x6c
//                                        "ai pursuit" 0x100       stride 0x28
//       ai_communication_initialize      "ai conversation" 8      stride 0x64
//         @0x42cf20                      (the data_array header is built inline)
//   - The two flat tables encounters_initialize reserves out of the game state fix the two
//     sub-record strides: 0x8000 bytes at 0x008802cc over 1024 encounter_squad_state
//     records (0x20 each) and 0x1000 bytes at 0x008802c4 over 256 encounter_platoon_state
//     records (0x10 each). encounter_new @0x437060 hands out consecutive runs of both, one
//     run per ScenarioEncounter, sized by ScenarioEncounter.squads.count and
//     ScenarioEncounter.platoons.count.
//   - ai_initialize_for_new_map allocates 0x8dc bytes for ai_globals and crc32s that size,
//     so 0x8dc is the structure size and not a guess.
//   - path_find_context_init @0x43a700 zeroes 0x4023 dwords, which is the 0x1008c size of
//     path_find_context; the three arrays inside it are pinned by the literal offsets
//     0x84, 0xd084 and 0xe08a that the node, heap and hash accessors use, and those
//     offsets divide exactly into 1024 nodes of 0x34 and 1025 heap slots of 4.
//   - actor_new @0x426760 is the actor constructor and is the primary evidence for the
//     actor layout; actor_set_mode @0x40d8d0, actor_snapshot_orientation @0x4294d0 and
//     the movement action setters pin the rest.
//   - The actor tag itself supplies three actor fields: Actor+0x14 type -> actor.type,
//     Actor.flags bit 26 "swarm" -> actor.swarm, bit 21 "flying" -> actor.flying, and
//     Actor+0x90 glass_ignorance_chance is rolled once into actor.ignores_glass.
//
// This header uses types declared in types/math.h and types/memory.h, so those two must
// be parsed before it.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   datum_index, data_array, data_iterator
//   types/math.h     real_point2d, real_point3d, real_vector2d, real_vector3d,
//                    real_matrix4x3
//   types/objects.h  object (0x1f4), object_header, object_type_definition
//   types/units.h    unit_data (the 0x1f4 extension; unit+0x1f4 is the controlling actor
//                    handle, unit+0x1f8 / +0x1fc / +0x200 the actor cluster links)
//   types/cache.h    tag_instance (0x0087bc14, tag data at +0x14)
//   types/tags.h     Actor (the actor tag), ActorVariant, ActorType, Scenario
//                    (encounters at 0x42c, command_lists at 0x438, ai_conversations at
//                    0x468), ScenarioEncounter (0xb0; squads 0x80, platoons 0x8c,
//                    firing_positions 0x98), ScenarioSquad (0xe8), ScenarioPlatoon (0xac),
//                    ScenarioFiringPosition (0x18), ScenarioAIConversation (0x74),
//                    ScenarioAIConversationParticipant (0x54), ScenarioCommandList (0x60)
//
// Functions in this address range that do NOT belong to the actor system, and whose types
// are therefore not defined here, are listed in out/phase4/ai_types_notes.md.

#include <stddef.h> // offsetof
#include "objects.h" // bsp_leaf_reference (actor.location)
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum ai_constants {
    k_actor_data_maximum_count = 0x100,        // actors_initialize
    k_actor_size = 0x724,
    k_swarm_data_maximum_count = 0x20,
    k_swarm_size = 0x98,
    k_swarm_maximum_components = 16,           // (0x98 - 0x58) / 4, and (0x58 - 0x18) / 4
    k_swarm_component_data_maximum_count = 0x100,
    k_swarm_component_size = 0x40,
    k_prop_data_maximum_count = 0x300,         // ai_initialize_for_new_map
    k_prop_size = 0x138,
    k_encounter_data_maximum_count = 0x80,     // encounters_initialize
    k_encounter_size = 0x6c,
    k_encounter_squad_state_size = 0x20,
    k_encounter_squad_state_count = 0x400,     // 0x8000 bytes at 0x008802cc
    k_encounter_platoon_state_size = 0x10,
    k_encounter_platoon_state_count = 0x100,   // 0x1000 bytes at 0x008802c4
    k_ai_pursuit_data_maximum_count = 0x100,
    k_ai_pursuit_size = 0x28,
    k_ai_pursuit_object_count = 6,             // 0x436b10 wraps the cursor modulo 6
    k_ai_conversation_data_maximum_count = 8,  // the inline data_array header
    k_ai_conversation_size = 0x64,
    k_ai_globals_size = 0x8dc,                 // the game-state allocation size
    k_ai_conversation_event_count = 16,        // squad_despawn masks the cursor with 0xf
    k_actor_mode_count = 16,                   // the 0x38-stride table at 0x00655254
    k_actor_mode_definition_size = 0x38,
    k_actor_recognition_count = 4,             // 0x4141a0 wraps the cursor modulo 4
    k_actor_mode_data_size = 0x84,             // the stack block 0x40e260 hands to actor_set_mode
    k_path_find_maximum_nodes = 0x400,         // (0xd084 - 0x84) / 0x34
    k_path_find_maximum_heap = 0x400,          // path_find_heap_push bound
    k_path_find_hash_buckets = 0x200,          // (vertex_id & 0x1ff)
    k_path_find_hash_bucket_size = 8,          // the bucket is probed 8 entries wide
    k_path_find_maximum_waypoints = 0x40,      // 0x43a4d0 drops nodes at or past this
    k_ai_search_maximum_nodes = 0x80,          // 0x43b5a0 bound
    k_ai_search_maximum_obstacles = 0x80,      // 0x43c4b0 bound
    k_actor_movement_maximum_obstacles = 0x400 // actor_movement_collect_obstacle_candidates
} ai_constants;

// The mode index in actor.mode selects a row of the 0x38-stride definition table at 0x00655254; the mode names are halo::ai::actor_mode
// in halo/ai/modes.hpp.

// actor_order_code_is_grenade_throw @0x404340 is the only place the module states an
// order-code range outright.
typedef enum actor_order_code {
    _actor_order_code_grenade_first = 9,   // codes 9..12 inclusive are grenade throws
    _actor_order_code_grenade_last = 12
} actor_order_code;

typedef enum actor_flags {
    _actor_flag_unknown_bit1 = 0x00000002,   // 0x42a5b0 sets it and nothing else reads it here
    _actor_flag_unknown_bit10 = 0x00000400,  // 0x4347b0 sets or clears it for a whole squad
    _actor_flag_override_target = 0x00000800 // 0x42a5e0 pairs it with actor.override_target
} actor_flags;

// ---------------------------------------------------------------------------
// the mode and per-actor-type dispatch tables (read-only, in .data)
// ---------------------------------------------------------------------------
// The procedure fields are plain addresses rather than function pointers so the CParser
// does not have to resolve a signature it cannot see.
typedef struct actor_mode_definition {
    uint32_t data_size;               // 0x00 how many bytes actor_set_mode copies into actor.mode_data
    int16_t combat_grade;             // 0x04 nonzero raises actor.awareness_level to 3, zero clamps it to 2
    uint8_t unknown_06[2];            // 0x06
    uint32_t enter_proc;              // 0x08 actor_set_mode calls it after switching in
    uint32_t process_proc;            // 0x0c the mode's per-tick decision (fight: 0x403180)
    uint32_t tick_proc;               // 0x10 actor_update_activation_state calls it after the transition loop
    uint32_t update_proc;             // 0x14 actor_invoke_type_handler calls it
    uint32_t exit_proc;               // 0x18 actor_set_mode calls the outgoing mode proc first
    uint8_t unknown_1c[4];            // 0x1c
    uint32_t replace_reference_proc;  // 0x20 actor_replace_object_reference calls it with (actor, old, new)
    uint32_t carry_over_proc;         // 0x24 called with the actor when a squad is carried over to another encounter or bsp
    uint32_t clear_target_proc;       // 0x28 actor_clear_target_state calls it with the actor
    uint8_t unknown_2c[12];           // 0x2c
} actor_mode_definition; // size 0x38
// global 0x00655254: actor_mode_definition actor_mode_definitions[16]
//   actor_set_mode reads the exit proc at +0x18 of the outgoing row and the enter proc at
//   +0x08 of the incoming row; actor_invoke_type_handler reads +0x14; 0x40e760 returns the
//   int16 at +0x04. The row count is not stated in the module, so 16 is the largest mode
//   index the code tests (0xc) rounded to the next power of two. UNRESOLVED.
// global 0x006853b8: void *actor_type_procs[16]
//   one vtable pointer per ActorType (actor.type indexes it). actor_dispatch_type_vtable
//   @0x426670 / 0x4266a0 / 0x4266d0 call slots +0x10, +0x18 and +0x1c, and 0x435420 reads
//   the byte at +0x0d and compares it against actor.swarm. ActorType has 16 values.

// ---------------------------------------------------------------------------
// actor
// ---------------------------------------------------------------------------
// The order record the builders at 0x401090..0x4049d0 fill in and actor_process_order_request
// consumes. 0x401090 zeroes 0x17 dwords, which fixes the size at 0x5c.
typedef struct actor_order {
    int16_t order_code;               // 0x00 the requested order; forced to 0 when actor.swarm is set
    int16_t unknown_02;               // 0x02 zeroed
    uint8_t valid;                    // 0x04 set to 1 by every builder that succeeds
    uint8_t unknown_05;               // 0x05
    int16_t target_index;             // 0x06 0xffff sentinel in the default order
    int16_t parameter;                // 0x08 the caller-supplied word
    uint8_t unknown_0a;               // 0x0a zeroed
    uint8_t unknown_0b[81];           // 0x0b
} actor_order;          // size 0x5c

// The queued / active movement action pair inside the actor. The setters at 0x417610,
// 0x417750, 0x417830 and 0x417910 write the queued copy at actor+0x400 and then copy all
// six dwords to the active copy at actor+0x46c, which is what fixes the 0x18-byte layout.
typedef struct actor_movement_action {
    int16_t type;                     // 0x00 0 stop, 2 explicit point, and the firing-position / formation / near-target kinds
    uint8_t cancelled;                // 0x02 actor_movement_action_cancel sets it
    uint8_t unknown_03;               // 0x03
    union {
        real_point3d destination;     // 0x04
        int16_t slot_index;           // 0x04 types 3 and 4: the firing position / formation slot / move position index
        uint32_t reference;           // 0x04 type 5: the prop handle
    };
    int32_t parameter;                // 0x10 object index, firing-position index or formation slot depending on type
    uint32_t extra;                   // 0x14
} actor_movement_action; // size 0x18

// The perception tally at actor+0x1ec. actor_choose_best_target @0x4203a0 is the only
// writer: it zeroes 0x1e dwords plus a word plus a byte starting at actor+0x1ec -- exactly
// 0x7b bytes -- and then, for every prop on the prop list of the actor whose kind is 2 or 3
// and which is not a vault, bumps the counters below. The three 16-wide runs are indexed by the
// ActorType of the tracked unit, or 6 when the object has a parent (object+0x218), or 14
// (actortype_none) when the object is not driven by an actor at all.
typedef struct actor_target_tally {
    uint8_t unit_props;                    // 0x00 (actor+0x1ec) bumped for every prop with prop.is_unit set
    uint8_t unit_props_unseen;             // 0x01 ... whose prop.unknown_9c is zero
    uint8_t by_threat_class[10];           // 0x02 histogram, index 0..8, of the threat class the scan derives
                                           //   from prop.unknown_122 / unknown_12f / unknown_74 / distance
    uint8_t threat_class_ge_1;             // 0x0c (0x1f8) bumped whenever the prop is not a low-priority kind
    uint8_t threat_class_2;                // 0x0d (0x1f9)
    uint8_t threat_class_3;                // 0x0e (0x1fa)
    uint8_t threat_class_4;                // 0x0f (0x1fb)
    uint8_t threat_class_5;                // 0x10 (0x1fc)
    uint8_t threat_class_6;                // 0x11 (0x1fd)
    uint8_t threat_class_7;                // 0x12 (0x1fe)
    uint8_t threat_class_8;                // 0x13 (0x1ff)
    uint8_t group_a_total;                 // 0x14 (0x200) prop within 8 world units, or further away but
                                           //   fighting the same object this actor is fighting
    uint8_t group_a_marked;                // 0x15 (0x201) ... and prop.owner_burst_length_exceeded set
    uint8_t group_a_marked_135;            // 0x16 (0x202) ... and prop.unknown_135 set as well
    uint8_t group_a_by_actor_type[16];     // 0x17 (0x203)
    uint8_t group_a_marked_by_actor_type[16];// 0x27 (0x213)
    uint8_t group_b_total;                 // 0x37 (0x223) prop.unknown_38 is 0 or 1
    uint8_t group_b_marked;                // 0x38 (0x224)
    uint8_t group_b_by_actor_type[16];     // 0x39 (0x225)
    uint8_t group_b_marked_by_actor_type[16];// 0x49 (0x235)
    uint8_t group_c_total;                 // 0x59 (0x245) group b and closer than 3 world units
    uint8_t group_c_marked;                // 0x5a (0x246)
    uint8_t group_c_by_actor_type[16];     // 0x5b (0x247)
    uint8_t group_c_marked_by_actor_type[16];// 0x6b (0x257)
} actor_target_tally;   // size 0x7b

typedef struct actor_recognition_entry {
    uint8_t type;                     // 0x00 the DL byte 0x4141a0 was handed
    uint8_t unknown_01;               // 0x01
    int16_t firing_position_index;    // 0x02 index into the encounter ScenarioFiringPosition block
} actor_recognition_entry; // size 0x4

// actor.mode_data: the current mode's state, laid out per mode (actor_set_mode copies the mode's initial data,
// actor_mode_definition.data_size bytes). raw is the byte view; the per-mode views name what each mode's
// functions (actor_mode_<mode>_*) use, relative to actor + 0x9c.
typedef struct actor_mode_wait_data {
    uint8_t finished;                   // 0x00 actor_mode_wait_process returns it once the wait is over
    uint8_t unknown_01;                 // 0x01 tested by the process and by the tick together with the unit
    uint8_t unknown_02;                 // 0x02 process gate (with unknown_04)
    uint8_t following_friend;           // 0x03 0x03 (actor+0x9f) set by actor_mode_wait_process when a nearby friend
                                        //    is further than 3.5 and worth following (then movement goes toward it);
                                        //    while clear the tick runs countdown_0c
    uint8_t unknown_04;                 // 0x04
    uint8_t unknown_05[3];              // 0x05
    int32_t start_game_time;            // 0x08 game_time when waiting began; process gives up after 2700 ticks
    int16_t countdown_0c;               // 0x0c counted down by the tick while following_friend is clear
    int16_t countdown_150;              // 0x0e counted down by the tick; process re-arms it at 150
    int16_t random_countdown;           // 0x10 counted down by the tick; re-armed at 300..599 (random)
    uint8_t unknown_12[6];              // 0x12
} actor_mode_wait_data;
typedef char actor_mode_wait_data_size[sizeof(actor_mode_wait_data) == 0x18 ? 1 : -1];

typedef struct actor_mode_flee_data {
    int16_t countdown_180;              // 0x00 counted down by the tick; process re-arms it at 180
    int16_t countdown_02;               // 0x02 counted down by the tick; enter/tick act when it reaches 0
    uint8_t use_last_seen_position;     // 0x04 passed to the firing position query by the melee target reachability check
    uint8_t cover_flag;                 // 0x05 copied to the firing position query when looking for cover
    uint8_t movement_cancelled;         // 0x06 set by actor_mode_flee_movement_cancelled and by process
    uint8_t unknown_07;                 // 0x07
    int16_t destination;                // 0x08 firing position fled to, -1 for none
    uint8_t destination_without_path;   // 0x0a copy of actor.firing_position_without_path for the destination; update copies it back
    uint8_t unknown_0b;                 // 0x0b
    int16_t panic;                      // 0x0c update's "panic"; 9..12 is the cowering band
    uint8_t engage;                     // 0x0e the actor gave up fleeing; the type updates fight on (engage) while it is set, else look away
    uint8_t finished;                   // 0x0f the panic is over or the destination was reached; with engage it makes the process report done
    uint8_t announced;                  // 0x10 the flee line was spoken; cleared again when the unit's current speech has no priority
    uint8_t unknown_11[3];              // 0x11
    int32_t announce_time;              // 0x14 game_time of the last spoken flee line or reaction animation
    int32_t ticks_in_mode;              // 0x18 incremented every tick, zeroed on enter
    datum_index reference;              // 0x1c the actor/object fled from (actor_mode_flee_replace_reference)
    uint8_t target_reachable;           // 0x20 a path to the target was found by the reachability checks
    uint8_t unknown_21[3];              // 0x21
    real_point3d target_position;       // 0x24 where the target can be reached from
} actor_mode_flee_data;
typedef char actor_mode_flee_data_reference_at_1c[offsetof(actor_mode_flee_data, reference) == 0x1c ? 1 : -1];
typedef char actor_mode_flee_data_size[sizeof(actor_mode_flee_data) == 0x30 ? 1 : -1];

typedef struct actor_mode_converse_data {
    datum_index conversation;           // 0x00
    uint8_t finished;                   // 0x04 actor_mode_converse_process returns it; set when the partner is lost or reached
    uint8_t arrived;                    // 0x05 set once within approach_distance (or 0.7) of the partner, then movement stops
    uint8_t unknown_06[2];              // 0x06
    float approach_distance;            // 0x08 process closes to within this of the partner
    datum_index partner_unit;           // 0x0c the partner, turned into partner_prop by actor_find_or_create_shared_prop
    datum_index partner_prop;           // 0x10 the partner's prop (actor_mode_converse_replace_reference swaps it)
} actor_mode_converse_data;
typedef char actor_mode_converse_data_size[sizeof(actor_mode_converse_data) == 0x14 ? 1 : -1];

typedef struct actor_mode_uncover_data {
    uint8_t crouch;                     // 0x00 copied to the crouch control flags by update; tick recomputes it
    uint8_t done;                       // 0x01 set by the tick once the uncovering is over; request_path_with_grenade_arc stops pathing
    uint8_t unknown_02;                 // 0x02 set by the tick while the actor is not making progress
    uint8_t unknown_03;                 // 0x03 enter tests it
    uint8_t use_last_seen_position;     // 0x04 request_path_with_grenade_arc passes it as use_last_seen_position, then sets it
    uint8_t unknown_05[3];              // 0x05
    int16_t stage;                      // 0x08 update branches on 0 / 1
    int16_t firing_position;            // 0x0a firing position reached or being moved to, -1 for none (movement_cancelled clears it in stage 1)
    int16_t target_cluster;             // 0x0c passed to the grenade path query as explicit_target_cluster_index
    uint8_t unknown_0e[2];              // 0x0e
    uint32_t target_object;             // 0x10 passed to the grenade path query as explicit_target_object
    real_point3d position;              // 0x14 the position to uncover; update copies it to the destination
    uint8_t target_reached;             // 0x20 set by the path request; the tick ends stage 1 with it
    uint8_t unknown_21[3];              // 0x21
    int32_t stage_ticks;                // 0x24 counted up, reset to 0; stage 0 acts at 30
    int32_t duration_ticks;             // 0x28 set on enter
    int32_t remaining_ticks;            // 0x2c copied from duration_ticks and counted down by the tick
    int32_t total_ticks;                // 0x30 counted up whenever remaining_ticks is not refreshed; the tick gives up at 360
} actor_mode_uncover_data;
typedef char actor_mode_uncover_data_remaining_at_2c[offsetof(actor_mode_uncover_data, remaining_ticks) == 0x2c ? 1 : -1];
typedef char actor_mode_uncover_data_size[sizeof(actor_mode_uncover_data) == 0x34 ? 1 : -1];

typedef struct actor_mode_search_data {
    uint8_t finished;                   // 0x00 the process returns it; the tick sets it when the search is over
    uint8_t unknown_01;                 // 0x01 set by the process and the tick
    uint8_t reachable;                  // 0x02 actor_evaluate_engagement_reachability's result (process)
    uint8_t unknown_03;                 // 0x03 set by the tick and the update
    uint8_t unknown_04;                 // 0x04 process gate for the shared search position
    uint8_t unknown_05;                 // 0x05 process gate together with reachable
    uint8_t unknown_06[2];              // 0x06
    int16_t stage;                      // 0x08
    int16_t firing_position;            // 0x0a the firing position searched from, -1 for none
    int16_t target_cluster;             // 0x0c passed as actor_evaluate_engagement_reachability's target_cluster
    uint8_t unknown_0e[2];              // 0x0e
    int32_t surface_index;              // 0x10 pathfinding surface of the firing position the search starts from
    real_point3d position;              // 0x14 the search position; update copies it to the destination
    int32_t duration_ticks;             // 0x20 set on enter
    int32_t remaining_ticks;            // 0x24 copied from duration_ticks and counted down by the tick
    int32_t elapsed_ticks;              // 0x28 counted up by the tick; acts past 120
} actor_mode_search_data;
typedef char actor_mode_search_data_elapsed_at_28[offsetof(actor_mode_search_data, elapsed_ticks) == 0x28 ? 1 : -1];
typedef char actor_mode_search_data_size[sizeof(actor_mode_search_data) == 0x2c ? 1 : -1];

typedef struct actor_mode_charge_data {
    int32_t charge_start_time;          // 0x00 game_time the charge began; process gives a melee charge up once Actor.melee_charge_time seconds passed
    int16_t stage;                      // 0x04 1..4
    uint8_t strike_started;              // 0x06 the unit started its melee strike; the tick (stage 3) stops counting stage_ticks while it is set
    uint8_t strike_finished;            // 0x07 process sets it once the unit left its busy (melee) animation after the strike
    uint8_t done;                       // 0x08 process sets it when the charge is over; it is part of the process result
    uint8_t turning_to_face;            // 0x09 update (stages 2 and 3) waits for it while the actor stands still
    uint8_t leap_allowed;               // 0x0a the target is flying or engaged, or the leap range was entered
    uint8_t jump_started;               // 0x0b update sets it when it issues the jump; the tick counts stage_ticks while it is set (stage 3)
    uint8_t jump_solved;                // 0x0c set once the leap is solved; update turns it into actor.jump_requested and clears it
    uint8_t unknown_0d;                 // 0x0d
    int16_t stage_ticks;                // 0x0e counted up by the tick, reset by update
    int32_t stage_start_time;           // 0x10 game_time when update last reset stage_ticks
    real_vector2d jump_direction;       // 0x14 update copies it to actor.jump_facing
    float jump_horizontal_speed;        // 0x1c copied to actor.jump_horizontal_velocity (compared with jump_vertical_speed)
    float jump_vertical_speed;          // 0x20 copied to actor.jump_vertical_velocity
    uint8_t target_weak;                // 0x24 stage 1: the target is a weak actor
    uint8_t stand;                      // 0x25 stage 1: the crouch control flags follow !stand
    int16_t weak_target_ticks;          // 0x26 stage 1: counts the passes that found a weak target
    uint8_t close_in;                   // 0x28 process wants the actor to close in on the target; update's crouch gate
    uint8_t approach_failed;            // 0x29 no destination near the target could be set; part of the process result
    uint8_t unknown_2a[2];              // 0x2a
    float wait_threshold;               // 0x2c consideration wait threshold: distance beyond which the target counts as far away
    uint8_t suicide_charge;             // 0x30 the actor strikes on proximity or closing speed (Actor.suicide_sensing_dist)
    uint8_t unknown_31;                 // 0x31
    int16_t lead_ticks;                 // 0x32 ticks the target position is led by
    float strike_range_extra;           // 0x34 added to Actor.melee_fudge_factor for the strike range
} actor_mode_charge_data;
typedef char actor_mode_charge_data_start_at_10[offsetof(actor_mode_charge_data, stage_start_time) == 0x10 ? 1 : -1];
typedef char actor_mode_charge_data_size[sizeof(actor_mode_charge_data) == 0x38 ? 1 : -1];

typedef struct actor_mode_guard_data {
    int16_t countdown_00;               // 0x00 counted down by the tick
    int16_t countdown_02;               // 0x02 counted down by the tick
    uint8_t settled;                    // 0x04 update sets it once the movement for the stage was issued; the tick needs it for the ambush timers
    uint8_t command_pending;            // 0x05 report_command_status runs when it is set; the tick clears it when countdown_00 expires
    uint8_t unknown_06;                 // 0x06
    uint8_t attack_point;               // 0x07 update fires at guard_point while it is set
    uint8_t ambush_active;              // 0x08 cleared by movement_cancelled in stage 3
    uint8_t ambush_triggered;           // 0x09 cleared by the tick when the ambush ends
    uint8_t ambush_retreat;             // 0x0a the ambush ends when retreat_timer runs out instead of countdown_0c
    uint8_t unknown_0b;                 // 0x0b
    int16_t countdown_0c;               // 0x0c counted down by the tick; movement_cancelled clears it
    uint8_t reselect;                   // 0x0e set to ask for a new guard position (movement cancelled, command expired, target cleared)
    uint8_t watch_pending;              // 0x0f update broadcasts the watch event and clears it when in place
    datum_index hold_reference;         // 0x10 swapped by actor_mode_guard_replace_reference
    uint8_t look_point_valid;           // 0x14 update looks at look_point (flee source 4) while it is set
    uint8_t look_point_hostile;         // 0x15 flee reason 5 instead of 3 for the look point
    uint8_t unknown_16[2];              // 0x16
    real_point3d look_point;            // 0x18
    int16_t stage;                      // 0x24 0..3
    uint8_t unknown_26[2];              // 0x26
    union {
        int16_t firing_position;        // 0x28 stage 3: -1 for none
        real_point3d guard_point;       // 0x28 stage 2: the point being guarded
    };
    int32_t guard_point_surface;        // 0x34 stage 2: pathfinding surface of guard_point; target_cleared and the tick reset it to -1
    float guard_radius;                 // 0x38 stage 2: distance from guard_point that counts as in place
    datum_index guard_target;           // 0x3c swapped by replace_reference; update makes it the actor's target
    uint8_t follow_movement;            // 0x40 the countdown_02 tick only runs while the movement completed when it is set
    uint8_t unknown_41[3];              // 0x41
} actor_mode_guard_data;
typedef char actor_mode_guard_data_target_at_3c[offsetof(actor_mode_guard_data, guard_target) == 0x3c ? 1 : -1];
typedef char actor_mode_guard_data_size[sizeof(actor_mode_guard_data) == 0x44 ? 1 : -1];

typedef struct actor_mode_alert_data {
    int16_t position_count;             // 0x00 move positions of the squad; actor_select_move_position chooses among them
    int16_t wait_ticks;                 // 0x02 counted down by the tick once the actor stands on a position
    uint8_t direction_flag;             // 0x04 out flag of actor_select_move_position (the walk direction through the positions)
    uint8_t unknown_05;                 // 0x05
    int16_t current_position;           // 0x06 move position being held, -1 for none
    int16_t next_position;              // 0x08 move position chosen by the process, -1 for none
    uint8_t position_reached;           // 0x0a set when the process commits to next_position; the tick plays its animation once and clears it
    uint8_t unknown_0b;                 // 0x0b
    real_point3d position;              // 0x0c destination of the held move position; the process copies the 0x50 byte ScenarioMovePosition here
    float facing;                       // 0x18 ScenarioMovePosition.facing
    float weight;                       // 0x1c ScenarioMovePosition.weight
    float time[2];                      // 0x20 ScenarioMovePosition.time: wait range in seconds
    int16_t animation_index;            // 0x28 ai animation reference the tick plays on arrival, -1 for none
    int8_t sequence_id;                 // 0x2a ScenarioMovePosition.sequence_id
    uint8_t unknown_2b;                 // 0x2b
    uint8_t unknown_2c[8];              // 0x2c
    int16_t cluster_index;              // 0x34 ScenarioMovePosition.cluster_index; target_cleared sets -1
    uint8_t unknown_36[0x22];           // 0x36 rest of the copied move position record
    int32_t surface_index;              // 0x58 ScenarioMovePosition.surface_index; target_cleared sets -1
} actor_mode_alert_data;
typedef char actor_mode_alert_data_size[sizeof(actor_mode_alert_data) == 0x5c ? 1 : -1];

typedef struct actor_mode_vehicle_data {
    datum_index vehicle_index;          // 0x00 the vehicle to board
    int16_t seat_index;                 // 0x04
    uint8_t unknown_06;                 // 0x06 selects the retry limit (5 or 50) and the alert-range test of the process
    uint8_t near_line;                  // 0x07 in/out flag of actor_avoid_obstacle_and_project
    uint8_t seated;                     // 0x08 the unit entered the seat
    uint8_t unit_replaced;              // 0x09 the actor already has an active unit; the mode is over
    uint8_t failed;                     // 0x0a boarding was abandoned; the mode is over
    uint8_t unknown_0b;                 // 0x0b
    int16_t path_failures;              // 0x0c failed path requests since the last success
    int16_t stuck_count;                // 0x0e number of 150 tick checks the actor stayed within 5 units; 8 abandons the boarding
    int32_t last_progress_time;         // 0x10 game_time of the last stuck check
    real_point3d last_progress_position;// 0x14 position at the last stuck check
    float alert_range_min;              // 0x20 passed to actor_is_within_alert_range
    float alert_range_max;              // 0x24
    uint8_t close;                      // 0x28 the actor is next to the seat entry
    uint8_t facing;                     // 0x29 the actor faces the seat entry
    int16_t in_front_ticks;             // 0x2a consecutive ticks the entry was in front; 30 counts as close and facing
    uint8_t entry_reached;              // 0x2c the entry point is within 1 unit
    uint8_t unknown_2d[3];              // 0x2d
    real_point3d path_destination;      // 0x30
    real_vector3d entry_direction;      // 0x3c copied to actor.flee_source by update
    int32_t path_surface;               // 0x48 pathfinding surface of path_destination
} actor_mode_vehicle_data;
typedef char actor_mode_vehicle_data_size[sizeof(actor_mode_vehicle_data) == 0x4c ? 1 : -1];

// One member's command list execution record (squad action): the obey mode keeps one at actor.mode_data + 0x08 for a lone
// actor, and every swarm component keeps one at swarm_component + 0x1c. actor_squad_action_execute / _is_complete / _list_process
// advance it, actor_get_body_axis_vector fills axis and direction.
typedef struct actor_squad_action_state {
    uint8_t command_index;              // 0x00 index of the command being run (0xff before the first)
    uint8_t retry_count;                // 0x01 how often the list jumped back in this pass, limited to 10
    int16_t timer_ticks;                // 0x02 ticks left before the running command completes
    uint8_t flags;                      // 0x04 bit 0 attack requested, bit 1 list finished, bit 2 wait for the vehicle line, bit 3/4 vehicle seat state
    uint8_t movement_flags;             // 0x05 bit 0 strafe, bit 1 move command running, bit 2 vehicle drive command, bit 3 jump landed, bit 4 jump parameters valid
    uint8_t unknown_06[2];              // 0x06
    int16_t axis;                       // 0x08 body axis the movement follows: 0 forward, 1 back, 2 and 3 sideways; vehicle drive commands store other kinds
    uint8_t unknown_0a[2];              // 0x0a
    union {
        real_vector3d direction;        // 0x0c movement direction of a move command
        struct {
            float horizontal_speed;     // 0x0c jump speeds of a scripted jump
            float vertical_speed;       // 0x10
        } jump;
    };
    real_point3d start_position;        // 0x18 where a move command started
} actor_squad_action_state;             // size 0x24
typedef char actor_squad_action_state_size[sizeof(actor_squad_action_state) == 0x24 ? 1 : -1];

// What the command list tells the actor to aim, look at, shoot, move to or say. The obey mode keeps one at actor.mode_data + 0x2c;
// the commands of a swarm have none.
typedef struct actor_command_aim {
    uint8_t crouch;                     // 0x00 copied to the crouch control flags
    uint8_t unknown_01;                 // 0x01
    int16_t movement_style;             // 0x02 0..3, copied to actor.movement_style_override
    uint8_t move_requested;             // 0x04 a move command set move_point
    uint8_t move_interrupts;            // 0x05 the move cancels the running movement action first
    uint8_t unknown_06[2];              // 0x06
    real_point3d move_point;            // 0x08 destination of the move command
    int32_t move_surface_index;         // 0x14 pathfinding surface of move_point
    uint8_t look_valid;                 // 0x18 update looks at look_point while it is set
    uint8_t unknown_19[3];              // 0x19
    real_point3d look_point;            // 0x1c
    uint8_t unknown_28;                 // 0x28 set by command 0x1a together with unknown_2c
    uint8_t unknown_29[3];              // 0x29
    float unknown_2c;                   // 0x2c command 0x1a parameter
    uint8_t secondary_action_pending;   // 0x30 a secondary action is queued once actor.secondary_action is free
    uint8_t unknown_31;                 // 0x31
    int16_t secondary_action;           // 0x32 -1 for none
    int16_t communication_line;         // 0x34 -1 for none; broadcast once
    uint8_t shoot_valid;                // 0x36 update fires at shoot_point while it is set
    uint8_t unknown_37;                 // 0x37
    real_point3d shoot_point;           // 0x38
    float burst_duration;               // 0x44 copied to actor.burst_duration_override
    uint8_t grenade_pending;            // 0x48 update turns it into actor.throw_grenade and clears it
    uint8_t grenade_thrown;             // 0x49
    int16_t grenade_style;              // 0x4a
    real_point3d grenade_target;        // 0x4c
} actor_command_aim;                    // size 0x58
typedef char actor_command_aim_size[sizeof(actor_command_aim) == 0x58 ? 1 : -1];

typedef struct actor_mode_obey_data {
    int16_t command_list_index;         // 0x00 the command list being run
    uint8_t allow_initiative;           // 0x02 command list flag 0: the actor may use its own initiative; passed to actor_update_combat_behavior
    uint8_t allow_look;                 // 0x03 command list flag 2 inverted: update looks and aims at the target while it is set
    uint8_t allow_communication;        // 0x04 command list flag 3 inverted
    uint8_t finished;                   // 0x05 set once every member finished the list (process)
    uint8_t unknown_06[2];              // 0x06
    actor_squad_action_state action;    // 0x08 the actor's own command execution record
    actor_command_aim aim;              // 0x2c
} actor_mode_obey_data;
typedef char actor_mode_obey_data_size[sizeof(actor_mode_obey_data) == 0x84 ? 1 : -1];

typedef struct actor_mode_fight_data {
    uint8_t unknown_00[4];              // 0x00
} actor_mode_fight_data;

typedef union actor_mode_data {
    uint8_t raw[0x84];
    actor_mode_alert_data alert;
    actor_mode_fight_data fight;
    actor_mode_wait_data wait;
    actor_mode_flee_data flee;
    actor_mode_converse_data converse;
    actor_mode_uncover_data uncover;
    actor_mode_search_data search;
    actor_mode_charge_data charge;
    actor_mode_guard_data guard;
    actor_mode_vehicle_data vehicle;
    actor_mode_obey_data obey;
} actor_mode_data;                      // size 0x84
typedef char actor_mode_data_size[sizeof(actor_mode_data) == 0x84 ? 1 : -1];

// The ad hoc {code, payload} record every caller of actor_resolve_flee_source_point @0x4146c0
// builds on its own stack. The code selects which of seven source kinds to resolve; the
// payload is either a datum handle or a point, never both.
typedef struct actor_flee_source_reason {
    int16_t code;              // 0x00 selects which of the 7 source kinds to resolve
    uint8_t unused_02[2];      // 0x02 padding
    union {
        uint32_t handle;       // 0x04 reason 1: prop_data datum; reason 6: object_data datum
        real_point3d point;    // 0x04 reason 3 (relative to the actor) or 4 (absolute)
    } payload;
} actor_flee_source_reason; // size 0x10

// The targeted jump actor_move packs for the biped jump: 0x416790 writes it, 0x417fa0 turns it into a launch velocity.
typedef struct actor_jump_request {
    uint8_t valid;                    // 0x00
    uint8_t unknown_01[3];            // 0x01
    real_vector2d direction;          // 0x04 horizontal direction of the jump
    float horizontal_speed;           // 0x0c
    float vertical_speed;             // 0x10
} actor_jump_request;                 // size 0x14
typedef char actor_jump_request_size[sizeof(actor_jump_request) == 0x14 ? 1 : -1];

typedef struct actor {
    int16_t identifier;               // 0x00 datum_header
    uint8_t unknown_02[2];            // 0x02
    int16_t type;                     // 0x04 ActorType, copied from the actor tag type at Actor+0x14 by actor_new; indexes actor_type_procs
    uint8_t swarm;                    // 0x06 Actor.flags bit 26 "swarm"; the order builders refuse to act while it is set
    uint8_t unit_control_pending;     // 0x07 actor_new and actor_freeze_unit-like 0x429000 set it;
                                      //    actor_apply_queued_look_to_unit 0x42a640 then calls unit 0x569bf0 with
                                      //    CL=1 (take unit control) and clears it
    uint8_t active;                   // 0x08 actor_set_units_active and squad_activate gate on this
    uint8_t encounterless;            // 0x09 1 while on ai_globals.first_encounterless_actor list: set by
                                      //    ai_actor_link_to_unassigned_list 0x436940, cleared by unlink 0x436990;
                                      //    actor_delete picks unlink vs encounter_remove_actor
    uint8_t force_active;             // 0x0a hs ai_force_active_by_unit (0x47df80 -> 0x435540) stores it for
                                      //    encounterless actors; encounters_update_activation / 0x429270 OR it with
                                      //    encounter.force_active
    uint8_t swarm_pending;            // 0x0b encounter_activate sets it when a swarm actor could not get a swarm
    datum_index deactivation_time;    // 0x0c game_time stamped when active goes 1->0 (0x437e20, encounter_deactivate,
                                      //    actor_toggle_active_state); 0x42acd0 sorts inactive actors by it. int32
                                      //    time, not datum_index
    int16_t activation_delay;         // 0x10 90 while visible/forced, -30 per 0x437e20
                                      //    pass, deactivates below 31; same scheme as encounter.activation_delay.
                                      //    link_to_unassigned sets 90/0
    uint8_t can_go_dormant;           // 0x12 0x437e20/0x436190: 1 unless a unit's cluster is in the player-visible
                                      //    cluster mask or squad dormancy_disabled; 0x429270 wakes units when 0 (or
                                      //    force_active)
    uint8_t keep_unit_alive;          // 0x13 actor_attach_to_unit marks the unit pending-delete when this is clear
    int16_t inactive_ticks;           // 0x14 counted up each tick while the actor is dormant-eligible and cleared when it stops being dormant; past 59 the units are set active
    uint8_t unknown_16[2];            // 0x16
    datum_index unit_index;           // 0x18 the one unit object this actor controls; the unit points back at 0x1f4
    uint8_t counts_toward_encounter;  // 0x1c actor_unlink_unit decrements encounter+0x1c only when set
    uint8_t unknown_1d;               // 0x1d
    int16_t cluster_count;            // 0x1e actor_link_to_unit_cluster increments, actor_remove_from_unit_cluster decrements
    int16_t total_cluster_count;      // 0x20 actor_link_to_unit_cluster 0x4279f0 increments it with cluster_count but
                                      //    nothing decrements it; encounter_recompute_morale uses cluster_count/this
                                      //    as swarm vitality
    uint8_t unknown_22[2];            // 0x22
    datum_index cluster_unit_index;   // 0x24 head of the unit cluster list, chained through object+0x1fc
    datum_index swarm_index;          // 0x28 actor_create_swarm / actor_delete_swarm
    datum_index next_in_encounter;    // 0x2c next actor in the encounter member list, or in the unassigned list
    datum_index original_encounter_index; // 0x30 bsp deactivate 0x42c940 stores the encounter a carried actor left;
                                          //    0x42ce90 re-adds it via encounter_add_actor(unknown_38, actor, this)
                                          //    when that bsp loads; ai_squads_merge remaps it
    datum_index encounter_index;      // 0x34 owning encounter datum, none while unassigned
    int16_t original_squad_index;     // 0x38 squad_index saved with unknown_30 by 0x42c940 (bsp carry-over), passed
                                      //    back to encounter_add_actor by 0x42ce90 / ai_squads_merge
    int16_t squad_index;              // 0x3a index of this actor encounter_squad_state, relative to encounter.first_squad
    int16_t platoon_index;            // 0x3c index of this actor encounter_platoon_state, or -1
    int16_t team;                     // 0x3e kept in sync with object+0xb8 and encounter.team
    uint8_t squad_link_saved;         // 0x40 set once the actor was moved to another encounter/squad by the vehicle shuffle; saved_* hold the way back
    uint8_t unknown_41[3];            // 0x41
    datum_index saved_encounter_index; // 0x44 the encounter the actor belonged to before the shuffle
    int16_t saved_squad_index;        // 0x48 ... and its squad
    int16_t idle_counter;             // 0x4a 0x429430 advances it and trips the global update stagger past 15
    uint8_t needs_new_path;           // 0x4c 0x4017b0 issues a fresh path request while set; 0x429430 also writes it
    uint8_t unknown_4d;               // 0x4d
    int16_t target_reaction_threshold;// 0x4e ticks a prop's reaction_timer must reach before the actor refreshes its aim (set by the target relationship pass)
    datum_index first_prop;           // 0x50 head of the prop list, chained through prop.next_in_actor at +0x08
    datum_index nearest_orphan_prop_index; // 0x54 actor_target_relationship_think 0x41abd0 stores the nearest prop in
                                           //    state 4..5 (orphan) each pass (none while target itself is an
                                           //    orphan); orphan_timer penalty uses it
    datum_index actor_definition_tag; // 0x58 the actor tag index; actor_get_actor_definition can override it per unit
    datum_index actor_variant_tag;    // 0x5c the actor_variant tag index actor_new was called with
    int16_t pending_order_request;    // 0x60 one-shot request code (-1 none): actor_process_order_request takes it
                                      //   and clears it; 0x435420 sets 2
    int16_t standing_order_request;   // 0x62 used when no one-shot request is pending (-1 meaning 0); never
                                      //   cleared by the taker. squad_members_assign_team_and_request_order sets it
    int32_t last_order_request_time;  // 0x64 game time of the last processed request, -1 never; implicit requests
                                      //   are throttled to one per 45 ticks
    uint8_t sequence_id;              // 0x68 actor_new_and_attach_to_unit stores
                                      //    ScenarioActorStartingLocation.sequence_id (request+0x12);
                                      //    actor_select_move_position 0x4014c0 skips move positions whose sequence_id
                                      //    differs
    uint8_t unknown_69;               // 0x69
    int16_t awareness_level;          // 0x6a 0..3; actor_set_mode clamps it to 2 or 3 by mode, actor_update_awareness_level drives it
    int16_t mode;                     // 0x6c actor_set_mode writes it; indexes actor_mode_definitions
    int16_t combat_status;            // 0x6e actor_update_awareness_level: max(suspicion_status, minimum_combat_status,
                                      //   the per-target_combat_status floor table 0x655880). 0 is the "guarding"
                                      //   stage of ai_status (0x435680); > 3 counts as threatened (definite
                                      //   enemy: attack vs pursue shield fractions, dialogue variants), > 6 as
                                      //   engaged (has_engaged). The CEA names this step actor_situation_combat_status_update.
    uint8_t mode_changed;             // 0x70 actor_set_mode sets 1
    uint8_t unknown_71;               // 0x71
    int16_t minimum_combat_status;    // 0x72 floor of combat_status that orders impose (order request 10 sets 2,
                                      //   0x41fbc0 clears it); "minimum < combat_status" is the test for a threat
                                      //   the actor perceived itself (0x428180, grenade eligibility, morale)
    int16_t suspicion_status;         // 0x74 latched maximum of the recorded perception events (perception_event,
                                      //   the CEA's suspicion_combat_status); cleared when combat_status rises
                                      //   past it or suspicion_timer runs out
    int16_t unknown_76;               // 0x76
    int32_t suspicion_timer;          // 0x78 ticks, from perception_event_data (450 / 600 / 900 at the callers);
                                      //   actor_update_squad_link_state counts it down and clears suspicion_status at 0
    int32_t ticks_in_combat;          // 0x7c consecutive ticks at awareness_level 3, else 0
    int32_t ticks_alerted;            // 0x80 consecutive ticks with combat_status > 0, else 0
    int32_t ticks_threatened;         // 0x84 consecutive ticks with combat_status > 3, else 0
    int32_t ticks_since_threatened;   // 0x88 0 while combat_status > 3, then counts up; -1 (actor_new) never threatened
    uint8_t has_engaged;              // 0x8c set once combat_status exceeds 6, never cleared; encounter morale reads it
    uint8_t witnessed_death;          // 0x8d actor_scan_backup_and_panic_reaction 0x423220 sets it whenever a prop's
                                      //    just_died is processed; encounter_recompute_morale needs has_engaged &&
                                      //    this to start post-combat
    uint8_t command_list_run_immediately;// 0x8e when set, actor_process_order_request runs pending_command_list even while awareness_level is 0 and without the reload check; cleared with it; nothing raises it in this build
    uint8_t unknown_8f;               // 0x8f
    int16_t pending_command_list;     // 0x90 command list index stored when the actor is told to run one while
                                      //   inactive (0x407140), -1 none (actor_new / ai_unit_create_actor)
    int16_t command_list_delay;       // 0x92 int16 set 2 at creation (ai_unit_create_actor,
                                      //    actor_new_and_attach_to_unit), -1 per 0x429270 tick; 0x40ab80 holds a
                                      //    pending_command_list while > 0
    int32_t command_list_finished_time; // 0x94 actor_mode_obey_process 0x407340 stamps game_time when the command
                                        //    list completes (mode_data[5]=1); 0x434f20 (ai_command_list_status)
                                        //    reports 1 for 150 ticks after
    uint8_t search_firing_positions;  // 0x98 0x412880 picks attacking_search/defending_search groups when set;
                                      //    0x413e50 flips it when the winner lies outside the current mask;
                                      //    flee/guard enter clear it
    uint8_t flying;                   // 0x99 Actor.flags bit 21 "flying"; read by every steering and step-test routine
    uint8_t unknown_9a[2];            // 0x9a
    actor_mode_data mode_data;        // 0x9c actor_set_mode memcpys actor_mode_definition.data_size bytes here.
                                      //   0x84 and not 0xbc: actor_play_first_valid_vocalization
                                      //   @0x40e260 reserves exactly 132 bytes of stack for the block it
                                      //   hands to actor_set_mode, and the two points below are refreshed
                                      //   every tick by the aiming code, so they cannot be inside the union.
    real_point3d aim_origin;          // 0x120 the firing / eye origin; 0x40e7b0 traces from it and 0x40fcb0
                                      //   measures the aim target against it
    real_point3d body_position;       // 0x12c the position every range and scoring routine uses; read by
                                      //   0x4112b0, 0x411bf0, 0x412ba0, 0x4180c0 and the avoidance sampler
    uint8_t unknown_138[12];          // 0x138
    bsp_leaf_reference location;      // 0x144 bsp leaf and cluster of the actor's head position; refreshed by actor_refresh_combat_context
    uint8_t unknown_14c[12];          // 0x14c
    datum_index active_unit_index;    // 0x158 preferred unit object for movement; 0x4193d0 falls back to unit_index
    uint8_t airborne;                 // 0x15c actor_refresh_combat_context 0x4297a0: biped airborne_ticks
                                      //    (unit+0x501) >= 6, on foot only; movement/firing/obey code skip while set
    uint8_t in_water;                 // 0x15d 0x4297a0: scenario_location_get_water_and_weather at the head marker;
                                      //    compared with prop.in_water (actor_rate_potential_target 0x41fd50), blocks
                                      //    firing in actor_update_firing_state
    int16_t vehicle_driving_type;     // 0x15e 0x4297a0: 0 not driving, 1 driver, 2 hovering / 3 sidestep / 4 flying
                                      //    per Vehicle flags ai_driver_enable/flying/can_sidestep/hovering
                                      //    (0x800/0x1000/0x2000/0x4000)
    uint8_t order_committed;          // 0x160 the order builders set it once the actor commits to the order they built
    uint8_t vehicle_gunner;           // 0x161 0x4297a0 sets it when the parent vehicle's gunner (vehicle+0x328) is
                                      //    this unit; aim/threat-weapon code then uses the vehicle
                                      //    (active_unit_index)
    uint8_t vehicle_gunner_bombards[2]; // 0x162 0x4297a0: gunner && ActorVariant.bombardment_range (+0x14c) > 0;
                                        //    orphan inspection 300 vs 45 ticks (0x41abd0, local misnamed
                                        //    nearly_dead), uncover mode keeps going
    int32_t pathfinding_surface_index; // 0x164 0x4297a0 copies biped cached_ground_surface_index (+0x4dc); path
                                       //    request start_surface_index (0x4017b0); -1 in vehicles/swarms; 0x429570
                                       //    lead-position refills it
    real_point3d pathfinding_point;   // 0x168 biped cached_ground_point (+0x4e0) copied by 0x4297a0; the start_position
                                      //    of path requests
    real_vector3d facing;             // 0x174 the actor unit forward vector, NOT a position: all 35 arithmetic
                                      //   uses across the module dot it against a normalized delta and compare
                                      //   the result against a cosine (0.4, 0.5, 0.8660254, 0.984). The real
                                      //   position is body_position at 0x12c. Together with the two vectors
                                      //   below it forms the 3x3 basis actor_snapshot_orientation @0x4294d0
                                      //   copies to 0x6fc / 0x708 / 0x714, which is what that name describes.
    real_vector3d unit_aiming_vector; // 0x180 the unit's aiming vector, copied in by the pre-update step; snapshotted to 0x708
    real_vector3d unit_looking_vector; // 0x18c the unit's looking vector, copied in by the pre-update step; snapshotted to 0x714
    real_vector3d looking_left_vector;// 0x198 normalize(unit_looking_vector x world up), rebuilt by the pre-update step
    real_vector3d looking_up_vector;  // 0x1a4 looking_left_vector x unit_looking_vector
    int32_t stuck_projectile_index;   // 0x1b0 datum_index (declared int32): 0x4297a0 sets it to an attached
                                      //    projectile child (stuck grenade / the danger projectile); flee panic 9/10
                                      //    ends when none
    uint8_t enemy_child_attached;     // 0x1b4 an enemy-team biped is attached as a child object of the unit (actor_alert_from_flag_1b4)
    uint8_t on_fire;                  // 0x1b5 the unit's flaming ticks are above zero (actor_alert_from_damage)
    uint8_t unknown_1b6[2];           // 0x1b6
    float body_vitality;              // 0x1b8 0x4297a0 copies unit body_vitality; berserk_damage_threshold test,
                                      //    crouch/vocalization code
    float shield_vitality;            // 0x1bc 0x4297a0 copies unit shield_vitality;
                                      //    compared to hide_shield_fraction, ==1.0f in crouch state
    float recent_body_damage;         // 0x1c0 0x4297a0 copies unit recent_body_damage; berserk_damage_amount,
                                      //    cover_damage_threshold, panic_damage_threshold tests
    float recent_shield_damage;       // 0x1c4 0x4297a0 copies unit recent_shield_damage
                                      //    (4th of the four unit values)
    uint8_t stood_down;               // 0x1c8 encounter_propagate_platoon_state_to_actors copies
                                      //    encounter.stood_down; 0x41abd0 resets ticks_since_engaged to -1 while set
    uint8_t platoon_defending;        // 0x1c9 encounter_add_actor and 0x439d80 propagate copy
                                      //    encounter_platoon_state.defending; actor_escalate_check_leader_flag reads
                                      //    it
    uint8_t playfight;                // 0x1ca encounter_propagate_platoon_state_to_actors copies encounter.playfight
                                      //    (hs ai_playfight); grenade, perception-scale and aim-wander code change
                                      //    behaviour when set
    uint8_t charge_disallowed;        // 0x1cb hs ai_allow_charge (0x47e790) -> 0x434d40 stores !allow on every
                                      //    referenced actor; charge mode / combat transitions test it
    uint8_t grenade_ally_phase_flag;     // 0x1cc 0 after the first grenade ally pass, 1 when the second pass found more than one ally; actor_new sets 0
    uint8_t unknown_1cd[3];           // 0x1cd
    datum_index nearby_friend_prop_index; // 0x1d0 0x40e540 records the nearest friendly prop whose actor is
                                          //    searching/investigating (combat_status 2..3, modes 5-8); wait mode
                                          //    follows it within 8, vocalization looks at it
    int16_t try_to_fight_type;        // 0x1d4 0 nothing / 1 ai reference / 2 player: hs
                                      //    ai_try_to_fight(_nothing/_player) -> 0x434cc0 etc.; tracking-speed code
                                      //    marks props preferred_target accordingly
    uint8_t unknown_1d6[2];           // 0x1d6
    uint32_t try_to_fight_reference;  // 0x1d8 packed ai reference set by ai_reference_set_search_target_point
                                      //    (try_to_fight_type 1); actor_target_update_tracking_speed matches its low
                                      //    word to the owner's encounter and reads the word at 0x1da as the
                                      //    squad/platoon index (kind in the top bits)
    datum_index conversation_index;   // 0x1dc ai_conversation_stop clears this and conversation_participant
    datum_index conversation_participant;// 0x1e0
    int16_t post_combat_action;       // 0x1e4 int16 line id written by encounter_choose_vocalizations 0x438580
                                      //    (post-combat pick) / cleared out of post_combat; search_wait order and
                                      //    report_command_status switch on it
    uint8_t unknown_1e6[2];           // 0x1e6
    datum_index post_combat_prop_index; // 0x1e8 prop paired with post_combat_action by 0x438580;
                                        //    actor_build_order_search_wait walks to its pathfinding_point; cleared
                                        //    with it
    actor_target_tally tally;         // 0x1ec the 0x7b-byte perception tally actor_choose_best_target
                                      //   zeroes (0x1e dwords, then a word, then a byte) and refills
                                      //   every time it walks the prop list. The three per-actor-type
                                      //   runs inside it are what fixes the 0x1ec base: the last one
                                      //   ends at 0x266, one byte short of target_combat_status.
    uint8_t unknown_267;              // 0x267 the byte the zeroing run does not reach
    int16_t target_combat_status;     // 0x268 actor_update_target_combat_status writes it, actor_update_awareness_level reads it
    uint8_t unknown_26a[2];           // 0x26a
    datum_index target_last_seen_time; // 0x26c int32 time (declared datum_index): actor_update_target_combat_status
                                       //    0x4200d0 copies target prop last_seen_time while visual_perception > 0;
                                       //    0x40b840 hide timer
    datum_index target_unit_index;    // 0x270 the unit the actor is fighting; actor_choose_best_target writes it
    uint8_t ever_had_target[4];       // 0x274 0x41abd0 latches 1 once target_combat_status > 5, never cleared in the
                                      //    rewrite; ai_communication_broadcast gate 0 is "not yet" (as
                                      //    encounter.ever_had_target)
    int32_t ticks_since_engaged;      // 0x278 -1 never; 0x41abd0 sets 0 while target_combat_status >= 10, +1
                                      //    otherwise, -1 while stood_down; ai_communication_broadcast uses it as
                                      //    encounter.ticks_since_engaged
    uint8_t target_alive;             // 0x27c 0x4200d0: target prop not dead (or object vitality bit 2 clear for
                                      //    orphans); ai_communication_broadcast uses it in place of
                                      //    encounter.has_live_target
    uint8_t unknown_27d[3];           // 0x27d
    int16_t danger_type;              // 0x280 0x41ea60 and 0x41ec90 only register a danger that outranks this
    int16_t danger_owner_relation;    // 0x282 who set the danger off: 0 an enemy or unknown, 1 a friend, 2 this actor's
                                      //    own unit (projectile danger); passed on with the threat direction
    int16_t danger_reaction_ticks;    // 0x284 countdown before the actor reacts: 6 for a point danger, 0x1e for a
                                      //    projectile, 0x14 for a vehicle; actor_target_relationship_think decrements it
    uint8_t danger_reaction_delayed;  // 0x286 the caller allows the countdown to run (the reaction is otherwise
                                      //    immediate)
    uint8_t danger_reacting;          // 0x287 actor_target_relationship_think sets it when a danger is noticed;
                                      //    actor_find_best_firing_position only reports a danger when set
    uint8_t danger_dive;              // 0x288 dive_from_grenade_chance roll (relationship_think); 0 for own danger
    uint8_t danger_reported;          // 0x289 the danger line was broadcast for the current danger
    uint8_t danger_is_own;            // 0x28a actor_danger_update_reaction 0x41eda0: the danger projectile's parent
                                      //    is this actor's unit; suppresses danger reaction / avoidance (0x41abd0,
                                      //    0x40c040)
    uint8_t unknown_28b;              // 0x28b
    datum_index danger_object_index;  // 0x28c
    datum_index danger_owner_unit;    // 0x290 the unit that fired the projectile or drives the vehicle (-1 none)
    float danger_object_radius;       // 0x294 radius of the dangerous object when it was registered; the avoid sphere
    real_point3d danger_object_position;// 0x298 its position when it was registered (projectile and vehicle dangers)
    real_vector3d danger_object_velocity;// 0x2a4 its velocity when it was registered
    real_point3d flee_from_point;     // 0x2b0 0x4146c0 resolves the point the actor flees away from; the
                                      //   danger scoring rule also uses it as a segment start
    real_vector3d danger_velocity;    // 0x2bc 0x41eda0 copies the danger
                                      //    object's velocity; end point = pos + 45*vel; actor_find_danger_escape uses
                                      //    -vel as axis
    real_point3d danger_segment_end;  // 0x2c8 0x4112b0 builds the segment flee_from_point -> here
    float danger_distance;            // 0x2d4 distance of the registered danger; a new one of the same type replaces it
                                      //    only when closer
    float danger_radius;              // 0x2d8 the sphere around danger_center a candidate has to be inside
    real_point3d danger_center;       // 0x2dc
    int16_t danger_countdown;         // 0x2e8 ticks until the registered danger goes off (grenade fuse / animation frames left), -1 unknown
    uint8_t unknown_2ea[2];           // 0x2ea
    uint8_t attack_pending;           // 0x2ec set by the look-at request pass; the squad-attack alert and the shield damage escalation consume it
    uint8_t vehicle_eviction;         // 0x2ed player_execute_pending_interaction / unit_find_best_seat_to_enter ->
                                      //    0x42b810 sets it when a friendly player wants the seat;
                                      //    actor_process_vehicle_seat_exit exits (CEA stimulus_vehicle_eviction)
    int16_t look_at_priority;         // 0x2ee 0x421bc0 keeps only the highest-priority look-at point
    uint8_t surprise_pending;         // 0x2f0 set by the surprise reaction; actor_alert_from_disturbance consumes it
    uint8_t unknown_2f1[3];           // 0x2f1
    datum_index look_at_reference;    // 0x2f4 the datum the highest-priority look-at request carries (swapped by
                                      //    actor_replace_object_reference)
    uint8_t look_at_has_point;        // 0x2f8 the request supplied a point
    uint8_t unknown_2f9[3];           // 0x2f9
    real_point3d look_at_point;       // 0x2fc the point to look at
    int16_t pending_panic_type;       // 0x308 max-raised panic kind (1 squad attack,2/3 friend killed,6,8 leader
                                      //    killed,11 0x1b4,12 damaged); 0x40a700 consumes -> flee order code / raises
                                      //    flee.panic
    uint8_t unknown_30a[2];           // 0x30a
    uint32_t pending_panic_prop_index; // 0x30c prop paired with pending_panic_type (source fled from); written with
                                       //    it by 0x4233d0/0x423220/actor_alert_from_*, swapped by
                                       //    actor_replace_object_reference
    int16_t escalation_level;         // 0x310 the escalation level the alert flow raised; escalate_apply compares it with its threshold and clears it
    int16_t search_priority;          // 0x312 0x421af0 keeps only the highest-priority search position
    uint8_t search_position_valid;    // 0x314 the highest-priority search request supplied a position
    uint8_t unknown_315[3];           // 0x315
    real_point3d search_position;     // 0x318 the position to search
    int32_t search_surface_index;     // 0x324 pathfinding surface of the position (-1 unknown); the start surface
                                      //    for actor_firing_position_near_point
    uint32_t search_position_extra;   // 0x328 caller parameter stored with the position (0 in the known callers); the guard order data copies it
    uint8_t search_velocity_valid;    // 0x32c the request supplied a velocity
    uint8_t unknown_32d[3];           // 0x32d
    real_vector3d search_velocity;    // 0x330 the direction the searched target was moving
    uint32_t search_velocity_ticks;   // 0x33c caller parameter (90 in the known caller); the guard order data takes it
                                      //    as its first word and uses the velocity only while it is positive
    datum_index search_prop_index;    // 0x340 prop the request came from (swapped by actor_replace_object_reference)
    uint32_t search_prop_value;      // 0x344 caller parameter stored with search_prop_index (150 in the known caller); the guard order data takes its low word when a prop is set
    uint8_t search_prop_flag;        // 0x348 caller parameter stored with search_prop_index (0 in the known callers); the guard order data copies it when a prop is set
    uint8_t unknown_349;              // 0x349
    int16_t perception_event;         // 0x34a 0x422070 records the highest-priority pending perception event
    int32_t perception_event_data;    // 0x34c
    // 0x350..0x36b block; actor_new zeroes 0x1a dwords starting here, i.e. 0x350..0x3b7. Member types agree between
    // actor_update_crouch_state, actor_update_facing_change_timer, actor_update_grenade_and_morale_reactions and
    // actor_movement_update; the ROLE of 0x358/0x35a is contested (crouch state + crouch timer vs facing-change
    // pending flag + ticks) so those are named after the first user. 0x35c..0x35f flags only used by crouch_state.
    float threat_level;               // 0x350 crouch_state: threat level (single user)
    float danger_meter;               // 0x354 smoothed threat level (crouch_state) / danger meter reset to 0 by
                                      //    grenade reactions / smoothing clamp (facing_change_timer)
    uint8_t crouch_active;            // 0x358 byte flag; crouching (crouch_state) vs pending flag (facing timer);
                                      //    also copied to 0x426/0x427 by fight/charge/avoid mode updates
    uint8_t unknown_359;              // 0x359
    int16_t crouch_ticks;             // 0x35a tick counter (crouch timer / facing-change ticks)
    uint8_t crouch_cover_flags[4];    // 0x35c crouch_state neighbour flags
    int16_t incoming_fire_ticks;      // 0x360 countdown: crouch_state; actor_movement_update tests >= 1
    uint8_t charge_trigger_active;    // 0x362 evaluate_custom_charge_trigger is running; cleared on every early exit
    uint8_t charge_trigger_decision;  // 0x363 the decision evaluate_custom_charge_trigger returns
    int16_t charge_trigger_ticks;     // 0x364 counted down to a new charge decision
    int16_t charge_trigger_delay;     // 0x366 counted down to the next decision attempt (30 after a decision)
    int16_t evasion_delay_ticks;      // 0x368 evasion_delay_time * 30 set by grenade reactions, must be 0 to retry;
                                      //    crouch_state counts it down
    uint8_t unknown_36a[2];           // 0x36a
    datum_index last_evasion_time;    // 0x36c 0x40b920: at most every 30 ticks (dive from retreat threat /
                                      //    evasion_seek_cover roll once danger meter +0x354 >= evasion threshold);
                                      //    actor_new -1
    datum_index last_cover_attempt_time; // 0x370 actor_try_grenade_evasion 0x40c530: shield under
                                         //    hide_shield_fraction while fighting tries to get away at most every 30
                                         //    ticks since it
    uint8_t defending;                // 0x374 copy of platoon defending (encounter_add_actor, 0x4213b0 from 0x1c9);
                                      //    picks defending_crouch/evasion_threshold and *_guard firing-position group
                                      //    (0x412880)
    uint8_t always_charge;            // 0x375 0x4213b0: = berserking || Actor always_charge_at_enemies(0x800) ||
                                      //    (always_charge_in_attacking_mode(0x1000000) && !defending), 0 in vehicle;
                                      //    actor_berserk 0x421a40 sets 1
    uint8_t ignores_glass;            // 0x376 actor_new rolls Actor.glass_ignorance_chance at Actor+0x90 once into this
    uint8_t friendly_player_greeted;  // 0x377 0x41abd0: first time a friendly player prop (is_parented) is seen
                                      //    within 7 units, broadcasts event 0x19 and sets it; never cleared in the
                                      //    rewrite
    uint8_t berserking;               // 0x378 set by 0x421a40 (actor_berserk: unit flag 0x80) from escalate/berserk
                                      //    triggers; selects berserk_firing_ranges/berserk_melee_range; 0x4213b0 ends
                                      //    it out of combat
    uint8_t berserk_announced;        // 0x379 cleared by 0x421a40 on berserk change; actor_movement_update queues the
                                      //    turn-to-target secondary action + event 0x2a once while berserking, then
                                      //    sets it
    uint8_t unknown_37a[2];           // 0x37a
    int32_t search_wait_time;         // 0x37c 0x4028e0 reads this and unknown_388 as reaction wait thresholds
    int32_t last_melee_time;          // 0x380 0x401da0 stamps game_time when the melee strike range is reached (-1 on
                                      //    leap); 0x40c620 waits difficulty-scaled Actor.melee_attack_delay*30 since
                                      //    it
    uint32_t last_vehicle_search_time; // 0x384 actor_seek_vehicle_to_board 0x40ac70: at most every 45 ticks since
                                       //    this game_time; actor_new -1
    int32_t last_vehicle_charge_time; // 0x388 0x401da0 stamps now for charge kinds 4/5; 0x40c620 gates vehicle charge
                                      //    on now > this + Vehicle.ai_charge_repeat_timeout(+0x390)*30
    uint8_t vehicle_exit_forced;      // 0x38c 0x40b080 holds its "forced" flag here during the exit; 0x42c370
                                      //    (misnamed actor_notify_weapon_pickup_once) skips the exit event 0x25 when
                                      //    set
    uint8_t unknown_38d[3];           // 0x38d
    datum_index exited_vehicle_index; // 0x390 0x40b080 stores active_unit_index on exit;
                                      //    actor_vehicle_not_recently_left 0x40ac30 compares candidate vehicle to it
    datum_index exited_vehicle_reentry_time; // 0x394 0x40b080 sets game_time+180; 0x40ac30 allows re-boarding the
                                             //    same vehicle once game_time >= it
    datum_index last_flee_abort_time; // 0x398 actor_mode_flee_process 0x4037f0 stamps game_time when fleeing gives up
                                      //    (+0xaa); 0x40a700 ignores new panic within 7 ticks of it
    datum_index panic_cooldown_time;  // 0x39c actor_mode_flee_tick sets game_time+750 while flee.panic>0;
                                      //    panic_in_groups rolls (0x4233d0, 0x423220) need it < game_time
    datum_index found_body_time;      // 0x3a0 actor_start_search_timer 0x422130 stamps game_time when an unaware
                                      //    actor finds a dead friend; later bodies that died before max(this,
                                      //    enc.last_idle_time) are ignored
    datum_index retreat_end_time;     // 0x3a4 0x420ec0 stamps game_time when retreat_timer runs out; a friend's
                                      //    retreat only counts toward friends_retreating_trigger if it started after
                                      //    this
    int16_t retreat_timer;            // 0x3a8 0x420ec0: ticks left, random Actor.retreat_time*30 when a danger bucket
                                      //    beats priority 5 (danger/friends killed/retreating triggers); counted down
                                      //    there
    uint8_t unknown_3aa[2];           // 0x3aa
    datum_index retreat_prop_index;   // 0x3ac 0x420ec0 stores the winning danger prop;
                                      //    actor_target_update_active_flag/actor_replace_object_reference clear it
                                      //    with retreat_timer
    datum_index retreat_start_time;   // 0x3b0 0x420ec0 stamps game_time with retreat_prop_index; compared against
                                      //    allies' retreat_end_time
    float stood_down_body_vitality;   // 0x3b4 0x4213b0 copies body_vitality (+0x1b8) while platoon stood_down
                                      //    (+0x1c8); actor_new 1.0; encounter_choose_vocalizations compares drop >
                                      //    0.3
    int16_t firing_position_index;    // 0x3b8 the encounter firing position this actor has claimed, or -1
    uint8_t firing_position_without_path; // 0x3ba actor_claim_firing_position = !path_ok; blocks random fallback
                                          //    (0x413e50) and the fight_tick discard (recognition push);
                                          //    saved/restored by flee
    uint8_t grenade_evasion_active;   // 0x3bb set with evasion_delay_ticks by grenade reactions; cleared by
                                      //    actor_claim_firing_position / set_destination_firing_position;
                                      //    movement_action_resolve completes a type-3 action at once while set
    uint8_t target_lost;              // 0x3bc actor_should_hold_position sets it when shooting at a prop in state
                                      //    4..5; cleared on new target (0x40cdf0) and by 0x41fbc0(none)
    uint8_t target_lost_reported;     // 0x3bd the target-lost line was broadcast; cleared when the pursuit target changes
    uint8_t unknown_3be[2];           // 0x3be
    datum_index pursuit_target_prop_index; // 0x3c0 0x40cdf0 resets pursuit_position_count when target_unit_index
                                           //    differs from it and stores the target; actor_update_combat_behavior
                                           //    compares it
    int16_t pursuit_position_count;   // 0x3c4 0x40cdf0 counts firing positions taken vs pursuit_target_prop_index
                                       //    (event 0x10 first); limited by Actor num_positions coord/normal
    int16_t recognition_cursor;       // 0x3c6 ring cursor, advanced modulo 4 by 0x4141a0
    actor_recognition_entry recognition[4];// 0x3c8 actor_set_mode and 0x414140 reset all four firing_position_index to -1
    uint8_t recognition_valid;        // 0x3d8 0x4141a0 sets it, 0x414140 and actor_set_mode clear it
    uint8_t recognition_type;         // 0x3d9
    uint8_t unknown_3da[2];           // 0x3da
    real_point3d recognition_position;// 0x3dc copied out of the encounter ScenarioFiringPosition block (stride 0x18)
    int16_t flee_reason;              // 0x3e8 why the actor wants to move away from flee_source: 0 none, 3..7 rising
                                      //    urgency (avoid 5, overwhelmed 7); cleared with the rest of the per-tick
                                      //    control block (0x3e8..0x46b) every update
    uint8_t unknown_3ea[2];           // 0x3ea
    actor_flee_source_reason flee_source;// 0x3ec what the actor flees from or looks at: code 1 prop, 2 firing target,
                                      //    3 point relative to the actor, 4 absolute point (0x4146c0 resolves it)
    int16_t look_posture;             // 0x3fc per-tick control, set by every actor_mode_*_update (0 sleep,1
                                      //    noncombat,2 guard,3 search,4 combat); 0x4150f0 picks
                                      //    noncombat/guard/combat_idle_facing by it
    uint8_t unknown_3fe[2];           // 0x3fe
    actor_movement_action queued_movement;// 0x400 the action the setters at 0x417610..0x417910 write
    int16_t secondary_action;         // 0x418 0x417a60 queues it, actor_action_has_queued_secondary reads it
    uint8_t unknown_41a[2];           // 0x41a
    real_vector2d secondary_action_direction; // 0x41c direction the queued secondary action faces (actor_queue_secondary_action)
    uint8_t unknown_424[2];           // 0x424 cleared together with the crouch decision by most mode updates
    uint8_t crouch_decision[2];       // 0x426 the crouch decision of the mode update, mirrored into the crouch control flag
    uint8_t crouch_hold;              // 0x428 charge and obey keep a crouch going while it is set
    uint8_t cowering;                 // 0x429 flee: the panic is in the cowering band (9..12)
    uint8_t force_turn;               // 0x42a movement forces turn_required while it is set
    uint8_t unknown_42b;              // 0x42b
    int16_t movement_style_override;  // 0x42c control_animation_mode to use; -1 derives it from the awareness level
                                      //    (reset to -1 every update, set by the obey mode update)
    int16_t strafe_axis_override;     // 0x42e steering axis passed to actor_movement_apply_steering (its cached_axis); reset to -1 every update
    uint8_t move_in_direction;        // 0x430 when set actor_movement_update steers straight along unknown_434; set
                                      //    by actor_mode_obey_update (command-list aim bit 0 / look)
                                      //   unknown_434 instead of running the avoidance sampler
    uint8_t unknown_431[3];           // 0x431
    real_vector3d move_direction;     // 0x434 explicit steering vector copied to unknown_518 when move_in_direction
                                      //    is set (actor_movement_update, actor_mode_obey_update)
    uint8_t jump_requested;           // 0x440 charge leap (mode_data +0xa8) and obey jump set it;
                                      //    actor_movement_update then plays anim state 0x27 (0x569b30) or sets the
                                      //    jump control bit
    uint8_t jump_is_leap;             // 0x441 = horizontal*0.7 > vertical (charge +0xb8/+0xbc, obey +0xb0/+0xb4);
                                      //    actor_movement_update then tries the leap animation (state 0x27) + event
                                      //    0x2f
    uint8_t jump_parameters_valid;    // 0x442 when set actor_movement_update copies jump_facing/velocities into the
                                      //    0x530 control record; charge_update sets 1, obey from +0xa9 bit 4
    uint8_t unknown_443;              // 0x443
    real_vector2d jump_facing;        // 0x444 2D direction: charge copies the solved leap direction (0x401da0
                                      //    md+0x14/0x18), obey the facing; copied to unknown_530[4]/[8]
    float jump_horizontal_velocity;   // 0x44c charge copies md+0x1c (horizontal_speed from 0x4beb30), obey +0xb0;
                                      //    copied to unknown_530[12]
    float jump_vertical_velocity;     // 0x450 charge copies md+0x20 (second 0x4beb30 output), obey +0xb4; copied to
                                      //    unknown_530[16]
    uint8_t wants_to_fire;            // 0x454 per-tick control set by mode updates; 0x40e7b0 fires at
                                      //    target_unit_index (or shoot point when +0x45d); 0x435680 activity 6;
                                      //    informant test in 0x41c8f0
    uint8_t unknown_455[2];           // 0x455
    uint8_t force_fire;               // 0x457 obey scripted target sets it; 0x40e7b0 skips the fire cooldown and
                                      //    stance/crouch restrictions when set; actor_should_hold_position returns 0
                                      //    for it
    float burst_duration_override;    // 0x458 aim wander 0x40fcb0: when > 0 used instead of the random burst length;
                                      //    obey copies +0x10c into it (header calls that an object, conflicting)
    uint8_t throw_grenade;            // 0x45c set by 0x40db00 once facing the grenade impact point and by obey
                                      //    (+0x110); 0x40e7b0 then throws (tops up grenade, flag 0x2000, event 9)
    uint8_t forced_aim_valid;         // 0x45d obey and guard set it to fire at forced_aim_point; the firing state then targets the point
    uint8_t unknown_45e[2];           // 0x45e
    real_point3d forced_aim_point;    // 0x460 the point the actor fires at while forced_aim_valid is set
    actor_movement_action active_movement;// 0x46c the six dwords the setters copy over from queued_movement
    uint8_t movement_completed;       // 0x484 actor_movement_action_complete sets it
    uint8_t unknown_485[3];           // 0x485
    real_point3d destination;         // 0x488 actor_movement_action_resolve 0x41a460 resolves the action to this
                                      //    world point (firing/move position, prop, point); goal of
                                      //    path_find_set_goal; check_arrival uses it
    uint32_t destination_surface_index; // 0x494 0x41a460 writes the surface index with destination (position/prop
                                        //    surface), passed to path_find_set_goal; actor_new/clear_target_state -1
    uint32_t destination_radius;      // 0x498 0x41a460: 0 or the action's radius; path_find_set_goal 3rd arg; arrival
                                      //    when path end within it; float (declared uint32)
    uint8_t unknown_49c[4];           // 0x49c
    float movement_timer;             // 0x4a0 actor_movement_action_complete zeroes it
    uint8_t path_resolved_this_tick;  // 0x4a4 0x41a460 sets 1 after pathing; 0x429270 clears every tick; movement
                                      //    setters only re-resolve on needs_new_path when clear
    uint8_t unknown_4a5[3];           // 0x4a5
    uint8_t movement_action_complete; // 0x4a8 actor_movement_action_is_complete returns it
    uint8_t unknown_4a9[3];           // 0x4a9
    real_point3d path_end_point;      // 0x4ac the end of the current path; the guard test for being in place and the movement towards-test measure against it
    uint8_t unknown_4b8[4];           // 0x4b8
    float path_remaining_distance;    // 0x4bc path record +0x14 (0x43a4d0: distance from path end to goal); 0x41a460
                                      //    completes when near; 0x401da0 marks target engaged when > wait radius
    uint8_t unknown_4c0;              // 0x4c0 advance_waypoint: with waypoint_reached, completes the movement action
    int8_t waypoint_count;            // 0x4c1 advance_waypoint stops advancing at count - 1
    int8_t waypoint_cursor;           // 0x4c2 current index into the 16-byte waypoint records at 0x4c8
    uint8_t unknown_4c3[9];           // 0x4c3
    uint32_t unknown_4cc;             // 0x4cc
    uint8_t unknown_4d0[52];          // 0x4d0
    uint8_t moving;                   // 0x504 CEA control.moving; movement_update 0x416790/advance_waypoint 0x4163e0
                                      //    set it while following a path; face_forward = moving &&
                                      //    moving_facing_direction==0 (squad_action_execute)
    uint8_t forced_aim;               // 0x505 set by 0x414250 when direction spec 0x3ec decodes into 0x524;
                                      //    apply_steering 0x4180c0 then calls strafe-axis chooser = CEA
                                      //    actor_move_calculate_controlled_by_aiming
    uint8_t waypoint_reached;         // 0x506 0x4180c0 out-param = |desired_movement_vector| <= accuracy (CEA
                                      //    movement_complete); 0x4163e0 advances waypoint cursor when set, completes
                                      //    action on last one
    uint8_t movement_thwarted;        // 0x507 0x4180c0 out-param set when facing not yet inside turn cone so no step
                                      //    taken (CEA actor_move_calculate_movement *movement_thwarted); 0x4163e0
                                      //    tests it
    uint8_t crouching;                // 0x508 0x416790 stores crouch decision (0x426/0x427, Actor
                                      //    cannot_move_while_crouching) mirrored to control flag bit0; 0x4173a0
                                      //    crouch_velocity_modifier; 0x40e7b0 crouch gun offset
    uint8_t unknown_509;              // 0x509
    int16_t moving_facing_direction;  // 0x50a CEA control.moving_facing_direction; 0x4180c0 *desired_facing_direction
                                      //    out (0 fwd,1 back,2/3 sides,4 free); squad_action_execute moving_forward =
                                      //    moving && ==0
    real_point3d current_waypoint;    // 0x50c 0x4163e0 copies the current 16-byte path waypoint point
                                      //    (0x4c8+cursor*0x10) here
    real_point3d desired_movement_vector; // 0x518 0x4163e0 = current_waypoint - body_position; passed as
                                          //    desired_movement_vector (CEA actor_move_calculate_movement) to
                                          //    0x4180c0; look decode code 0 reads it
    real_vector3d forced_aim_direction; // 0x524 out of 0x4146c0 (actor_look_decode_direction) via 0x414250; passed as
                                        //    forced_aim_direction to 0x418a40 (CEA
                                        //    actor_move_calculate_controlled_by_aiming) in 0x4180c0
    actor_jump_request jump_velocity_request; // 0x530 0x416790 packs the targeted jump from 0x440..0x450; 0x417fa0 turns it into
                                       //    launch velocity for biped jump 0x55ecf0
    int16_t vocalization_line;        // 0x544 actor_clear_vocalization zeroes 0x544, 0x546 and 0x548
    int16_t vocalization_variant;     // 0x546
    int16_t vocalization_state;       // 0x548
    uint8_t unknown_54a[2];           // 0x54a
    actor_flee_source_reason vocalization_source;// 0x54c the prop or point the pending vocalization is about
                                      //    (copied wholesale from the context actor_begin_vocalization is handed)
    uint8_t idle_major_active;        // 0x55c 0x414d00 (CEA actor_look_idle_new_major_direction) sets it once idle
                                      //    major direction+timer armed; 0x415480 clears/tests it; relationship_think
                                      //    keeps its prop
    uint8_t idle_major_is_aiming;     // 0x55d 0x414d00 stores use_aiming_deviation (CEA major_is_aiming param);
                                      //    0x415480 tests it with idle_major_active
    uint8_t idle_interesting_direction; // 0x55e interesting-prop flag from 0x414a90 (CEA interesting_direction)
    uint8_t idle_minor_active;        // 0x55f idle minor direction active (0x414f50)
    int32_t idle_facing_timer;        // 0x560 idle facing timer
    int32_t idle_major_timer;         // 0x564 0x414d00 arms it from 0x415150 (actor_look_idle_timer, type
                                      //    aiming/looking); 0x415480 decrements it and re-picks at 0
    int32_t idle_minor_timer;         // 0x568 0x414f50 (CEA actor_look_idle_new_minor_direction) arms it from
                                      //    0x415150 type 2; 0x415480 decrements, re-randomizes at 0
    int16_t idle_major_direction_type; // 0x56c code word of 16-byte direction_specification at 0x56c (1 prop, 4
                                       //    point) written by 0x414d00/0x415480, decoded by 0x4146c0; 0x428470
                                       //    replace_object_reference patches 0x570
    uint8_t unknown_56e[2];           // 0x56e
    union {
        real_point3d idle_major_point;  // 0x570 idle_major_direction_type 4
        datum_index idle_major_prop_index; // 0x570 idle_major_direction_type 1
    };
    int16_t idle_look_direction_type; // 0x57c 1 prop, 4 point (actor_look_randomize_direction writes 4)
    uint8_t unknown_57e[2];           // 0x57e
    union {
        real_point3d idle_look_point;   // 0x580 idle_look_direction_type 4
        datum_index idle_look_prop_index; // 0x580 idle_look_direction_type 1 (replace_object_reference patches it)
    };
    uint8_t look_claimed;             // 0x58c a vocalization or firing look claimed the look point this tick
    uint8_t aim_unlocked;              // 0x58d movement_update clears it while the actor moves or faces a heading; the idle look treats the aim as free while it is set
    uint8_t look_unlocked;              // 0x58e same for the look direction
    uint8_t stationary_facing_enabled; // 0x58f
    uint8_t stationary_facing_held;   // 0x590 the actor holds the stationary facing at stationary_facing_hold
    uint8_t turn_required;            // 0x591 set by 0x415480 when body must turn to its aim and by 0x4180c0 when no
                                      //    step taken; enables oversteer hold 0x594; mirrored to control flags bit
                                      //    0x20 by 0x415480
    uint8_t unknown_592[2];           // 0x592
    float oversteer_angle[4];         // 0x594 0x4180c0 holds the oversteer angle in [0]; [1..3] (0x598) is the
                                      //    latched fixed-crouch facing 0x415480/0x416790 use when 0x590 set; needs
                                      //    split
    real_point3d desired_facing_vector; // 0x5a4 0x4180c0 (actor_move_calculate_movement) writes the desired facing
                                        //    here; 0x415480 the facing / aiming / looking trio
    real_point3d desired_aiming_vector; // 0x5b0 the desired aiming vector (0x415480)
    real_point3d desired_looking_vector; // 0x5bc the desired looking vector (0x415480)
    uint8_t avoidance_ray_clear_ticks[16]; // 0x5c8 0x4193d0 (actor_move_vector_avoidance) keeps one clear-tick byte
                                           //    per ray at [k * 2 + j]; actor_new sets all to 0xff
    int16_t avoidance_last_direction; // 0x5d8 0x4193d0 biases weights toward the best direction chosen last tick
                                      //    (0..7) and stores the new one; -1 none (actor_new)
    uint8_t unknown_5da[2];           // 0x5da
    real_vector3d avoidance_direction;// 0x5dc actor_movement_update low-pass filters the avoidance
                                      //   sampler output into this vector (0.05 or 0.3 blend)
    float avoidance_scale;            // 0x5e8 the matching low-pass filtered magnitude, snapped to
                                      //   0 below 0.001; passed to actor_movement_apply_steering
    float avoidance_emergency;        // 0x5ec raw out_scale of 0x4193d0 (CEA emergency_amount, 0..2); 0x416790 stores
                                      //    it unfiltered (0x5e8 is the filtered one); >0.9 reverses flying probe
                                      //    0x4163e0
    int16_t avoidance_turn_around_ticks; // 0x5f0 0x4193d0 counts ticks of a turn-around toward the best direction
                                         //    (held while <90), -1 when not turning
    int16_t firing_state;             // 0x5f2 0x40e7b0 burst state machine 0 idle,1 first-burst delay,2 firing,3
                                      //    burst pause,4 fire wildly (0x40a1e0 react_to_disturbance sets 4 w/
                                      //    surprise_fire_wildly_time)
    int16_t firing_state_timer;       // 0x5f4 0x40e7b0 ages it; seeded by 0x4105c0 (first_burst_delay_time), 0x40fcb0
                                      //    (burst_duration), 0x4104e0 (burst separation), react_to_disturbance (fire
                                      //    wildly time)
    int16_t firing_delay_timer;       // 0x5f6 0x40e7b0 ages it and refuses to fire while >0 unless forced; raised
                                      //    from ActorVariant surprise_delay_time (+0x8c) in
                                      //    actor_react_to_disturbance
    int16_t refire_timer;             // 0x5f8 0x40e7b0 pulls the trigger only at 0, then reloads it to
                                      //    30/(rate_of_fire*pattern scale) ticks, min 2
    int16_t line_of_fire_blocked_ticks; // 0x5fa 0x40e7b0 counts ticks a friend blocks the shot (0x42b190), shouts
                                        //    comm 0xe at 45, reset when clear
    int16_t special_fire_timer;       // 0x5fc 0x40e7b0 gates the special-fire roll while >0 and reseeds it from
                                      //    special_fire_delay (+0x15c) + random 0..1.5s
                                      //   ActorVariant.special_fire_delay
    int16_t special_fire_strafe_cooldown; // 0x5fe 0x40e7b0 sets 3 when special_fire_situation==3 (strafing) fires;
                                          //    blocks special-fire roll while >0; actor_mode_charge_enter decrements
                                          //    on stage 4
    uint8_t new_target_firing_pattern; // 0x600 0x40fcb0 sets while firing_target_ticks <
                                       //    new_target_firing_pattern_time; 0x4106b0 then picks ActorVariant
                                       //    new_target_* block (+0x100)
    uint8_t moving_firing_pattern;    // 0x601 0x40fcb0 sets when moving (or vehicle speed>1); 0x4106b0 then picks
                                      //    ActorVariant moving_* block (+0x118)
    uint8_t special_fire_overcharge;  // 0x602 0x40e7b0 sets it for special_fire_mode 1 (overcharge): holds the
                                      //    primary trigger until the next shot releases it
    uint8_t special_fire_secondary;   // 0x603 0x40fcb0 latches it from 0x604 per burst; 0x40e7b0 then fires the
                                      //    secondary trigger (flag 0x1000) and uses trigger 1 for aiming/lead
    uint8_t special_fire_secondary_pending; // 0x604 0x40e7b0 sets it for special_fire_mode 2 (secondary trigger);
                                            //    0x40fcb0 validates (special_fire_situation) and moves it to 0x603
    uint8_t unknown_605[3];           // 0x605
    float maximum_firing_distance;    // 0x608 ActorVariant maximum_firing_distance (+0x74), copied by 0x40e7b0 when
                                      //    the threat weapon is usable; compared with the target distance
    int16_t firing_target_type;       // 0x60c 0x40e7b0: 0 none, 1 prop (0x610 = prop handle from target_unit_index),
                                      //    2 point (0x610 = 0x460 point); read by many grenade/aim fns
    uint8_t unknown_60e[2];           // 0x60e
    union {
        datum_index firing_target_prop_index; // 0x610 0x40e7b0 copies the target prop (type 1) here; prop users
                                              //    0x40f700/0x4105c0/0x40fcb0/relationship_think
        real_point3d firing_target_free_point; // 0x610 the forced aim point (type 2)
    };
    int32_t firing_target_ticks;      // 0x61c 0x40e7b0 counts ticks on the same firing target (reset on change);
                                      //    0x40fcb0 compares with new_target_firing_pattern_time; %10 reachability
                                      //    recheck
    uint8_t firing_line_clear;        // 0x620 the obstruction test of the firing target is 0 or 1
    uint8_t firing_target_in_water;   // 0x621 prop.in_water of the firing target
    uint8_t use_high_arc;             // 0x622 the target is beyond the variant's arc range; passed to weapon_trigger_get_aiming_vector
    uint8_t fire_blindly;             // 0x623 the actor fires without a clear line (forced aim and a blind-fire range)
    uint8_t firing_target_hidden;     // 0x624 the firing target's cluster is not visible to the local player
    uint8_t unknown_625;              // 0x625
    int16_t firing_target_obstruction; // 0x626 obstruction of the firing target (0 and 1 are clear)
    uint8_t target_in_firing_range;   // 0x628 0x40e7b0 clears each tick, sets once all fire gates pass (range < max
                                      //    firing distance 0x608); look decode code 2 then uses target_aim_vector
                                      //    0x63c
    uint8_t unknown_629[3];           // 0x629
    real_point3d firing_target_point; // 0x62c where the actor aims: the target prop's position or the firing target
                                      //    point; the aim error setup 0x40fcb0 starts from it
    float firing_target_distance;     // 0x638 distance from the aim origin to firing_target_point; bounds the error
                                      //    radius and is compared with the weapon's minimum range
    real_vector3d target_aim_vector;   // 0x63c 0x40e7b0 weapon_trigger_get_aiming_vector(origin 0x120 -> target 0x62c)
                                      //    out vector; 0x4281f0/look decode read the vector
    float target_aim_range;           // 0x648 range out of the same call
    real_point3d aim_target_point;    // 0x64c firing_target_point after the bombardment scatter
    real_point3d firing_aim_point;    // 0x658 0x40e7b0 = aim target (0x64c) plus target_tracking drift and
                                      //    target_leading lead; + accumulated error gives 0x67c
    real_vector3d aim_wander_offset;  // 0x664 aim error offset the aim error setup 0x40fcb0 rolls for this burst
    real_vector3d aim_recoil_per_tick;// 0x670 per tick share of the recoil error (divided by the firing state timer)
    real_vector3d grenade_aim_direction;// 0x67c written by 0x40f7e0 and 0x40fcb0
    uint8_t firing_vector_ballistic;  // 0x688 0x40e7b0 sets = !used_straight_line from 0x4c2b40; 0x40f7e0 (CEA
                                      //    actor_aim_projectile) then uses 0x68c instead of aiming at 0x67c
    uint8_t unknown_689[3];           // 0x689
    real_vector3d firing_vector;      // 0x68c 0x40e7b0 out_direction of weapon_trigger_get_aiming_vector from firing
                                      //    origin to final aim point; 0x40f7e0 and look-decode code 2 (firing_state
                                      //    2) read it
    float projectile_error;           // 0x698 0x40fcb0 = projectile_error * pattern/difficulty scale
                                      //    (+special_projectile_error); 0x40f7e0 returns it through CEA
                                      //    actor_aim_projectile error_reference
    float perception_scale;           // 0x69c 0x42aa90 scales hearing and awareness ranges by this
    uint8_t grenade_throw_pending;    // 0x6a0 0x40dc30 sets when it decides to throw; 0x40db00 clears once facing
                                      //    within 30deg of grenade_impact_point and starts throw (0x45c); 0x416790
                                      //    faces impact point
    uint8_t grenade_high_arc[3];      // 0x6a1 byte 0 passed as use_high_arc to projectile_get_aiming_vector in
                                      //    0x410780; cleared by 0x411180 on commit; no writer of 1 found
    uint32_t last_grenade_check_time; // 0x6a4 0x40dc30 stores game_time of last roll; no new roll until
                                      //    grenade_check_time*30 ticks later; actor_new sets -1
    real_point3d grenade_impact_point;// 0x6a8 0x410710 records a validated landing point
    uint32_t grenade_target_prop_index; // 0x6b4 0x411180 commits the target prop of the toss; 0x410a60 reads prop
                                        //    state/position; replace_object_reference patches it; actor_new -1
    datum_index grenade_exclude_object_index; // 0x6b8 0x411180 stores the object excluded from the arc check; 0x410780
                                             //    passes it to 0x42b5d0 path check
    real_vector3d grenade_throw_direction;// 0x6bc direction of the planned grenade throw, written by 0x410a60 and
                                      //    the grenade arc solver
    float grenade_throw_speed;        // 0x6c8 launch speed of the planned throw
    uint8_t grenade_eligible;         // 0x6cc 0x42f260 caches the eligibility test here
    uint8_t unknown_6cd;              // 0x6cd
    int16_t grenade_recheck_ticks;    // 0x6ce actor_new sets 30
    uint32_t control_flags;           // 0x6d0 the unit control flags word staged for unit_apply_control_block (crouch
                                      //    1, 0x20, primary 0x800, secondary 0x1000, grenade 0x2000)
    int16_t persistent_control_ticks; // 0x6d4 0x42a640 copies it (when >0) to unit+0x210 persistent_control_ticks
                                      //    with 0x6d8 -> unit+0x214; int16 source, int32 dest
    uint8_t unknown_6d6[2];           // 0x6d6
    uint32_t persistent_control_flags; // 0x6d8 0x42a640 copies it to unit+0x214 persistent_control_flags together
                                       //    with 0x6d4
    int16_t control_animation_mode;   // 0x6dc 0x416790 picks 0..4 (awareness / 0x428 / 0x429 / order 0x42c); 0x42a640
                                      //    maps it via table 0x6558b8 to unit_control animation_state; 1 = no free
                                      //    look, 4 = always step
    uint8_t unknown_6de[2];           // 0x6de
    real_vector3d throttle;           // 0x6e0 the control throttle: 0x4180c0's desired throttle output, copied into
                                      //    the unit control block by 0x42a640
    int16_t control_animation_impulse; // 0x6ec 0x416790 copies secondary_action record 0x418..0x423 (CEA animation
                                       //    impulse+alignment) here; 0x42a640 starts it on the unit (0x569530) with
                                       //    alignment 0x6f0
    uint8_t unknown_6ee[14];          // 0x6ee
    real_vector3d snapshot_facing;    // 0x6fc copy of facing taken by actor_snapshot_orientation
    real_vector3d aiming_vector_snapshot;// 0x708 copy of unit_aiming_vector
    real_vector3d looking_vector_snapshot;// 0x714 copy of unit_looking_vector
    datum_index override_target;      // 0x720 0x42a5e0 writes it together with flags bit 0x800
} actor;                // size 0x724
// global 0x00880360: data_array *actor_data          element size 0x724, capacity 0x100

// ---------------------------------------------------------------------------
// swarm
// ---------------------------------------------------------------------------
typedef struct swarm {
    int16_t identifier;               // 0x00 datum_header
    int16_t component_count;          // 0x02 0..16
    datum_index actor_index;          // 0x04 the actor that owns this swarm
    int16_t component_pick_delay;     // 0x08 ticks until the infection swarm update picks the next component; counted down by it
    uint8_t unknown_0a[2];            // 0x0a
    real_point3d aggregate_position;  // 0x0c the mean of the swarm_component.position of every
                                      //      component, recomputed each tick by
                                      //      actor_refresh_combat_context @0x4297a0 (which
                                      //      seeds all THREE floats from the vector at
                                      //      0x006966f8, sums component+0x04/+0x08/+0x0c into
                                      //      them and divides by component_count)
    datum_index unit_index[16];       // 0x18 one unit object per component
    datum_index component_index[16];  // 0x58 the matching swarm_component datums
} swarm;                // size 0x98
// global 0x0088035c: data_array *swarm_data          element size 0x98, capacity 0x20

// The per-component state the infection form swarm update keeps in swarm_component + 0x18; the same bytes hold the command list
// execution record (actor_squad_action_state) while the swarm runs a command list.
typedef struct swarm_infection_state {
    uint8_t parent_ticks;               // 0x00 ticks the form has ridden its parent unit (saturates at 255), 0 while it is free
    uint8_t free_ticks;                 // 0x01 ticks the form has been free and not airborne (saturates at 255); the leap needs 45
    uint8_t detach_delay;               // 0x02 set to 45 when the form is made to leave its parent, counted down while free
    uint8_t unknown_03;                 // 0x03
    uint8_t turn_wait_ticks;            // 0x04 counted down to the next wander turn
    uint8_t turn_ticks;                 // 0x05 ticks left in the current wander turn
    uint8_t unknown_06[2];              // 0x06
    real_vector3d heading;              // 0x08 wander heading (swarm_component + 0x20)
    float turn_rate;                    // 0x14 current turn rate of the wander heading (swarm_component + 0x2c)
    uint8_t unknown_18[0x10];           // 0x18
} swarm_infection_state;                // size 0x28
typedef char swarm_infection_state_size[sizeof(swarm_infection_state) == 0x28 ? 1 : -1];

typedef struct swarm_component {
    int16_t identifier;               // 0x00 datum_header
    uint16_t flags;                   // 0x02 swarm_component_flag; bit 3 (active) read by ai_object_list_max_flee_grade
    real_point3d position;            // 0x04 object_get_position of the component unit
    datum_index marker_index;         // 0x10 object+0x4d8 when object+0xb4 is 0, otherwise none
    uint32_t leap_target_index;       // 0x14 0x14 -1 at swarm_add_component / actor_create_swarm;
                                      //    actor_compute_swarm_avoidance_offset uses it as 'target' for the leap
                                      //    solve when flag bit 0 is set; actor_replace_object_reference remaps it
                                      //    like the other object references
    union {
        swarm_infection_state infection;  // 0x18 infection form wander / leap state
        struct {
            uint8_t unknown_18[4];            // 0x18
            actor_squad_action_state action;  // 0x1c the command list execution record of this member (zeroed by command_list_reset_record)
        };
    };
} swarm_component;      // size 0x40
// global 0x00880358: data_array *swarm_component_data  element size 0x40, capacity 0x100

// ---------------------------------------------------------------------------
// prop -- one record per object an actor currently perceives
// ---------------------------------------------------------------------------
// Every actor keeps a singly linked list of these at actor.first_prop. 0x43e270 finds or
// allocates one, 0x43e640 initializes it from the tracked object and links it in, and
// 0x43ea20 unlinks it. The repurpose path in 0x43e270 zeroes 0x4e dwords while preserving
// the datum identifier, which is what fixes the size at 0x138.
typedef struct prop {
    int16_t identifier;               // 0x00 datum_header
    uint8_t unknown_02[2];            // 0x02
    datum_index actor_index;          // 0x04 the actor whose prop list this is on
    datum_index next_in_actor;        // 0x08 next prop in actor.first_prop list
    datum_index pair_index;           // 0x0c the paired prop allocated by 0x43e910 / 0x43e980
    int16_t actor_type;               // 0x10 the owning actor type, or 6 for a swarm prop, or -1
    int16_t team;                     // 0x12 the tracked object's owner team (0x43e706)
    uint8_t swarm_owned;              // 0x14 set when the object has a swarm actor (object+0x1f8), not a parent
    uint8_t unknown_15;               // 0x15
    int16_t unknown_16;               // 0x16
    datum_index object_index;         // 0x18 the tracked object
    datum_index owner_actor_index;    // 0x1c the actor that currently owns the tracked object, or none
    float danger_radius;              // 0x20 Unit.ai_danger_radius (object tag +0x284), copied by 0x43e640; tracking
                                      //    passes it to actor_danger_register_point
    int16_t state;                    // 0x24 0..3 perception states, 4 / 5 the uninspected / inspected orphan (the
                                      //    CEA _prop_state_*), 6 parented
    int16_t reaction_timer;           // 0x26 actor_target_relationship_think counts it up per tick (>>3 for non-enemies, >>1
                                      //    when distance_class > 2) against the actor reaction threshold, then zeroes it
    int32_t swarm_reassign_time;      // 0x28 game time of the last swarm reassignment: stamped at init for a
                                      //    swarm-owned object, 0x41c4b0 reassigns at most every 90 ticks
    float acknowledge_progress;       // 0x2c a float accumulator in this slot: state 1 adds the inverse (non-combat /
                                      //    guard / combat) perception time per tick, >= 1.0 acknowledges (state 3)
    int16_t perception_level;         // 0x30 max of the visual / auditory / 0x36 channels (0..3); drives states 0 ->
                                      //    1 -> 3 and 3 -> 2
    int16_t visual_perception;        // 0x32 0..3 from the perception range / field-of-view test 0x41bb30 against
                                      //    head_position; >= 2 reads as seen
    int16_t auditory_perception;      // 0x34 actor_target_hearing_check (0x41c030) 0/2/3, or 3 for stimulus types 1/2
    int16_t ambient_perception;       // 0x36 third perception channel: 3 while the prop has stimulus type 0, raised to 1 by a lit
                                      //    flashlight (aiming/distance class below 3, obstruction 0/1)
    int16_t obstruction;              // 0x38 actor_evaluate_engagement_reachability (0x42b270) from the actor to
                                      //    head_position: 0 clear, 1 partly blocked, 2..4 blocked
    int16_t orphan_timer;             // 0x3a 900 when actor_copy_prop_and_reset makes the orphan copy (state 4);
                                      //    counts down by usage, the orphan is deleted below 0
    int16_t inspection_ticks;         // 0x3c ticks the orphan has been looked at; at 45 (300 when nearly dead) state
                                      //    4 becomes 5 (inspected orphan)
    uint8_t unknown_3e[2];            // 0x3e
    real_vector3d perceived_to_known_delta; // 0x40 actor_copy_prop_and_reset: last_known_position - last_perceived_position;
                                      //    actor_find_best_firing_position reads it as the query target_vault_point
    int16_t lost_timer;               // 0x4c 10 (60 when seen) on entering state 2; while set and near
                                      //    last_perceived_position the prop stays in state 2
    uint8_t dead_confirmed;           // 0x4e 0x41c4b0: an orphan whose object is dead, not feigning, unperceived and
                                      //    still; later non-forced refreshes skip it
    uint8_t unknown_4f;               // 0x4f
    float desirability;               // 0x50 actor_rate_potential_target writes the score here
    float interest;                   // 0x54 actor_compute_target_priority_weight (0x414590), the CEA
                                      //    actor_look_compute_prop_interest; the idle look selector (0x414a90) scores
                                      //    on it
    float interest_satisfied;         // 0x58 raised to interest when the prop is looked at or vocalized about; 0
                                      //    while not visually perceived
    int32_t last_attention_time;      // 0x5c game time of the last look / vocalization at it (-1 never); gates
                                      //    re-vocalizing for 600 ticks
    uint8_t enemy;                    // 0x60 teams_are_enemies(object team, actor team) (0x43e640); the CEA name
    uint8_t allegiance;               // 0x61 team_pair_flag_test(actor team, object team), the bitmap hs
                                      //    ai_allegiance_broken tests; recomputed by
                                      //    ai_recompute_all_relationship_flags
    uint8_t team_pair_status;         // 0x62 0x62 actor_init_prop_from_object stores
                                      //    team_pair_override_get_flag(actor.team, prop.team), the
                                      //    team_pair_override.status byte (0x45be00); the relationship think gates
                                      //    the team broadcast on it, and the
                                      //    ai_mark_recognized/ai_notify_actors_of_encounter_state_change loops set 0
                                      //    / 1 next to allegiance
    uint8_t in_use;                   // 0x63 set while the actor references the prop (target, vocalization / search
                                      //    slots, ...), mirrored to the pair; an in-use prop is never dropped
    uint8_t combat_dirty;             // 0x64 actor_target_reset_combat_flags sets it
    uint8_t unknown_65;               // 0x65
    int16_t stimulus_type;            // 0x66 highest stimulus heard (0..3; weapon fire 1, a unit scream 2), -1 none;
                                      //    1 sets shooting
    int16_t stimulus_timer;           // 0x68 30 (150 for type 3) with stimulus_type; at 0 stimulus_type goes back to
                                      //    -1
    int16_t retain_timer;             // 0x6a 30 on a newly created shared prop; while > 0 the prop is never dropped
    int16_t seen_state;               // 0x6c actor_target_reset_seen_flags sets 0xffff
    int16_t unknown_6e;               // 0x6e
    float unknown_70;                 // 0x70 0x43e640 zeroes it
    uint8_t seen;                     // 0x74 actor_target_reset_seen_flags clears it
    uint8_t unknown_75;               // 0x75
    int16_t dead_ticks;               // 0x76 0 while alive, then +1 per tick (1000 for an object already dead at
                                      //    init); the CEA actor_perception_desire_prop dead_ticks
    int16_t sighted_ticks;            // 0x78 consecutive ticks with visual_perception >= 2, up to 0x7fff (a short:
                                      //    0x41abd0 reads and writes it 16-bit; it was declared float)
    uint8_t pad_7a[2];                // 0x7a
    int32_t last_perceived_time;      // 0x7c game time perception_level was last nonzero, -1 never
    real_point3d last_perceived_position; // 0x80 last_known_position when perception_level was last nonzero
    int32_t last_seen_time;           // 0x8c game time visual_perception was last nonzero, -1 never; copied to the
                                      //    actor at 0x26c
    real_point3d last_seen_position;  // 0x90 head_position when last seen
    int16_t engaged_ticks;            // 0x9c 1 when actor_target_mark_engaged (0x41fa80) marks it, 0 when cleared,
                                      //    counts up to 0x7fff
    uint8_t unknown_9e[2];            // 0x9e
    int32_t last_engaged_time;        // 0xa0 game time of the last engagement mark, -1 none; unmarked 150 ticks later
    uint8_t engaged;                  // 0xa4 0x41fa80 marks the target actively engaged
    uint8_t unknown_a5;               // 0xa5
    int16_t friends_killed;           // 0xa6 allies this target killed (0x423220); compared with
                                      //    Actor.friends_killed_trigger
    int16_t friends_killed_timer;     // 0xa8 750 per kill; each expiry takes one off friends_killed
    int16_t shots_fired;              // 0xaa actor_target_reset_shot_counters zeroes 0xaa, 0xac and 0xae
    int16_t shots_hit;                // 0xac
    int16_t danger_trigger_ticks;     // 0xae ticks of being shot at after which the prop raises the danger priority to 7 (drawn from the actor definition danger_trigger_time)
    int16_t information_age;          // 0xb0 ticks since the information about it was last refreshed (-1 none); past
                                      //    59 has_current_information clears
    uint8_t unknown_b2[2];            // 0xb2
    int32_t information_source_actor; // 0xb4 the friend actor that handed the prop over (0x41f7d0), -1 none
    uint8_t has_current_information;  // 0xb8 set on fresh information (from a friend, or seen); expires after 60
                                      //    ticks
    uint8_t noticed_a;                // 0xb9 set by 0x41fb00 (unit+0xb9), cleared by actor_target_reset_combat_flags
    uint8_t noticed_b;                // 0xba set by 0x41fb60 (unit+0xba)
    uint8_t noticed_c;                // 0xbb set by 0x41fbc0
    real_point3d last_known_position; // 0xbc
    real_point3d center_of_mass;      // 0xc8 the "body" marker position (0x41c4b0); the CEA name
    real_point3d velocity;            // 0xd4 the object velocity (0x41c4b0); zeroed for an orphan
    real_point3d direction;           // 0xe0 normalized last_known_position - the actor's sense position; its length
                                      //    goes to distance
    int32_t pathfinding_surface_index; // 0xec the CEA name
                                      //   and actor_movement_action_resolve @0x41a460 hands it to
                                      //   the pathfinder as the destination surface index. The
                                      //   header used to frame 0xec..0xf7 as one real_point3d,
                                      //   which is one dword low: 0x41a460 reads the point at
                                      //   0xf0/0xf4/0xf8 and the surface index at 0xec.
    real_point3d pathfinding_point;   // 0xf0 the CEA name (actor_move_to_prop)
                                      //   type-5 movement action; the flying path uses aim_offset
    bsp_leaf_reference location;      // 0xfc the root object's bsp leaf and the BSP cluster the tracked object was last seen in (-1 when none);
                                      //    the location for water / weather and hearing
    real_point3d head_position;       // 0x104 the "head" marker's world position; the perception and LOS target point
    int32_t relationship_object_index;// 0x110 actor_target_get_relationship_object caches it lazily
    datum_index parent_object_index;  // 0x114 actor_target_data_refresh stores -1, then the parent object
                                      //    handle when the tracked unit's parent is a non-vehicle unit (biped);
                                      //    actor_update_firing_state reads it as the 'exclude' object of the aim ray
    uint8_t in_water;                 // 0x118 scenario_location_get_water_and_weather at the body marker
    uint8_t unknown_119[3];           // 0x119
    float distance;                   // 0x11c the ascending sort key of ai_target_distance_qsort_compare
    uint8_t perception_range_class;   // 0x120 always 2; the range class (0.4 / 0.6 / 0.8 / 1.0 x vision range) the
                                      //    perception test uses
    uint8_t distance_class;           // 0x121 distance bucket: < 1, < 6, < 10, < 30, else 4
    int8_t aiming_at_actor_class;     // 0x122 how directly the target unit aims back at the actor: 0 dead-on .. 4
                                      //    facing away
    uint8_t speed_class;              // 0x123 the object's speed bucket 0..3
    uint8_t closing_speed_class;      // 0x124 relative closing speed bucket 0..6 (3 static, 6 closing fast)
    uint8_t child_unit_count;         // 0x125 children that are bipeds or vehicles
    uint8_t just_created;             // 0x126 set on a new shared prop; tracking runs its drop test once and clears
                                      //    it
    uint8_t dead;                     // 0x127 the object's vitality bit 2 (dead), read by init and tracking; the CEA
                                      //    name
    uint8_t dead_not_feigning;        // 0x128 dead and the unit's feign-death countdown (unit+0x420) is 0
    uint8_t just_died;                // 0x129 dead now but not last tick; relationship_think runs the ally-death
                                      //    panic scan and clears it
    uint8_t just_sighted;             // 0x12a visual_perception rose from 0; relationship_think notifies the
                                      //    engagement and clears it
    uint8_t owner_not_in_combat;      // 0x12b the owning actor's awareness_level < 3
    uint8_t owner_stalled;            // 0x12c 0x12c actor_target_update_tracking_speed stores the local owner_stalled
                                      //    (owner awareness_level == 3 and minimum_combat_status < combat_status, 0
                                      //    with no owner) here; read by actor_scale_value_by_ally_exposure (counts
                                      //    exposed allies) and actor_target_relationship_think (broadcast 0xf while
                                      //    awareness < 3)
    uint8_t owner_burst_length_exceeded;  // 0x12d actor_check_burst_length_exceeded of the owning actor, stored by the prop update
    uint8_t is_parented;              // 0x12e 0x43e640 sets it when the tracked object has a parent (object+0x30)
    uint8_t shooting;                 // 0x12f stimulus_type 1 (weapon fire)
    uint8_t flying;                   // 0x130 Biped.flags bit 2 (flying); 0 for non-bipeds
    uint8_t camouflaged;              // 0x131 unit active camouflage (unit+0x37c) above 0.5
    uint8_t flashlight_on;            // 0x132 unit flags bit 19
    uint8_t disregarded;              // 0x133 unit flags bit 10, the bit hs ai_disregard sets; perception is zeroed
                                      //    while set
    uint8_t preferred_target;         // 0x134 unit flags bit 11, the bit hs ai_prefer_target sets
    uint8_t is_vehicle_gunner;        // 0x135 the parent vehicle's gunner is this object
    uint8_t is_vehicle_driver;        // 0x136 the parent vehicle's driver is this object and it causes collision
                                      //    damage
    uint8_t unknown_137;              // 0x137
} prop;                 // size 0x138
// global 0x008802c0: data_array *prop_data           element size 0x138, capacity 0x300

// ---------------------------------------------------------------------------
// encounter, and its per-squad and per-platoon sub-records
// ---------------------------------------------------------------------------
// One encounter datum per ScenarioEncounter. encounter_new @0x437060 (currently named
// squad_create) builds it, and encounters_reset @0x435cb0 calls it once per scenario
// encounter block while threading two running index counters through it, which is how the
// first_squad / first_platoon runs are assigned.
typedef struct encounter {
    int16_t identifier;               // 0x00 datum_header
    int16_t team;                     // 0x02 ScenarioEncounter.team_index
    int16_t first_squad;              // 0x04 index of this encounter first encounter_squad_state
    int16_t squad_count;              // 0x06 ScenarioEncounter.squads.count
    int16_t first_platoon;            // 0x08 index of this encounter first encounter_platoon_state
    int16_t platoon_count;            // 0x0a ScenarioEncounter.platoons.count
    uint8_t force_active;             // 0x0c hs ai_force_active (0x47df00); encounters_update_activation keeps the
                                      //    encounter active while set
    uint8_t units_active;             // 0x0d encounter_add_actor calls actor_set_units_active when set
    int16_t activation_delay;         // 0x0e ticks remaining before encounters_update_activation re-evaluates this encounter; encounter_add_actor sets 0x96
    int32_t activation_tick;          // 0x10 encounter_new sets -1; encounter_activate stamps the current game tick
    datum_index first_actor;          // 0x14 head of the member list, chained through actor.next_in_encounter
    int16_t member_count;             // 0x18 encounter_add_actor increments, squad_remove_actor decrements
    int16_t pre_combat_living_count;  // 0x1a living_count copied while idle / stood down (0x437940); post-combat
                                      //    lines compare against it
    int16_t live_count;               // 0x1c only actors with counts_toward_encounter set are counted
    uint8_t squads_carried_over;      // 0x1e ai_squads_merge: 1 on the target encounter, 0 on the source, once
                                      //    the squad references were remapped; also gates skipping empty squads
    uint8_t pad_1f;                   // 0x1f
    int16_t activation_link_count;    // 0x20 encounters linked by hs ai_link_activation (0x437820), at most 3, at
                                      //    0x22
    int16_t activation_link[3];       // 0x22 encounter indices linked by ai_link_activation; encounters_update_activation
                                      //    keeps this encounter active while any of them is pending (count at 0x20)
    uint8_t dirty;                    // 0x28 set by every member add / remove; 0x435f00 re-runs morale for dirty encounters
    uint8_t unknown_29;               // 0x29
    int16_t living_count;             // 0x2a weighted member count (1 per unit, cluster_count per swarm); hs
                                      //    ai_living_count (ai_reference_get_stat_pair 0x432f90)
    int16_t swarm_count;              // 0x2c weighted swarm member count; hs ai_swarm_count, and ai_nonswarm_count =
                                      //    living - swarm
    int16_t combat_count;             // 0x2e weighted members at awareness_level 3 with combat_status >
                                      //    minimum_combat_status; starts the squad timers
    int16_t engaged_count;            // 0x30 weighted members with combat_status > 6
    uint8_t unknown_32[2];            // 0x32
    float average_vitality;           // 0x34 encounter_recompute_morale sums one vitality sample per live member here and then divides by member_count
    datum_index first_pursuit;        // 0x38 head of the ai_pursuit ("recently seen object") list
    uint8_t respawn_enabled;          // 0x3c ScenarioEncounter flags bit 1; hs ai_set_respawn (0x47d3b0);
                                      //    reinforcements stop while clear
    uint8_t unknown_3d;               // 0x3d
    int16_t respawn_delay_ticks;      // 0x3e random(respawn_delay) * 30 per reinforcement; counted down by 15 per
                                      //    update, spawns at 0
    uint8_t blind;                    // 0x40 ScenarioEncounter flags bit 2 (initially blind); hs ai_set_blind
                                      //    (0x47d4b0)
    uint8_t deaf;                     // 0x41 ScenarioEncounter flags bit 3 (initially deaf); hs ai_set_deaf
                                      //    (0x47d440)
    uint8_t stood_down;               // 0x42 1 at creation and after encounter_release_stale_props (the CEA
                                      //    encounter_stand_down); cleared by combat
    uint8_t ever_had_target;          // 0x43 set once any member has a target; never cleared
    uint8_t has_live_target;          // 0x44 a member's target is engaged or not dead (0x437940)
    uint8_t engaged;                  // 0x45 a member with a target has combat_status >= 7
    uint8_t unknown_46;               // 0x46 squad_create zeroes it
    uint8_t post_combat;              // 0x47 set by encounter_choose_vocalizations (0x438580, the CEA
                                      //    encounter_post_combat); cleared when combat resumes
    uint8_t post_combat_quiet;        // 0x48 no member still has a post-combat line pending
    uint8_t unknown_49;               // 0x49
    int16_t post_combat_timer;        // 0x4a 120 at post_combat, -15 per update; the encounter stands down at 0
    int16_t enemy_death_count;        // 0x4c enemies of this encounter killed during the fight (0x435f90, the CEA
                                      //    encounters_unit_died); reset at post combat / stand down
    uint8_t unknown_4e[2];            // 0x4e
    datum_index ticks_since_engaged;  // 0x50 +15 per update while not engaged, 0 while engaged, -1 never (ticks, not
                                      //    a datum)
    datum_index ticks_since_live_target; // 0x54 the same keyed on has_live_target
    int32_t last_idle_time;           // 0x58 game time stamped while idle; only bodies that died after it count as
                                      //    this fight's (prop scans)
    datum_index last_grenade_time;    // 0x5c game time a member last committed a grenade throw; the encounter grenade
                                      //    timeout counts from it
    uint8_t playfight;                // 0x60 hs ai_playfight (0x47e070); copied to the members
    uint8_t unknown_61;               // 0x61
    int16_t follow_target_type;       // 0x62 0 none, 1 players, 2 unit, 3 ai (hs ai_follow_target_*)
    int32_t follow_target;            // 0x64 the unit index or packed ai reference followed; -1 when the unit is gone
    float follow_distance;            // 0x68 hs ai_follow_distance (0x47e590) stores a real here; encounter_update_follow
                                      //   uses 2.0 when it is <= 0 (0x4394a0 compares it as a float)
} encounter;            // size 0x6c
// global 0x008802c8: data_array *encounter_data      element size 0x6c, capacity 0x80

// One record per ScenarioSquad of the owning encounter, addressed as
// encounter_squad_states[encounter.first_squad + actor.squad_index].
typedef struct encounter_squad_state {
    // The two parallel starting-location bit masks, one bit per
    // ScenarioSquad.starting_locations entry.
    // encounter_squad_reset_starting_location_mask @0x436f90 fills +0x00 from the tag
    // (bit 0 of ScenarioActorStartingLocation.flags) and sets every bit of +0x04, and
    // squad_pick_random_starting_location @0x437220 consumes +0x00 first, then +0x04,
    // refilling +0x04 when it runs out. Both are addressed as mask[index >> 5], so the
    // squad is limited to 32 starting locations in practice.
    uint32_t starting_location_mask;  // 0x00 locations this squad is allowed to use
    uint32_t starting_location_free;  // 0x04 locations not yet handed out this round
    float major_upgrade_error;        // 0x08 error diffusion of the major-variant upgrade roll (ai_drift_zone_bias
                                      //    0x42a9d0)
    int16_t respawn_budget;           // 0x0c ScenarioSquad.respawn_total (999 when that is 0), only set when the squad has a respawn range; the reinforcement spawner decrements it
    int16_t respawn_delay_ticks;      // 0x0e random(ScenarioSquad respawn_delay) * 30, counted down by 15
    uint8_t automatic_migration;      // 0x10 ScenarioSquad flags bit 5; hs ai_automatic_migration_target
    uint8_t timer_started;            // 0x11 hs ai_timer_start, or start_timer_immediately / encounter combat;
                                      //    squad_delay_ticks counts down once set
    int16_t squad_delay_ticks;        // 0x12 ftol(ScenarioSquad.squad_delay_time * 30), or 999 when ScenarioSquad.flags bit 3 is set
    uint8_t dormancy_disabled;        // 0x14 hs ai_allow_dormant stores !allow; members are kept awake
    uint8_t unknown_15;               // 0x15
    int16_t member_count;             // 0x16 encounter_add_actor increments, squad_remove_actor decrements
    int16_t living_count;             // 0x18 as encounter.living_count, per squad
    int16_t swarm_count;              // 0x1a as encounter.swarm_count, per squad
    float average_vitality;           // 0x1c encounter_recompute_morale @0x437940 sums one vitality sample per live member here and then divides by member_count
} encounter_squad_state; // size 0x20
// global 0x008802cc: encounter_squad_state *encounter_squad_states  0x8000 bytes, 0x400 records

// One record per ScenarioPlatoon of the owning encounter, addressed as
// encounter_platoon_states[encounter.first_platoon + actor.platoon_index].
typedef struct encounter_platoon_state {
    uint8_t defending;                // 0x00 ScenarioPlatoon flags bit 2 (start in defending state); hs ai_defend
                                      //    sets it, ai_attack clears it
    uint8_t maneuvering;              // 0x01 hs ai_retreat / ai_maneuver (0x4332e0), or latched from maneuver_when
                                      //    (0x4393b0); members move to ScenarioSquad.maneuver_to_squad while set
    uint8_t maneuver_disabled;        // 0x02 hs ai_maneuver_enable (0x433350) stores !enable
    uint8_t pad_03;                   // 0x03
    int16_t member_count;             // 0x04 encounter_add_actor increments, squad_remove_actor decrements
    int16_t living_count;             // 0x06 as encounter.living_count, per platoon
    int16_t swarm_count;              // 0x08 as encounter.swarm_count, per platoon
    int16_t unknown_0a;               // 0x0a
    float average_vitality;           // 0x0c same running sum as encounter_squad_state.average_vitality, divided by member_count at 0x04
} encounter_platoon_state; // size 0x10
// global 0x008802c4: encounter_platoon_state *encounter_platoon_states  0x1000 bytes, 0x100 records

// The iterator the encounter sweeps use: a types/memory.h data_iterator over encounter_data
// with a trailing "skip encounters whose units_active is clear" flag. Recovered from the
// disassembly of encounters_recompute_dirty @0x435f00 (objdump -d -M intel
// --start-address=0x435f00 --stop-address=0x435f85 bin/halo.exe): the 0x18-byte frame is
// seeded with encounter_data, a word 0, -1, data XOR 'iter' and a byte 0, and
// data_iterator_next is called with EDI pointing at its first field.
typedef struct encounter_iterator {
    data_array *data;            // 0x00 encounter_data
    int16_t next_index;          // 0x04 written as an int16, read as an int32 by data_iterator_next
    uint8_t pad_06[2];           // 0x06
    datum_index index;           // 0x08 handle of the encounter the last _next returned
    uint32_t signature;          // 0x0c data XOR 0x69746572 ('iter'), the data_iterator self-check
    datum_index encounter_index; // 0x10 the loop body copy of index
    uint8_t active_only;         // 0x14 skip encounters whose units_active is clear
    uint8_t pad_15[3];           // 0x15
} encounter_iterator;            // size 0x18

// One candidate slot of the four-bucket vocalization table encounter_choose_vocalizations
// @0x438580 builds. Each bucket is two of these (a best and a runner-up);
// ai_insert_scored_candidate_pair @0x4383f0 keeps each pair sorted by descending score and
// ai_pick_weighted_candidate @0x438480 draws one bucket in proportion to its best score.
typedef struct ai_scored_candidate {
    datum_index handle;   // 0x00 the actor the candidate belongs to; none marks an empty slot
    float score;          // 0x04 the weight, always positive for a live slot
    datum_index payload;  // 0x08 the prop the score came from, or none
    datum_index key;      // 0x0c the object that prop tracks, or none
} ai_scored_candidate;    // size 0x10

// One row of the 32-entry table at ai_globals + 0x3b8 that ai_object_attention_find_or_create
// @0x435900 hands out and ai_object_attention_remove @0x435990 compacts, keyed by an object
// handle. The count lives in ai_globals.unknown_3b6 and the table runs 0x3b8..0x8b7, which is
// exactly up to ai_globals.vehicle_entry_count at 0x8b8.
// ai_globals.object_attention_table is this table (the earlier overlapping communication_hold_tick/3fa were unit fields).
typedef struct ai_object_attention_record {
    datum_index object_index;  // 0x00 the key; the search compares the whole 32-bit handle
    float weight;              // 0x04 seeded to 8.0 on creation
    uint8_t unknown_08[0x20];  // 0x08 zeroed on creation, never read inside this module
} ai_object_attention_record;  // size 0x28

// The view the actor vehicle search takes of an object attention record: a vehicle offered to actors, the radius it
// is offered within (FLT_MAX for unlimited), the team and actor type bit masks that may take it and up to six packed
// ai references (encounter / squad / platoon) that may take it.
typedef struct ai_vehicle_offer {
    datum_index vehicle;       // 0x00
    float radius;              // 0x04
    int16_t team_mask;         // 0x08 bit per team, 0 or less for any
    int16_t type_mask;         // 0x0a bit per actor type, 0 or less for any
    int16_t filter_count;      // 0x0c valid entries of filters
    uint8_t unknown_0e[2];     // 0x0e
    uint32_t filters[6];       // 0x10 packed ai references
} ai_vehicle_offer;            // size 0x28

// The "ai pursuit" datum: a per-encounter, per-type ring of recently seen objects.
typedef struct ai_pursuit {
    int16_t identifier;               // 0x00 datum_header
    int16_t type;                     // 0x02 the per-type key squad_recent_object_get_or_create matches on
    int32_t last_tick;                // 0x04 the tick of the last sighting, or -1 while empty
    int16_t count;                    // 0x08 total sightings recorded
    int16_t cursor;                   // 0x0a next slot to overwrite, modulo 6
    datum_index object_index[6];      // 0x0c
    datum_index next;                 // 0x24 next pursuit record in the encounter list
} ai_pursuit;           // size 0x28
// global 0x008802d0: data_array *ai_pursuit_data     element size 0x28, capacity 0x100

// ---------------------------------------------------------------------------
// ai conversation
// ---------------------------------------------------------------------------
// A running instance of one Scenario.ai_conversations entry. The data_array is built
// inline by ai_communication_initialize with the name "ai conversation", maximum_count 8
// and element size 0x64.
typedef struct ai_conversation {
    int16_t identifier;               // 0x00 datum_header
    int16_t definition_index;         // 0x02 index into Scenario.ai_conversations (stride 0x74)
    uint8_t priority;                 // 0x04 ai_conversation_new stores its allow_eviction argument here
    uint8_t started;                  // 0x05 the first line has started (0x431e70); status 2 until then
    uint8_t active;                   // 0x06 every participant resolved; status 1 (trying to begin) while clear
    uint8_t finished;                 // 0x07 ai_conversation_update sets it past the last line
    uint8_t waiting_for_advance;      // 0x08 a line with flag bit 3 (wait until told to advance) is waiting; status 4
    uint8_t advance;                  // 0x09 ai_conversation_mark_all (0x430a20, the CEA ai_conversation_advance) sets it
    uint8_t pad_0a[2];                // 0x0a
    int32_t start_tick;               // 0x0c the game tick the instance was created
    int32_t player_unit_index;        // 0x10 the nearest player's unit, the addressee for player lines
    uint32_t participant_mask;        // 0x14 bit i set once participant i has been resolved
    int16_t participant_variant[8];   // 0x18 the line variant chosen for each participant when it was resolved
    datum_index participant_actor[8]; // 0x28 one actor datum per resolved participant
    int16_t line_index;               // 0x48 index into ScenarioAIConversation.lines, -1 before the first
    int16_t speaker_participant_index; // 0x4a line->participant
    int16_t line_delay_ticks;         // 0x4c line_delay_time * 30, counted down after the line
    int16_t line_flags;               // 0x4e copy of line->flags (look-at bits 0..2, advance handshake 3,
                                      //    wait-for-participant 4/5)
    int32_t speaker_actor_index;      // 0x50 the participant actor of the current line
    int32_t speaker_unit_index;       // 0x54 that actor's unit; cleared when it dies
    uint32_t addressee_unit_index;    // 0x58 the listener unit
    uint32_t sound_index;             // 0x5c the chosen line variant's sound tag
    uint8_t speaker_disembodied;      // 0x60 no actor, or a radio participant: the line plays as a sound impulse
    uint8_t line_started;             // 0x61 the line's sound or speech has been launched
    uint8_t line_spoken;              // 0x62 the speech finished
    uint8_t line_finished;            // 0x63 the line is done (after its delay, or skipped)
} ai_conversation;      // size 0x64
// global 0x008802d4: data_array *ai_conversation_data  element size 0x64, capacity 8

typedef struct ai_conversation_event {
    int16_t definition_index;         // 0x00 the ai_conversation definition that stopped
    uint8_t reason_a;                 // 0x02
    uint8_t reason_b;                 // 0x03
    int32_t tick;                     // 0x04 game time at 0x006f1d6c+0x0c
    uint8_t unknown_08[8];            // 0x08
} ai_conversation_event; // size 0x10

// One slot of the 32-entry ring at ai_globals.recent_events that ai_accumulate_repeated_event
// @0x42c0f0 maintains.
typedef struct ai_recent_event_record {
    int16_t event_id;      // 0x00 -1 marks an expired or free slot
    int16_t count;         // 0x02
    real_point3d position; // 0x04 running (weighted) average position
    int32_t last_tick;     // 0x10
} ai_recent_event_record; // size 0x14

// ---------------------------------------------------------------------------
// ai globals
// ---------------------------------------------------------------------------
typedef struct ai_globals {
    uint8_t ai_active;                // 0x00 hs ai (the CEA ai_globals_ai_active)
    uint8_t actors_valid;             // 0x01 every actor and encounter entry point returns early when this is clear
    uint8_t ai_was_active;            // 0x02 set after each tick with AI on; the first tick with AI off resets
                                      //    perception once and clears it
    uint8_t stagger_claimed;          // 0x03 0x429430 claims the per-tick idle slot
    int16_t stagger_threshold;        // 0x04
    int16_t stagger_highest;          // 0x06 0x429430 tracks the highest idle counter seen
    datum_index first_encounterless_actor; // 0x08 head of the list of actors with no encounter (the CEA
                                           //    encounterless_*)
    float major_upgrade_error;        // 0x0c global twin of encounter_squad_state.major_upgrade_error
    uint8_t dialogue_triggers_enabled; // 0x10 hs ai_dialogue_triggers (the CEA ai_globals_dialogue_triggers_enabled)
    uint8_t unknown_11;               // 0x11
    int16_t unknown_12;               // 0x12
    int32_t loudest_line_tick[3][2];  // 0x14 0x42d230 zeroes 0x14..0x2b, ai_reset_for_new_map sets them all to none.
                                      //    [tier][category]: ai_communication_record_line_played
                                      //    (0x42f9e0) max-accumulates its stamp into [0] when tier <= 5, [1] when
                                      //    tier >= 3, [2] when tier >= 5; category is
                                      //    actor_classify_communication_object_type (0 or 1). Read as
                                      //    (int32_t *)&loudest_line_tick[tier][0] + category
    int16_t conversation_event_count; // 0x2c high-water mark, capped at 16
    int16_t conversation_event_cursor;// 0x2e next ring slot, modulo 16
    ai_conversation_event conversation_events[16];// 0x30 0x42d230 zeroes the whole 0x100-byte ring
    int16_t recent_event_head;        // 0x130 ring head (oldest) of the 32 recent events at 0x134
                                      //    (ai_accumulate_repeated_event 0x42c610)
    int16_t recent_event_tail;        // 0x132 next free slot of that ring
    ai_recent_event_record recent_events[32]; // 0x134 the 32-entry ring between recent_event_head/tail
                                      //    (ai_accumulate_repeated_event 0x42c610); ai_reset_for_new_map zeroes it
    uint8_t grenades_enabled;         // 0x3b4 the ai_grenades script command; actors only throw while set
                                     //       (actor_attempt_grenade_throw); ai_reset_for_new_map sets it
    uint8_t unknown_3b5;              // 0x3b5
    int16_t object_attention_count;   // 0x3b6 entries of the 32 x 0x28 object attention table at 0x3b8
    ai_object_attention_record object_attention_table[32]; // 0x3b8 0x500 bytes, runs 0x3b8..0x8b7; the count is
                                      //    object_attention_count at 0x3b6 (ai_object_attention_find_or_create /
                                      //    ai_object_attention_remove). The old communication_hold_tick/3f4/3fa/3fc fields
                                      //    were phantoms: ai_communication_record_line_played (objdump-verified)
                                      //    reads +0x3f0/+0x3fa off the SPEAKING UNIT, not ai_globals, so they
                                      //    are just record 1 (0x3e0..0x407) of this table
    int16_t vehicle_entry_count;      // 0x8b8 ai_process_vehicle_entry_queue drains the queue and zeroes this
    uint8_t unknown_8ba[2];           // 0x8ba
    datum_index vehicle_entry_queue[8];// 0x8bc unit object indices waiting for a seat
} ai_globals;           // size 0x8dc
// global 0x00880354: ai_globals *ai_globals

// ---------------------------------------------------------------------------
// path_find -- the A*-style search over the navigation mesh
// ---------------------------------------------------------------------------
typedef struct path_find_node {
    int16_t unknown_00;               // 0x00
    int16_t parent;                   // 0x02 0xffff on the start node; the reconstruction walks this chain
    int32_t previous_vertex_id;       // 0x04 0x04 vertex_id of the node this one was expanded from (path_find_search
                                      //    next->unknown_04 = node->vertex_id); an edge equal to it is not passable,
                                      //    so the search never steps straight back; -1 on the start node
    uint32_t vertex_id;               // 0x08 hashed as (vertex_id & 0x1ff) into the 512-bucket table
    real_point3d position;            // 0x0c
    float cost;                       // 0x18 g, zero on the start node
    float avoid_distance;             // 0x1c 0x1c smallest avoid-sphere distance seen along the path (FLT_MAX on the
                                      //    start node; path_find_search keeps min(parent, edge) and compute_heuristic
                                      //    returns it as out_secondary)
    float travelled_distance;         // 0x20 0x20 plain path length so far (parent + step), compared with
                                      //    request.limit_distance and used as the leash in
                                      //    path_find_compute_heuristic; 0 on the start node
    float accumulated_cost;           // 0x24 0x24 g, the parent accumulated_cost plus this edge weighted cost (step
                                      //    scaled by the avoid penalty); distance = g + goal distance; 0 on the start
                                      //    node
    float distance;                   // 0x28 the heuristic distance to the goal
    int16_t key;                      // 0x2c the heap ordering key
    int16_t waypoint;                 // 0x2e index into the caller waypoint array, must stay below 0x40
    int16_t heap_index;               // 0x30 the heap keeps this in sync as it sifts
    uint8_t unknown_32[2];            // 0x32
} path_find_node;       // size 0x34

typedef struct path_find_heap_entry {
    int16_t node;                     // 0x00 index into path_find_context.nodes
    int16_t key;                      // 0x02 copy of node.key; the heap is ordered on this
} path_find_heap_entry; // size 0x4

// path_find_context_init zeroes 0x4023 dwords and then overwrites the first 0x48 bytes
// with the callers request block, so the leading fields below are the request and not
// search state. The context is far too large for a stack frame and is passed in by
// pointer; the owning buffer is outside this module.
typedef struct path_find_context {
    uint8_t unknown_00[0x14];         // 0x00 the first 0x48 bytes are copied wholesale from the callers request block
    real_point3d start_position;      // 0x14 path_find_push_start_node rejects a z below -1000.0
    uint32_t start_vertex_id;         // 0x20 none means there is nothing to search from
    uint8_t unknown_24[36];           // 0x24
    uint32_t obstacle_cache;          // 0x48 0x48 second argument of path_find_context_init, a pointer to the
                                      //    per-actor obstacle/search cache (valid +0x10588, count +0x1058a, lists
                                      //    +0x1058c, searches +0x12dac) that ai_navigate_around_obstacles reads;
                                      //    callers pass 0 for none
    uint8_t have_goal;                // 0x4c the whole search and the reconstruction are gated on this
    uint8_t unknown_4d;               // 0x4d
    int16_t target_reaction_threshold;// 0x4e ticks a prop's reaction_timer must reach before the actor refreshes its aim (set by the target relationship pass)
    real_point3d goal_position;       // 0x50
    uint32_t goal_vertex_id;          // 0x5c
    float goal_cost;                  // 0x60
    uint32_t structure_bsp;           // 0x64 path_find_context_init copies the global at 0x00746f9c,
                                      //      ScenarioStructureBSP *global_structure_bsp (scenario.h);
                                      //      kept as a 4-byte pointer field
    int16_t best_node;                // 0x68
    uint8_t unknown_6a[2];            // 0x6a
    float best_cost;                  // 0x6c
    float best_estimate;              // 0x70 0x70 f (g + goal distance) of the node that last improved best_cost;
                                      //    FLT_MAX at path_find_run start; the search stops once node distance
                                      //    exceeds max(5, best_cost) * 10 + this
    real_point3d best_position;       // 0x74
    int16_t node_count;               // 0x80 capped at 1024 by the array below
    uint8_t unknown_82[2];            // 0x82
    path_find_node nodes[1024];       // 0x84
    int16_t heap_count;               // 0xd084 path_find_heap_push refuses past 0x400
    path_find_heap_entry heap[1025];  // 0xd086 one-based, slot 0 unused
    int16_t vertex_hash[4096];        // 0xe08a 512 buckets of 8 entries, probed linearly modulo 0x1000
    uint8_t unknown_1008a[2];         // 0x1008a
} path_find_context;    // size 0x1008c

// ---------------------------------------------------------------------------
// ai point search -- the separate obstacle-graph search (0x43b450..0x43be90)
// ---------------------------------------------------------------------------
typedef struct ai_search_obstacle {
    uint16_t flags;                   // 0x00 bit 0 counts toward the flagged total
    int16_t link;                     // 0x02 the paired obstacle entry, or -1
    uint32_t object_index;            // 0x04
    real_point2d position;            // 0x08
    float radius;                     // 0x10
} ai_search_obstacle;   // size 0x14

// ai_search_gather_obstacles @0x43c510 fills this from object_find_in_sphere plus each
// objects vault / cover surface points; 0x43c4b0 appends and refuses past 0x80 entries.
typedef struct ai_search_obstacle_list {
    int16_t group_count;              // 0x00 0x00 zeroed before gathering; ai_search_partition_into_groups uses it as
                                      //    the next group id and increments per flood-filled group
    int16_t count;                    // 0x02 0x43c4b0 refuses to append past 0x80
    int16_t flagged_count;            // 0x04 entries whose flags bit 0 is set
    uint8_t unknown_06[2];            // 0x06
    ai_search_obstacle obstacles[128];// 0x08
} ai_search_obstacle_list; // size 0xa08

typedef struct ai_search_node {
    real_point2d position;            // 0x00
    float z;                          // 0x08
    real_vector2d direction;          // 0x0c normalized delta from the node to the search origin
    float length;                     // 0x14 the length the normalize returned
    int16_t point_id;                 // 0x18 index into the obstacle list, or -1 for a free point
    uint8_t side;                     // 0x1a which tangent side this node bends around
    uint8_t unknown_1b;               // 0x1b
    int16_t side_link;                // 0x1c two child links, one per side; initialized to -1
    uint8_t squads_carried_over;      // 0x1e ai_squads_merge: 1 on the target encounter, 0 on the source, once
                                      //    the squad references were remapped; also gates skipping empty squads
    uint8_t pad_1f;                   // 0x1f
    float cost;                       // 0x20 length plus the inherited cost
    int16_t parent;                   // 0x24 the node this one was expanded from
    uint8_t unknown_26[2];            // 0x26
} ai_search_node;       // size 0x28

// 0x43b790 initializes the context and pushes the first node; 0x43bcb0 runs one pop /
// expand step and 0x43be20 drives it to completion. The heap count is addressed both as
// context+0x1430 and as the dword index 0x50c of the same base, which is the same byte.
typedef struct ai_search_context {
    uint32_t search_radius;           // 0x00 0x00 float stored as a dword; ai_navigate_around_obstacles passes
                                      //    max(request.pathfinding_radius, 0.2); read as the float radius by
                                      //    ai_search_step, ai_search_expand_point_neighbors and the covering-point
                                      //    lookup
    uint8_t ignores_glass;            // 0x04 the path request's ignores_glass, forwarded as the edge cost permission flag
    uint8_t unknown_05[3];            // 0x05
    uint32_t obstacles;               // 0x08 pointer to the ai_search_obstacle_list this search reads
    uint32_t structure_bsp;           // 0x0c 0x0c pointer to the structure bsp the search traces surfaces in (passed
                                      //    as map to ai_search_evaluate_edge_cost and path_find_heights_are_close)
    real_point2d origin;              // 0x10
    uint32_t origin_surface_index;    // 0x18 0x18 surface index of origin, compared with each edge.surface_index and
                                      //    passed to path_find_heights_are_close(origin, surface, edge surface)
    int16_t goal_point_id;            // 0x1c taken from obstacle[goal].link, or -1
    int16_t result_node;              // 0x1e -1 until a node reaches the goal
    int16_t best_node;                // 0x20 the fallback best-effort node
    uint8_t unknown_22[2];            // 0x22
    float best_cost;                  // 0x24 FLT_MAX until best_node is set
    uint8_t complete;                 // 0x28 set when result_node is valid
    uint8_t final_leg;                // 0x29 set from the caller: the waypoint leg is the last one of a valid path; never read
    uint8_t ignore_flagged_obstacles; // 0x2a 0x2a set to 1 by the rerun in ai_navigate_around_obstacles that ignores
                                      //    flagged obstacles; forwarded to ai_search_evaluate_edge_cost by
                                      //    ai_search_step and ai_search_expand_point_neighbors
    uint8_t unknown_2b;               // 0x2b
    int16_t node_count;               // 0x2c 0x43b5a0 refuses past 0x80
    uint8_t unknown_2e[2];            // 0x2e
    ai_search_node nodes[128];        // 0x30
    int16_t heap_count;               // 0x1430 capped at 0x80
    int16_t heap[128];                // 0x1432 node indices, ordered by ai_search_node.cost
} ai_search_context;    // size 0x1532

// ---------------------------------------------------------------------------
// actor movement obstacle avoidance
// ---------------------------------------------------------------------------
typedef struct actor_movement_obstacle {
    datum_index object_index;         // 0x00
    real_point2d position;            // 0x04 object+0xa0 and object+0xa4
    float bottom;                     // 0x0c object+0xa8 minus (object+0xac minus radius)
    float height;                     // 0x10 twice object+0xac minus twice radius, clamped at zero
    float radius;                     // 0x14 the largest marker-projected radius of the objects collision spheres
} actor_movement_obstacle; // size 0x18

// The 3-dword out block of path_find_trace_cluster_boundary_from_vertex @0x43d790, whose one
// real signature is
//   uint8_t path_find_trace_cluster_boundary_from_vertex(void *context /*EAX*/,
//       uint8_t ignore_permission, real_point2d *point, int32_t start_index,
//       real_vector2d *direction, float max_distance, path_find_boundary_trace_result *out);
// cdecl with 6 stack arguments (every caller cleans with add esp,0x18), returns AL.
// (types/ headers carry no prototypes, so it is recorded here as a comment.) The field roles
// come from 0x43d8b2..0x43d908: out[1] is always the surface the walk ended in (EBP), out[2]
// the enter/exit edge_index of the collision_bsp_boundary_clip (physics.h).
typedef struct path_find_boundary_trace_result {
    float distance;                   // 0x00 the clip t that stopped the walk, max_distance on a miss
    int32_t surface_index;            // 0x04 the surface reached (start_index or one crossed into)
    int32_t edge_index;               // 0x08 the edge hit, -1 on a miss
} path_find_boundary_trace_result;    // size 0x0c

// The stack frame actor_movement_choose_avoidance_direction @0x4193d0 builds and hands to
// actor_movement_collect_obstacle_candidates and actor_movement_test_obstacle_ray. The
// obstacle array runs from 0x40 to 0x603f, which divides exactly into 0x400 entries of
// 0x18, and the two trailing floats are the last two locals of that frame.
typedef struct actor_movement_context {
    uint32_t structure_bsp;           // 0x00 the global at 0x00746f9c, ScenarioStructureBSP *
                                      //      global_structure_bsp (4-byte pointer field)
    uint32_t collision_bsp;           // 0x04 the global at 0x00746f98, ModelCollisionGeometryBSP *
                                      //      global_structure_collision_bsp (ScenarioStructureBSP
                                      //      +0xb4); handed to collision_bsp_query_segment_init
    datum_index unit_index;           // 0x08 actor.active_unit_index, or actor.unit_index
    real_point3d position;            // 0x0c object_get_position of that unit
    real_vector3d forward;            // 0x18 object+0x74
    real_vector3d left;               // 0x24 forward cross up
    real_vector3d up;                 // 0x30 object+0x80
    int16_t obstacle_count;           // 0x3c actor_movement_collect_obstacle_candidates refuses past 0x400
    uint8_t unknown_3e[2];            // 0x3e
    actor_movement_obstacle obstacles[1024];// 0x40
    float ray_scale;                  // 0x6040 0x6040 1.0; multiplies the sample direction before the actor position
                                      //    is added in actor_movement_test_obstacle_ray (set by
                                      //    actor_movement_choose_avoidance_direction)
    float search_radius;              // 0x6044 12.0
} actor_movement_context; // size 0x6048

// ---------------------------------------------------------------------------
// firing position selection (0x411000..0x414130)
// ---------------------------------------------------------------------------
// actor_find_best_firing_position @0x412ba0 walks the owning ScenarioEncounter
// firing_positions block, wraps every position whose group_index bit is in
// actor_firing_position_query.group_mask in one of these candidate records, runs the
// scoring table at 0x006555c0 and the rejection table at 0x006555f8 over them, sorts by
// score and returns the winning firing position index. The 0x3c stride is fixed by the
// initializer in that function, which writes every field below, and by the four scoring
// routines, which all step their cursors by 0xf floats.
typedef struct actor_firing_position_candidate {
    uint32_t position;                 // 0x00 -> the ScenarioFiringPosition (a real_point3d then group_index)
    int16_t firing_position_index;     // 0x04 index of that position inside the encounter block
    int16_t request_result;            // 0x06 actor_report_firing_position_request stores the perception result code here
    float distance_from_actor;         // 0x08 path length, or the straight-line distance while flying; FLT_MAX until filled
    real_vector3d direction_from_actor;// 0x0c unit vector, zero until filled
    float distance_from_target;        // 0x18 second path query, run from the target position
    float segment_distance;            // 0x1c distance to the actor-to-target segment
    real_vector3d direction_from_target;// 0x20
    float distance_squared_to_target;  // 0x2c actor_score_firing_positions_by_range takes its square root
    uint8_t valid;                     // 0x30 cleared by a rejection rule; the initializer sets 1
    uint8_t rejected;                  // 0x31 a rejection rule fired but query.collect_all kept the candidate alive
    uint8_t unknown_32[2];             // 0x32
    float score_before_rejects;        // 0x34 copy of score taken between the scoring and the rejection pass
    float score;                       // 0x38 HIGHER is better: every rule adds desirability, and
                                       //   actor_find_best_firing_position keeps the largest surviving score
} actor_firing_position_candidate; // size 0x3c

// One entry of the danger-sphere array inside the query.
typedef struct actor_firing_position_danger_sphere {
    float radius;                      // 0x00
    real_point3d position;             // 0x04
} actor_firing_position_danger_sphere; // size 0x10

// One entry of the hazard array inside the query. kind 0 and 1 are the two marked-object
// kinds actor_score_firing_positions_by_history counts, kind 2 is the avoidance plane
// actor_score_firing_positions_by_range projects candidates onto.
typedef struct actor_firing_position_hazard {
    int16_t kind;                      // 0x00
    uint8_t unknown_02[2];             // 0x02
    real_point3d position;             // 0x04
    real_vector3d direction;           // 0x10
} actor_firing_position_hazard; // size 0x1c

// The caller-owned request-and-result block every routine in the firing position pipeline
// is handed. The two array bounds are the literal caps the gatherer enforces (0x20 danger
// spheres, 0x20 hazards); the trailing fields are the resolved threat description the
// scoring rules read. 0x664 is where the last field ends, not a size the binary states.
typedef struct actor_firing_position_query {
    uint32_t group_mask;               // 0x00 ScenarioSquadAttacking bits; actor_get_firing_position_group_mask builds it
    int16_t goal_kind;                 // 0x04 selects table rows: bit (1 << goal_kind) against each rule mask
    int16_t unknown_06;                // 0x06
    union {
        uint8_t unknown_08[8];             // 0x08
        struct {
            datum_index pursuit_target_index;       // 0x08 goal_kind 5 (pursuit): the prop's target unit
            uint32_t pursuit_last_perceived_time;   // 0x0c the target prop's last_perceived_time, or none
        };
    };
    uint8_t score_instead_of_reject;   // 0x10 the pursuit rule adds a penalty rather than rejecting
    uint8_t unknown_11[3];             // 0x11
    uint8_t collect_all;               // 0x14 mark rejected candidates instead of clearing valid
    uint8_t allow_random_fallback;     // 0x15 when nothing is in range, pick one candidate at random
    uint8_t unknown_16[2];             // 0x16
    float maximum_distance;            // 0x18 15.0, or 80.0 when actor.unknown_15e is nonzero
    float search_radius;               // 0x1c defaults to maximum_distance when the caller leaves it zero
    uint8_t have_explicit_target;      // 0x20 use the explicit block below instead of the actor own threat
    uint8_t unknown_21[3];             // 0x21
    real_point3d explicit_target_position;// 0x24
    uint32_t explicit_target_object;   // 0x30
    int16_t explicit_target_cluster_index;// 0x34 cluster of an explicit target (actor+0xa8)
    uint8_t unknown_36;                // 0x36
    uint8_t unknown_37;                // 0x37
    float avoid_weight;                // 0x38 0x38 copied to path_find_request.avoid_weight in
                                       //    actor_find_best_firing_position (the old comment said avoidance radius,
                                       //    the code says weight)
    float avoid_radius;                // 0x3c 0x3c copied to path_find_request.avoid_radius in
                                       //    actor_find_best_firing_position
    uint8_t danger_active;             // 0x40 the actor is registering a danger; the threat rule runs the segment tests
    uint8_t use_last_seen_position;    // 0x41 0x41 target_lead_position takes the prop last_seen_position instead of
                                       //    head_position when set and the target was ever seen;
                                       //    actor_check_melee_target_reachable and
                                       //    actor_request_path_with_grenade_arc set it from record[4] / actor[0xa0]
    uint8_t unknown_42;                // 0x42 goal_kind == 5
    uint8_t want_direction_from_target;// 0x43 also fill direction_from_target on each candidate
    uint8_t flying;                    // 0x44 copy of actor.flying; skips every path query
    uint8_t check_vehicle_aim_cone;    // 0x45 0x45 set when actor.vehicle_driving_type == 4;
                                       //    actor_score_firing_positions_by_threat then penalises/rewards candidates
                                       //    by their angle to the driven vehicle forward vector
    uint8_t vehicle_ignore_velocity;   // 0x46 0x46 set together with check_vehicle_aim_cone; skips the vehicle-speed
                                       //    test in actor_score_firing_positions_by_threat
    uint8_t unknown_47;                // 0x47
    uint32_t marked_group_mask;        // 0x48 a second group mask; matching candidates take marked_group_penalty
    float marked_group_penalty;        // 0x4c
    int32_t danger_sphere_count;       // 0x50 capped at 0x20 by the gatherer
    actor_firing_position_danger_sphere danger_spheres[32]; // 0x54
    int16_t hazard_count;              // 0x254
    int16_t hazard_count_kind_01;      // 0x256
    int16_t hazard_count_kind_2;       // 0x258
    uint8_t unknown_25a[2];            // 0x25a
    actor_firing_position_hazard hazards[32];// 0x25c
    uint8_t have_standing_gun_offset;  // 0x5dc
    uint8_t unknown_5dd[3];            // 0x5dd
    real_vector3d standing_gun_offset; // 0x5e0 ActorVariant.custom_stand_gun_offset, else Actor.standing_gun_offset
    uint8_t have_crouching_gun_offset; // 0x5ec
    uint8_t unknown_5ed[3];            // 0x5ed
    real_vector3d crouching_gun_offset;// 0x5f0 ActorVariant.custom_crouch_gun_offset, else Actor.crouching_gun_offset
    uint8_t have_target;               // 0x5fc every threat-relative rule is gated on this
    uint8_t unknown_5fd[3];            // 0x5fd
    float target_distance;             // 0x600 prop.distance, or the straight-line distance for an explicit target
    real_point3d target_position;      // 0x604 prop.last_known_position
    real_point3d target_aim_position;  // 0x610 prop+0x104
    real_point3d target_lead_position; // 0x61c prop+0x90 when unknown_41 and prop+0x8c is set, else prop+0x104
    uint8_t target_is_large;           // 0x628 goal_kind 4 or 6
    uint8_t unknown_629[3];            // 0x629
    int32_t target_relationship_object;// 0x62c prop.relationship_object_index
    uint32_t target_surface_index;     // 0x630 prop+0xec, or explicit_target_object; a navmesh surface id
    real_point3d target_surface_point; // 0x634 prop+0xf0, or explicit_target_position; the point on it
    int16_t target_cluster_index;        // 0x640 the target prop's cluster_index (prop+0x100)
    uint8_t unknown_642[2];            // 0x642
    datum_index target_prop_index;     // 0x644 the prop the block above was read from, or none
    uint8_t have_target_vault_point;   // 0x648 set when the prop kind is 4 or 5
    uint8_t unknown_649[3];            // 0x649
    real_point3d target_vault_point;   // 0x64c prop+0x40
    float target_danger_radius;          // 0x658 the target prop's danger_radius (0 for an explicit target)
    uint8_t baseline_accept;           // 0x65c the rejection table run with no candidate at all
    uint8_t unknown_65d[3];            // 0x65d
    float baseline_penalty;            // 0x660 the score an ideal candidate would earn; used as the
                                       //   early-out margin while scanning the sorted candidates
} actor_firing_position_query; // size 0x664

// One row of the two rule tables. A rule runs when (1 << query.goal_kind) & kinds. The
// procedure field is a plain address so the CParser does not need its signature.
//   0x006555c0 actor_firing_position_score_rules[7]
//     0xffff 0x4112b0, 0x0009 0x411bf0, 0x004d 0x411ee0, 0x0010 0x411b60,
//     0x0002 0x411980, 0x0020 0x411840, then the { 0, 0 } terminator
//   0x006555f8 actor_firing_position_reject_rules[6]
//     0xffff 0x412290, 0x0051 0x412620, 0x0008 0x412570, 0x0006 0x4124c0,
//     0x0020 0x412350, then the { 0, 0 } terminator
// Read straight out of bin/halo.exe. The five procedures 0x411840, 0x411980, 0x411b60,
// 0x4124c0 and 0x412570 have no entry in out/functions.json: Ghidra never created
// functions there because nothing but these tables reaches them, so they are missing from
// out/phase4/ai_functions.md as well and have no rewrite under src/ai.
// The 0x48-byte request block path_find_context_init copies over the head of a
// path_find_context. actor_firing_position_near_point @0x412960 and
// actor_find_best_firing_position @0x412ba0 both build one on the stack, zero it, fill the
// fields below and then memcpy it in, which is what names the fields the header could only
// call unknown up to 0x48.
typedef struct path_find_request {
    float pathfinding_radius;          // 0x00 Actor.pathfinding_radius
    uint8_t ignores_glass;             // 0x04 actor.ignores_glass
    uint8_t unknown_05[3];             // 0x05
    datum_index exclude_object_index_a; // 0x08 0x08 object excluded when ai_navigate_around_obstacles gathers
                                        //    obstacles (ai_search_gather_obstacles arg);
                                        //    actor_build_path_find_request sets the actor unit, the firing position
                                        //    callers set none
    datum_index exclude_object_index_b; // 0x0c 0x0c second object excluded from the gathered obstacles
                                        //    (ai_search_gather_obstacles arg); none for most callers,
                                        //    actor_movement_action_resolve passes the movement action extra
    uint8_t have_start;                // 0x10
    uint8_t unknown_11[3];             // 0x11
    real_point3d start_position;       // 0x14
    uint32_t start_surface_index;      // 0x20
    uint8_t have_avoid_sphere;         // 0x24
    uint8_t unknown_25[3];             // 0x25
    real_point3d avoid_position;       // 0x28
    datum_index avoid_object_index;    // 0x34
    float avoid_radius;                // 0x38
    float avoid_weight;                // 0x3c
    uint8_t have_limit;                // 0x40
    uint8_t unknown_41[3];             // 0x41
    float limit_distance;              // 0x44
} path_find_request; // size 0x48

typedef struct actor_firing_position_rule {
    int16_t kinds;                     // 0x00 bitmask over goal_kind
    uint8_t unknown_02[2];             // 0x02
    uint32_t proc;                     // 0x04 zero terminates the table
} actor_firing_position_rule; // size 0x8

// ---------------------------------------------------------------------------
// small caller-owned request blocks
// ---------------------------------------------------------------------------
// The block actor_build_order_look @0x4046c0 fills in.
typedef struct actor_look_request {
    uint8_t unknown_00[8];             // 0x00
    int16_t explicit_direction;        // 0x08 -1 when no explicit direction or waypoint is given
    uint8_t unknown_0a[2];             // 0x0a
    int16_t force_random;              // 0x0c nonzero forces the combat timing table
    uint8_t unknown_0e[18];            // 0x0e
    uint8_t has_target_point;          // 0x20 nonzero: target_point below is valid
    uint8_t unknown_21[3];             // 0x21
    real_point3d target_point;         // 0x24
} actor_look_request; // size 0x30, only verified up to 0x2f

// The block actor_get_body_axis_vector @0x405390 fills in; actor_squad_action_execute
// builds one on the stack purely to call it.
typedef struct actor_axis_request {
    uint8_t unknown_00[8];             // 0x00
    int16_t axis;                      // 0x08 0 forward, 1 back, 2 and 3 a perpendicular pair
    uint8_t unknown_0a[2];             // 0x0a
    real_vector3d result;              // 0x0c
} actor_axis_request; // size 0x18, only verified up to 0x17

// The block actor_consider_combat_mode @0x401a60 fills in and
// actor_get_consideration_wait_threshold @0x4028e0 reads back.
typedef struct actor_combat_consideration {
    int32_t game_tick;                 // 0x00 snapshot of the current tick
    int16_t mode;                      // 0x04 the resulting consideration mode
    uint8_t unknown_06[4];             // 0x06
    uint8_t grenade_eligible;          // 0x0a a random-chance melee-leap or grenade roll succeeded
    uint8_t unknown_0b[33];            // 0x0b
    float wait_threshold;              // 0x2c actor_get_consideration_wait_threshold result
    uint8_t suicidal;                  // 0x30 Actor.flags has suicidal_melee_attack
    uint8_t unknown_31;                // 0x31
    int16_t position_index;            // 0x32
    float distance_delta;              // 0x34
} actor_combat_consideration; // size 0x38

// The 16-byte block actor_begin_vocalization @0x4142d0 is handed and copies wholesale into
// actor.vocalization_source: an actor_flee_source_reason (code 1 names a prop by handle, code 3 a point).
typedef actor_flee_source_reason actor_vocalization_context;

// The out-parameter of actor_get_grenade_launch_velocity @0x410980, which
// actor_commit_grenade_toss @0x411180 passes through.
typedef struct grenade_solution {
    real_vector3d velocity;            // 0x00 aim direction scaled by the solved speed
    float unknown_0c;                  // 0x0c
    float unknown_10;                  // 0x10
    float unknown_14;                  // 0x14
} grenade_solution; // size 0x18

// The out-parameter of ai_conversation_get_run_to_player_range @0x402cf0.
typedef struct ai_conversation_range_lookup {
    uint32_t conversation_index;       // 0x00 echoes the caller index
    uint32_t unknown_04;               // 0x04 always zero
    float run_to_player_dist;          // 0x08 ScenarioAIConversation.run_to_player_dist, or 0 when disabled
    int32_t player_unit_index;         // 0x0c 0x0c ai_conversation_get_run_to_player_range copies
                                       //    ai_conversation.player_unit_index here (-1 when run_to_player_dist is 0)
    uint32_t unknown_10;               // 0x10 always -1
} ai_conversation_range_lookup; // size 0x14

// The firing-pattern block of an ActorVariant tag at +0xcc (burst_origin_radius .. burst_angular_velocity) that the
// actor's aim and burst timing draw from, and the four-float scale block (+0x100 new target, +0x118 moving,
// +0x130 berserk) multiplied onto it; ai_actor_select_stance_offset_pair picks both.
typedef struct actor_burst_parameters {
    float origin_radius;     // 0x00
    float origin_angle;      // 0x04
    float return_length[2];  // 0x08
    float return_angle;      // 0x10
    float duration[2];       // 0x14
    float separation[2];     // 0x1c
    float angular_velocity;  // 0x24
} actor_burst_parameters;    // size 0x28

typedef struct actor_burst_scale {
    float duration;          // 0x00
    float separation;        // 0x04
    float rate_of_fire;      // 0x08
    float projectile_error;  // 0x0c
} actor_burst_scale;         // size 0x10

// One row of the per-ActorType table actor_type_procs points at. Only the offsets the
// module actually reads are named.
typedef struct actor_type_table_entry {
    uint8_t unknown_00[4];             // 0x00
    uint16_t flags;                    // 0x04 type flag word; bit 1 (value 2) is tested by the unit-class and dormancy checks
    int16_t ax_mode;                   // 0x06 seed of the first mode word actor_update passes to starting_location_derive_placement_flags
    int16_t cx_mode;                   // 0x08
    int16_t mode_b;                    // 0x0a
    uint8_t swarm;                     // 0x0c 0x0c compared against the actor swarm flag (caller_type_flag) in
                                       //    actor_validate_grenade_ally_candidate; also the placement phase byte seed
    uint8_t swarm_actor;               // 0x0d compared against actor.swarm when a unit is placed or an actor changes type
    uint8_t unknown_0e[2];             // 0x0e
    uint32_t proc_10;                  // 0x10 actor_dispatch_type_vtable @0x426670
    uint32_t proc_14;                  // 0x14 0x14 per-actor-type procedure called with the actor index each pass of
                                       //    actor_run_mode_transition_loop when non-null (same proc_10/18/1c
                                       //    convention)
    uint32_t proc_18;                  // 0x18 @0x4266a0
    uint32_t proc_1c;                  // 0x1c @0x4266d0
} actor_type_table_entry; // size 0x20, only verified up to 0x1f

// The 8-byte {flag, candidate} record the recognition/look scan helpers fill in through an
// out-parameter. Written by actor_select_facing_target_prop @0x414a90 and read back by
// actor_resolve_look_target @0x414d00 and actor_look_randomize_direction @0x414f50.
typedef struct actor_recognition_scan_result {
    int16_t flag;             // 0x00 set to 1 on success
    uint8_t unused_02[2];     // 0x02 padding
    datum_index candidate;    // 0x04 the winning prop datum handle
} actor_recognition_scan_result; // size 0x08


// One bucket of the call-for-help grouping table ai_group_bucket_find_or_add @0x420de0
// maintains on the caller stack for actor_scan_allies_for_backup_request @0x420ec0.
typedef struct ai_group_bucket_entry {
    int16_t priority;      // 0x00 danger level raised for this object: 6 friends retreating, 7 danger timer, 8 player
                           //    danger, 9 friends killed
    uint8_t unknown_02[2]; // 0x02
    int32_t prop_index;    // 0x04 this actor's prop for the object, none until known
    int32_t key;           // 0x08 the object index this bucket groups calls-for-help by
    prop *prop;            // 0x0c that prop record
    int16_t retreating_friend_count; // 0x10 allies with a call out about this object; compared with
                                     //    Actor.friends_retreating_trigger
    uint8_t unknown_12[2]; // 0x12
    float nearest_friend_distance_squared; // 0x14 FLT_MAX at creation (0x420de0), the minimum over calling allies
    int32_t nearest_friend_actor_index; // 0x18 the ally at that distance
} ai_group_bucket_entry; // size 0x1c

// The work queue ai_release_inactive_encounters walks: how many entries it holds, the next one to release, then the
// entries (kind 0 releases an encounter, anything else an encounterless actor).
typedef struct ai_release_entry {
    uint8_t kind;             // 0x00
    uint8_t unknown_01[3];    // 0x01
    uint32_t index;           // 0x04 the encounter or actor datum
    uint8_t unknown_08[4];    // 0x08
} ai_release_entry;           // size 0x0c

typedef struct ai_release_state {
    int16_t count;            // 0x00
    int16_t cursor;           // 0x02
    ai_release_entry entries[1]; // 0x04 count entries
} ai_release_state;

// One entry of the two perception candidate lists actor_target_scan_potential_targets
// @0x41d7e0 builds on its stack, sorted with ai_target_distance_qsort_compare @0x41d7a0.
typedef struct ai_target_candidate {
    datum_index object_index; // 0x00 prop.object_index of the candidate
    datum_index prop_index;   // 0x04 an existing prop datum index, or none for a fresh one
    float distance;           // 0x08 distance^2 * 0.6944444, the qsort key
} ai_target_candidate; // size 0x0c

typedef struct ai_target_candidate_list {
    int16_t seen_count;  // 0x00 how many candidates this list accepted (the caller caps it at 4)
    int16_t entry_count; // 0x02 how many of the entries below are valid
    ai_target_candidate entries[128];
} ai_target_candidate_list; // size 0x604

// The callback actor_swarm_for_each_component @0x407040 and its thunk invoke per member.
typedef void (*actor_swarm_member_callback)(uint32_t actor_index, datum_index unit_index,
                                            uint16_t command_list_index, actor_squad_action_state *action,
                                            actor_command_aim *aim, uint32_t callback_extra);

// ---------------------------------------------------------------------------
// Caller-owned scratch records
//
// These are stack blocks the functions of this module pass to each other, not entries of any
// data_array, so nothing in the image declares their size directly; each layout below is
// the union of the offsets the functions that build and read it actually touch. They were
// folded here from per-file TYPES-GAP typedefs in src/ai/*.c during the phase-4 review pass
// so that every file that shares one shares the same declaration.
// ---------------------------------------------------------------------------

// The filter/cursor block actor_iterator_next @0x436a70 walks. Every caller builds it
// inline: filter_array is the data_array the scan is restricted to (encounter_data in every
// call site seen so far), cursor and actor_index start at -1, active is 1, and signature is
// filter_array XOR 0x69746572 (ASCII iter, little-endian) -- the same self-check
// data_iterator uses. actor_index is the field callers read back to learn which actor the
// last _next returned; established from the disassembly of
// ai_communication_select_speaker_by_team (mov esi,[esp+0x34] at 0x43025d = iterator + 0x14).
typedef struct actor_iterator_state {
    data_array *filter_array;  // 0x00
    int16_t next_index;        // 0x04 data_iterator.next_index over encounter_data (the first 0x10 bytes are a
                               //    data_iterator)
    uint8_t unknown_06[2];     // 0x06
    int32_t cursor;            // 0x08 -1 (not yet started)
    uint32_t signature;        // 0x0c filter_array XOR 0x69746572
    uint8_t encounterless_done; // 0x10 set when the walk moves on to ai_globals.first_encounterless_actor, so that
                                //    list runs once
    uint8_t active;            // 0x11 1
    uint8_t unknown_12[2];     // 0x12
    datum_index actor_index;   // 0x14 handle of the actor the last _next returned, else none
    int32_t next_actor_index;  // 0x18 the next actor of the current list (encounter.first_actor / next_in_encounter)
} actor_iterator_state; // size 0x20

// The iterator the pair at 0x432650 (new; ECX -> iterator, stack -> packed ai reference) and
// 0x4326d0 (next; EDX -> iterator) use instead, for every actor named by one packed
// squad/team reference. Only its size (a 0x18-byte stack slot in both call sites) and the
// actor-handle slot are established.
typedef struct ai_reference_actor_iterator {
    int32_t encounter_index;  // 0x00 the encounter being walked, -1 once the reference is invalid
    int32_t squad_filter;     // 0x04 squad index to keep, -1 for every squad
    int32_t platoon_filter;   // 0x08 platoon index to keep, -1 for every platoon
    datum_index cursor_start; // 0x0c first of the three cursor dwords ai_reference_actor_iterator_init_cursor fills
    datum_index actor_index;  // 0x10 handle of the actor the last _next returned
    datum_index next_actor_index; // 0x14 the actor the next call returns
} ai_reference_actor_iterator; // size 0x18

// One candidate of the grenade-avoidance scan: built by actor_grenade_avoidance_entry_init
// @0x42ad40, filled in bulk by actor_gather_nearby_grenade_targets, and consumed by
// actor_grenade_trajectory_blocked and actor_grenade_parabolic_path_clear.
typedef struct ai_grenade_avoidance_entry {
    uint8_t already_clear;        // 0x00 1 when the crouch-offset lookup returned exactly 0.0
    uint8_t unknown_01[3];        // 0x01
    real_point3d target_position; // 0x04 UNSURE: read by the trajectory tests, writer not identified
    uint32_t unknown_10;          // 0x10 zeroed by the writer; read as target_offset.x
    uint32_t unknown_14;          // 0x14 zeroed by the writer; read as target_offset.y
    float crouch_offset;          // 0x18 the first result of the crouch-offset lookup
    datum_index prop_index;       // 0x1c
    datum_index object_index;     // 0x20
    float avoid_until;            // 0x24 the second result of that lookup plus a fixed 0.15s window
} ai_grenade_avoidance_entry; // size 0x28

// The list ai_build_priority_target_list @0x42a4a0 fills and ai_squad_priority_compare
// @0x42a5d0 sorts (a qsort comparator, stride 0x0c).
typedef struct ai_priority_target_record {
    uint8_t tiebreak;   // 0x00
    uint8_t pad[3];     // 0x01
    uint32_t handle;    // 0x04
    int32_t priority;   // 0x08
} ai_priority_target_record; // size 0x0c

typedef struct ai_priority_target_list {
    int16_t count;      // 0x00
    int16_t unknown_02; // 0x02
    ai_priority_target_record records[256];
} ai_priority_target_list; // size 0xc04


// The trace scratch actor_evaluate_engagement_reachability @0x42b1f0 hands to the collision
// request at 0x505880.
typedef struct ai_reachability_scratch {
    uint8_t unknown_00[20]; // 0x00
    float closing_speed;    // 0x14 UNSURE: read back only when the trace reports a hit
} ai_reachability_scratch; // size 0x18, only verified up to 0x17

// The 0x20-byte AI communication event record. ai_communication_broadcast @0x42d340 builds
// one on its stack (Ghidra local_438..local_41c) and passes it to
// ai_propagate_communication_reaction @0x42e9c0, ai_communication_play_event_line @0x42eee0
// and ai_dispatch_queued_order @0x42c5a0. Those readers interpret overlapping but different
// field sets, so the record is declared once per reader rather than merged into one guess:
// ai_queued_order and ai_communication_order are the SAME storage seen two ways.
// Established so far: +0x08 is the event "kind" ai_communication_play_event_line matches a
// table row required_kind field against, and +0x0c / +0x0e are a short pair.
typedef struct ai_queued_order {
    uint8_t unknown_00[0xc]; // 0x00
    int16_t single_target;   // 0x0c
    int16_t target_count;    // 0x0e
    datum_index object_a;    // 0x10
    uint8_t unknown_14[12];  // 0x14
} ai_queued_order; // size 0x20

typedef struct ai_communication_order {
    uint8_t unknown_00[6];    // 0x00
    int16_t row;              // 0x06 the communication line definition row, -1 for none
    uint8_t unknown_08[4];    // 0x08
    int16_t count;            // 0x0c
    uint8_t unknown_0e[6];    // 0x0e
    int16_t order_type;       // 0x14 0 or 1
    uint8_t unknown_16[2];    // 0x16
    int16_t team_a;           // 0x18 UNSURE
    int16_t team_b;           // 0x1a UNSURE
    uint8_t status;           // 0x1c UNSURE
    uint8_t unknown_1d[3];    // 0x1d
} ai_communication_order; // size 0x20

// The 0x20-byte block ai_communication_target_result_reset @0x42d2c0 clears.
// The 0x20 byte record that follows the 0x10 byte header of a unit_speech (the line a unit is about to say and who it is about):
// the communication builders fill it, ai_communication_target_result_reset clears it.
typedef struct ai_communication_target_result {
    datum_index target;       // 0x00 the object the line is about, none when unset
    int16_t event;            // 0x04 the communication event, -1 when unset
    int16_t row;              // 0x06 the event definition row, -1 when unset
    int16_t object_b;         // 0x08 second event object, -1 when unset
    uint8_t valid;            // 0x0a 1 once a builder filled the record
    uint8_t unknown_0b;       // 0x0b
    int16_t look_marker;      // 0x0c marker the listeners look at
    int16_t look_kind;        // 0x0e 1 object, 2 danger
    datum_index look_object;  // 0x10 the object the listeners look at
    int16_t tag_value;        // 0x14 third event object, 0 when none
    uint8_t unknown_16[2];    // 0x16
    uint32_t extra_data[2];   // 0x18 the two extra words of the event, copied through
} ai_communication_target_result; // size 0x20
typedef char ai_communication_target_result_size[sizeof(ai_communication_target_result) == 0x20 ? 1 : -1];

// The record ai_communication_gate_line_played @0x42cfe0 inspects.
typedef struct ai_communication_record {
    datum_index object_index; // 0x00 UNSURE: guessed from context
    uint8_t unknown_04[2];    // 0x04
    int16_t line_row;         // 0x06 the communication event row passed on to ai_communication_record_line_played
    uint8_t unknown_08[2];    // 0x08
    uint8_t silenced;         // 0x0a
} ai_communication_record; // size 0x0c, only verified up to 0x0a

// One row of the AI communication event table at 0x00656b08, terminated by a row whose
// event_id is -1. Stride 0x24, confirmed by the PTR_FUN_00656b28 / PTR_FUN_00656b4c
// predicate slots sitting exactly 0x24 apart. class_index selects a column of the three
// eight-entry per-class tables at 0x006558c4 (int16 line class), 0x006558d4 (float delay)
// and 0x006558f4 (int16 follow-up order).
typedef struct ai_communication_event_definition {
    int16_t event_id;            // 0x00
    int16_t required_kind;       // 0x02 -1 = any; else must equal +0x08 of the event record
    int16_t selection;           // 0x04 2 = squad speaker, 3 = fixed object, 4 = hostile speaker
    int16_t line_id;             // 0x06
    int16_t seat_filter;         // 0x08
    int16_t class_index;         // 0x0a
    uint8_t flags;               // 0x0c bit 0 = ignore the global warm-up tick
    uint8_t unknown_0d[3];       // 0x0d
    float probability;           // 0x10 0 = never, unless the caller forces it
    float unknown_14;            // 0x14
    float delay_seconds;         // 0x18
    float unknown_1c;            // 0x1c
    uint8_t (*predicate)(datum_index object_index, void *event_record,
                         datum_index speaker_actor_index); // 0x20 optional extra gate
} ai_communication_event_definition; // size 0x24
typedef char ai_communication_event_definition_size[sizeof(ai_communication_event_definition) == 0x24 ? 1 : -1];

// One row of the second, larger AI communication table at 0x00655aa0 -- the "ai conversation"
// line table ai_communication_broadcast @0x42d340 walks. A chain of rows shares one event_id
// and ends at the first row whose event_id no longer matches the id being looked up; the
// starting row index for an event code comes from the int16 index table at 0x008802e0.
// Stride 0x28, recovered from the row arithmetic in ai_communication_broadcast. Sibling of
// ai_communication_event_definition above; several field names are guessed by analogy and
// carry an UNSURE tag.
typedef struct ai_communication_line_definition {
    int16_t event_id;             // 0x00 chain key; matched against the broadcast event code
    int16_t class_index;          // 0x02 column into the three per-class tables at
                                  //      0x006558c4 (tier) / 0x006558d4 (unused by the
                                  //      broadcast path) / 0x006558f4 (follow_up_order)
    int16_t table_arg_a;          // 0x04 UNSURE: default direction_class, also forwarded
                                  //      verbatim to 0x4300d0 / 0x42ff80 / 0x42ec90
    int16_t table_arg_b;          // 0x06 UNSURE: forwarded alongside table_arg_a
    int16_t participant_selector; // 0x08 which resolved unit/object speaks or is addressed
                                  //      (switch on this field, cases 0/1/2/4)
    int16_t fallback_order;       // 0x0a UNSURE: alternate lookup key; -1 and 1 both mean
                                  //      "use the per-class default"
    int16_t look_target_selector; // 0x0c who looks at whom (switch, cases 1..4)
    int16_t look_marker_selector; // 0x0e UNSURE: node/marker id override
    float   probability;          // 0x10 multiplies directly into the candidate weight
    uint8_t unknown_14[4];        // 0x14 UNSURE: never read by the broadcast path
    uint8_t flags;                // 0x18 bit0/bit4/bit5 build the flag byte of 0x4300d0; bit1 =
                                  //      broadcast to every recognizer; bit3 = allow a
                                  //      no-actor object as a valid speaker
    uint8_t unknown_19;           // 0x19 (one byte only -- +0x1a is read as an int16, see
                                  //      the DAT_00655aba reference in 0x42d340)
    int16_t capability_index;     // 0x1a UNSURE: index into the per-participant capability
                                  //      byte array (self_capability / other_capability)
    int16_t required_kind;        // 0x1c index into the 5-slot reason bucket array
    int16_t relationship_gate;    // 0x1e UNSURE: index into the 8-byte combat flag block
    uint16_t source_type_mask;    // 0x20 must intersect the source object type/team mask
    uint16_t target_type_mask;    // 0x22 must intersect the target object type/team mask
    int16_t required_seat;        // 0x24 must equal the broadcast seat argument unless -1
    uint8_t unknown_26[2];        // 0x26 never read by the broadcast path
} ai_communication_line_definition; // size 0x28
typedef char ai_communication_line_definition_size[sizeof(ai_communication_line_definition) == 0x28 ? 1 : -1];

// The per-candidate scratch record ai_communication_broadcast @0x42d340 builds on its stack,
// one per surviving ai_communication_line_definition row (up to 16), scores, and then hands
// to four different readers that each use a different overlapping field set. The byte offsets
// are confirmed by stack-offset arithmetic against the disassembly of 0x42d340..0x42e930
// (entry_esp - N for every Ghidra local_N, with a flat sub esp,0x4cc prologue); the field
// names beyond score / entry_index / participant_object_index are best-effort. entry_index is
// independently confirmed: it is the value passed to ai_communication_record_line_played as
// its communication_line_id.
typedef struct ai_communication_candidate {
    float    score;                    // 0x00 selection weight; <= 0 candidates are dropped
    uint8_t  global_broadcast;          // 0x04 bit 1 of the row flags byte
    uint8_t  already_played;            // 0x05 "already resolved a target" end gate
    int16_t  direction_class;           // 0x06 table_arg_a, or a 0x00655914 override
    int16_t  tier;                      // 0x08 the line class/tier 0..7, from 0x006558c4
    int16_t  raw_field_0a;              // 0x0a table_arg_b; re-read as +10 for a -1
                                        //      "no marker" test
    int16_t  line_delay_low;            // 0x0c low 16 bits of a recency fraction
    int16_t  look_marker;               // 0x0e low 16 bits of a look-marker id
    int16_t  result_index;              // 0x10 UNSURE: an object/prop table index
    uint8_t  unknown_12[2];             // 0x12
    datum_index participant_object_index; // 0x14 the object handed to the queue/marker calls
    datum_index speaker_actor_index;    // 0x18 the resolved speaker/listener actor
    datum_index other_object_index;     // 0x1c UNSURE: a second resolved object handle
    datum_index object_result;          // 0x20 result of the 0x42ec90 target search, or -1
    int16_t  follow_up_order;           // 0x24 fallback_order, or a per-class default
    int16_t  look_node_a;               // 0x26 UNSURE: look-node id
    int16_t  look_node_b;               // 0x28 UNSURE: look-node id
    uint8_t  unknown_2a[2];             // 0x2a
    datum_index order_target;           // 0x2c the object/actor 0x4302e0 should target
    datum_index order_fallback;         // 0x30 the fallback 0x4302e0 should use instead
    uint16_t entry_index;               // 0x34 row index into ai_communication_lines
    uint8_t  unused_36[2];              // 0x36 stride padding, never referenced
} ai_communication_candidate; // size 0x38

// Not a game structure: the shape this module uses to model one specific MSVC calling-
// convention artifact, a helper that returns a boolean in AL and a float in ST0 at the same
// time. Two ai functions call such helpers (actor_rate_potential_target @0x41dc50 through
// 0x41d800, actor_target_hearing_check @0x41e470 through 0x53e810) and both used to declare
// their own identical local copy of this record.
typedef struct bool_float_return {
    uint8_t truthy; // AL
    float value;    // ST0
} bool_float_return;

// The 0x38-byte block actor_get_firing_positions fills: a copy of the actor's aim_origin .. unknown_14c run
// (aim origin, body position, forward vector, bsp location, velocity), or the same data taken from the nearest
// unit of a swarm. Target perception, hearing, danger and squad-link code measure against it.
typedef struct actor_firing_positions {
    real_point3d aim_origin;      // 0x00 actor.aim_origin
    real_point3d body_position;   // 0x0c actor.body_position
    real_vector3d forward;        // 0x18 the unit forward vector
    bsp_leaf_reference location;  // 0x24 bsp leaf and cluster of the head position
    real_vector3d velocity;       // 0x2c
} actor_firing_positions;         // size 0x38


// One cell of the per-line history tables of the communication and conversation lines (game state, row * 2 + category).
typedef struct ai_line_history {
    int32_t last_tick;           // 0x00 game tick the line was last spoken in this category, -1 never
    int32_t cooldown_until_tick; // 0x04 game tick before which the line may not repeat, -1 none
} ai_line_history;               // size 0x08

// The spawn request actor_place_new_unit @0x421ea0 reads out of EAX.
typedef struct actor_placement_request {
    real_point3d position;  // 0x00 ScenarioActorStartingLocation.position
    float yaw;              // 0x0c ScenarioActorStartingLocation.facing
    uint16_t cluster_index; // 0x10 the starting location's cluster
    int8_t sequence_id;     // 0x12 read as a signed byte and passed to actor_new_and_attach_to_unit
    uint8_t flags;          // 0x13 ScenarioActorStartingLocationFlags
    int16_t return_state_override;  // 0x14 when positive replaces the squad return_state
    int16_t initial_state_override; // 0x16 when positive replaces the squad initial_state passed to
                                    //    actor_new_and_attach_to_unit in actor_place_new_unit
    uint16_t actor_type;    // 0x18 the starting location's palette entry override
    uint16_t command_list;  // 0x1a passed through as actor_new_and_attach_to_unit's command list word (-1 for none)
    int16_t unknown_1c;     // 0x1c UNSURE, actor.unknown_62 default
} actor_placement_request; // size 0x1e, the layout of ScenarioActorStartingLocation (0x1c) plus a trailing word

// The block actor_reset_perception_scratch @0x41d3b0 zeroes.
typedef struct actor_perception_request {
    uint8_t flag_a;         // 0x00
    uint8_t flag_b;         // 0x01
    uint16_t unknown_02;    // 0x02
    int16_t unknown_04;     // 0x04
    int16_t unknown_06;     // 0x06
    int16_t unknown_08;     // 0x08
    real_vector3d origin;   // 0x0a UNSURE offset/alignment
} actor_perception_request; // size 0x16

// The header actor_dispatch_squad_order @0x41ff40 reads off a squad order block.
typedef struct actor_squad_order_header {
    uint8_t unknown_00[0x14]; // 0x00
    int16_t type;             // 0x14
    uint8_t unknown_16[2];    // 0x16
    datum_index squad_prop_index; // 0x18 order type 3: the prop whose owner the order is relayed to
} actor_squad_order_header; // size 0x1c, only verified up to 0x1b

// ---------------------------------------------------------------------------
// caller-owned scratch records
// These are stack records that one function fills and another reads; none of them is a
// datum array element. They were each recovered inside a single file during the phase-4
// rewrite and are collected here so every file that touches one agrees on the layout.
// ---------------------------------------------------------------------------

typedef struct ai_reference_squad_iterator {
    int32_t encounter_index; // 0x00
    int32_t platoon_filter;  // 0x04
    int32_t cursor;          // 0x08
    int32_t squad_start;     // 0x0c
    int32_t squad_end;       // 0x10
} ai_reference_squad_iterator; // size 0x14

typedef struct ai_reference_platoon_range {
    int32_t encounter_index;
    int32_t platoon_start;
    int32_t platoon_end;
} ai_reference_platoon_range;

typedef struct path_find_boundary_crossing {
    uint8_t found;
    uint8_t unknown_01[3];
    real_point3d position;
    int32_t edge_a;
    int32_t edge_b;
    float fraction;
} path_find_boundary_crossing;

// One path waypoint as path_find_reconstruct_path (0x43a4d0) collects them from the node chain and
// path_find_simplify_waypoints (0x43cc00) / ai_navigate_around_obstacles (0x43be90) pass them on.
typedef struct path_find_waypoint {
    int32_t surface_index;  // 0x00 the collision surface the point lies on, -1 for none
    real_point3d position;  // 0x04
} path_find_waypoint;       // size 0x10

// The path record path_find_reconstruct_path fills. It overlays the actor's movement_action_complete .. waypoint
// run (+0x4a8 .. +0x504), so the movement code reads the finished path out of the actor itself.
typedef struct path_find_result {
    uint8_t found;                   // 0x00 actor.movement_action_complete
    uint8_t unknown_01[3];           // 0x01
    real_point3d end_point;          // 0x04 actor.path_end_point
    int32_t end_surface_index;       // 0x10
    float remaining_distance;        // 0x14 actor.path_remaining_distance
    uint8_t valid;                   // 0x18
    int8_t waypoint_count;           // 0x19 actor.waypoint_count
    uint8_t unknown_1a[2];           // 0x1a
    path_find_waypoint waypoints[4]; // 0x1c the 16-byte waypoint records actor.waypoint_cursor indexes
} path_find_result;                  // size 0x5c

typedef struct path_find_adjacent_edge {
    int32_t edge_id;        // 0x00 the neighboring vertex id (despite the name every caller uses it as a vertex, not an edge)
    uint8_t flag;           // 0x04
    uint8_t unknown_05[3];  // 0x05
    float start_x;          // 0x08
    float start_y;          // 0x0c
    float start_z;          // 0x10
    float direction_x;      // 0x14
    float direction_y;      // 0x18
    float direction_z;      // 0x1c
} path_find_adjacent_edge; // size 0x20

typedef struct actor_prop_iterator {
    datum_index current; // 0x00
    datum_index next;    // 0x04
} actor_prop_iterator;

typedef struct path_find_simplify_scratch {
    uint8_t unknown_00[8];
    real_point3d point_a; // UNSURE: local_70/6c/68 region
    float unknown_1c;
    uint8_t unknown_20[8];
    uint8_t unknown_28[24];
    uint8_t unknown_40[4];
    real_point3d point_b;
    int32_t vertex_id;
    uint8_t unknown_58[20];
    int32_t result;
} path_find_simplify_scratch;

typedef struct ai_search_nearest_point_result {
    float distance;    // 0x00
    int16_t point_id;  // 0x04
    int16_t link;       // 0x06
} ai_search_nearest_point_result;

// ai_search_evaluate_edge_cost @0x43b830 fills it (0x43b841..0x43b850 seed it, 0x43ba57 writes
// +0x04 last). The two dwords were read as float "headings"; the disassembly moves only
// path_find_boundary_trace_result indices into them.
typedef struct ai_search_edge_result {
    float cost;             // 0x00
    int32_t surface_index;  // 0x04 path_find_boundary_trace_result.surface_index of the final trace
    int32_t edge_index;     // 0x08 the boundary edge that set the cost, -1 when none did
    int16_t point_id;       // 0x0c ai_search_nearest_point_result.point_id, -1 when none
    int16_t link;           // 0x0e ai_search_nearest_point_result.link (0x43b9f8 WORD copy)
} ai_search_edge_result;    // size 0x10

typedef struct ai_platoon_condition {
    int16_t code;           // 0x00 1..9 select the case below; anything else is always false
    int16_t platoon_index;  // 0x02 an encounter_platoon_state index, or out of range to mean: use the totals of the encounter itself
} ai_platoon_condition;

typedef struct ai_path_candidate_goal {
    uint8_t valid;              // 0x00
    uint8_t unknown_01[3];      // 0x01
    real_point3d position;      // 0x04
    uint32_t unknown_10;        // 0x10 set to 0xffffffff on success
    uint32_t unknown_14;        // 0x14 set to 0 on success
    uint8_t reachable;          // 0x18 out_success of path_find_test_direct_reachability
    uint8_t flag_19;            // 0x19 set to 1 on success
    uint8_t flag_1a;            // 0x1a set to 0 on success
    uint8_t unknown_1b;         // 0x1b
    uint32_t unknown_1c;        // 0x1c set to 0xffffffff on success
    real_point3d alt_position;  // 0x20 out_position of path_find_test_direct_reachability
    uint8_t unknown_2c[0x30];   // 0x2c zeroed, never independently written here
} ai_path_candidate_goal; // size 0x5c

typedef struct ai_nearby_actor_candidate {
    datum_index actor_index;
    float distance_squared;
    uint8_t is_type_9;
    uint8_t pad[3];
} ai_nearby_actor_candidate; // size 0xc, matches the stride of object_sort_by_flag_then_distance

typedef struct ai_conversation_speech_request {
    int16_t priority;             // 0x00
    int16_t scream_type;          // 0x02
    datum_index sound_tag;        // 0x04
    int16_t delay_ticks;          // 0x08
    int16_t lipsync_ticks;        // 0x0a
    int16_t tail_ticks;           // 0x0c
    int16_t unknown_0e;           // 0x0e
    int32_t unknown_10;           // 0x10
    int16_t unknown_14;           // 0x14
    int16_t ai_line_index;        // 0x16
    int16_t unknown_18;           // 0x18
    uint8_t suppress_line_record; // 0x1a
    uint8_t unknown_1b;           // 0x1b
    int16_t unsure_flag_a;        // 0x1c UNSURE: always 1 at this call site
    int16_t unsure_flag_b;        // 0x1e UNSURE: always 1 at this call site
    datum_index unsure_unit_index;// 0x20 UNSURE: the unit this speech plays on
    uint8_t unknown_24[12];       // 0x24
} ai_conversation_speech_request; // size 0x30

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// global 0x00880354: ai_globals *ai_globals             0x8dc bytes out of the game state
// global 0x00880360: data_array *actor_data             stride 0x724, capacity 0x100
// global 0x0088035c: data_array *swarm_data             stride 0x98, capacity 0x20
// global 0x00880358: data_array *swarm_component_data   stride 0x40, capacity 0x100
// global 0x008802c0: data_array *prop_data              stride 0x138, capacity 0x300
// global 0x008802c8: data_array *encounter_data         stride 0x6c, capacity 0x80
// global 0x008802cc: encounter_squad_state *encounter_squad_states      0x8000 bytes
// global 0x008802c4: encounter_platoon_state *encounter_platoon_states  0x1000 bytes
// global 0x008802d0: data_array *ai_pursuit_data        stride 0x28, capacity 0x100
// global 0x008802d4: data_array *ai_conversation_data   stride 0x64, capacity 8
//
// The direction-sample tables FUN_0041a2d0 (called once from ai_initialize_for_new_map)
// precomputes for the obstacle-avoidance sampler. The bases and strides come from the
// pointer arithmetic in that function; the three runs are contiguous and do not overlap.
// global 0x00880380: float actor_avoidance_samples_a[16][7]   stride 0x1c, 0x880380..0x88053f
// global 0x00880540: float actor_avoidance_circle[8][3]       stride 0x0c, 0x880540..0x88059f
// global 0x008805a0: float actor_avoidance_samples_b[9][7]    stride 0x1c, 0x8805a0..0x88069b
//   Each 7-float row is { 1.0 or 0.7, 0.0, plane i, plane j, cos(elevation),
//   sin(elevation)*i, sin(elevation)*j }; the source angles and radii are the .rdata
//   constants at 0x006556a0, 0x006556c8, 0x006556ec, 0x00655714, 0x00655734 and 0x0065573c.
//
// Globals this module reads but does not own, listed so the ownership stays honest:
//   0x008603b0 data_array *object_data                (types/objects.h)
//   0x0087bc14 tag_instance *tag_instances            (types/cache.h)
//   0x00746f8c global_scenario                        the scenario tag data
//   0x00746f98 ModelCollisionGeometryBSP *global_structure_collision_bsp (the structure BSP
//              +0xb4) and 0x00746f9c ScenarioStructureBSP *global_structure_bsp (the resident
//              structure BSP tag data), both scenario.h; neither is an index or a counter
//   0x006f1d6c the game time globals, current tick at +0x0c
//   0x006e2dc8 / 0x006e2dcc / 0x006e2dd4 the game-state bump allocator and its crc
//   0x00719cd0 random_seed_global                     (types/math.h)
//   0x00696714 / 0x00696718 the shared zero vectors   (types/math.h)
//   0x00655254 the actor mode definition table and 0x006853b8 the per-type vtable table
//   0x006f0c98 / 0x006f0c9c and 0x006f0ca0 / 0x006f0ca4 the two communication timestamp
//              tables 0x42d230 resets (count and base, 8 bytes per entry, two per index)
#pragma pack(pop)
