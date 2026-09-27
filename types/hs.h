// Blam hs module (halo.exe 1.0.10 retail, 0x482b20..0x48b340, 125 functions).
// HaloScript: the compiler (tokenizer, parser, postprocess pass), the runtime (threads,
// globals, the evaluation stack) and the object-list container that the object_list script
// type is built on. Offsets in comments are byte offsets from the struct base and were
// recovered from the decompiled module, from raw instruction bytes where Ghidra lost a
// register argument, and from direct reads of the .rdata/.data tables in bin/halo.exe
// (PE VA -> file offset: .rdata 0x63a000 -> 0x23a000, .data 0x676000 -> 0x276000).
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here (see out/phase4/hs_types_notes.md for which function proved which):
//   types/memory.h   data_array, datum_index, data_iterator, memory_pool
//   types/tags.h     Scenario (script_syntax_data 0x474, script_string_data 0x488,
//                    scripts 0x49c, globals 0x4a8, references 0x4b4, source_files 0x4c0,
//                    object_names 0x204, cutscene_flags 0x4e4), ScenarioScript,
//                    ScenarioGlobal, ScenarioReference, ScenarioSourceFile,
//                    ScenarioObjectName, ScenarioCutsceneFlag, ScenarioScriptNode,
//                    ScenarioScriptNodeTable, ScenarioScriptType, ScenarioScriptValueType
//   types/cache.h    tag_instance (0x0087bc14)
//   types/math.h     random_seed (0x00719cd0 random_seed_global, used by hs_evaluate_random)
//
// hs_type below is the engine-side (snake_case, Bungie) spelling of the same 49 values that
// types/tags.h exports as ScenarioScriptValueType, and hs_syntax_node is the runtime view of
// the same 20 bytes that types/tags.h exports as ScenarioScriptNode. They are declared again
// here, the way math.h re-declares real_point3d, so that hs.h reads in the vocabulary the
// module itself uses and parses standalone.
//
// Note on sizes: like types/memory.h, structs holding pointers only measure to the documented
// size under a 32-bit data organization.
//
// The last section of this header ("Slices of records other, not-yet-type-recovered modules own")
// is deliberately temporary: partial views of the objects / units / game / effects / files
// records that hs reaches into. They carry the hs_ prefix so the real definitions can land in
// those modules own headers later without colliding. The one exception is file_reference, which
// is opaque here and keeps its real name because hs never names a field inside it.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module-wide constants
// ---------------------------------------------------------------------------
typedef enum hs_limits {
    k_hs_type_count = 0x31,                  // hs_find_function_by_name / the 0x31-wide tables
    k_hs_script_type_count = 5,              // startup, dormant, continuous, static, stub
    k_hs_function_count = 0x20a,             // loop bound in hs_find_function_by_name and hs_doc
    k_hs_builtin_global_count = 0x1eb,       // chimera__get_global_index, hs_runtime_initialize
    k_hs_syntax_node_maximum_count = 0x4a39, // data_new("script node", 0x4a39)
    k_hs_syntax_node_table_size = 0x5ccac,   // 0x38 + 0x4a39*0x14, the tag on scenario+0x474
    k_hs_thread_maximum_count = 0x100,       // game_state_new("hs thread", 0x100)
    k_hs_global_maximum_count = 0x400,       // game_state_new("hs globals", 0x400)
    k_hs_thread_stack_size = 0x200,          // 0x218 - 0x18
    k_hs_maximum_arguments = 0x20,           // the 32-slot argument buffers
    k_hs_maximum_random_children = 0x40,     // two words of chosen bits in hs_evaluate_random
    k_hs_maximum_name_length = 0x20,         // script and global names must be under 32 chars
    k_hs_maximum_expression_length = 0x400,  // hs_compile_expression refuses 0x400 or more
    k_hs_error_buffer_size = 0x100,          // 0x006b15dc - 0x006b14dc
    k_hs_object_list_header_count = 0x30,    // game_state_new("object list header", 0x30)
    k_hs_object_list_reference_count = 0x80  // game_state_new("list object reference", 0x80)
} hs_limits;

// A packed reference to a global variable, as produced by chimera__get_global_index @0x483480
// and consumed by hs_global_get_name @0x483450, hs_global_get_type @0x483420,
// hs_global_get_value @0x48a720, hs_global_read_value and hs_global_write_value.
// Bit 15 set means an engine builtin, indexing hs_global_definitions; clear means a
// scenario-defined global, indexing Scenario::globals. 0xffff means unknown.
typedef enum hs_global_reference_bits {
    k_hs_global_builtin_bit = 0x8000,
    k_hs_global_index_mask = 0x7fff,
    k_hs_global_reference_none = 0xffff
} hs_global_reference_bits;
typedef uint16_t hs_global_reference;

// ---------------------------------------------------------------------------
// hs_type  (== ScenarioScriptValueType in types/tags.h)
// Proved by hs_type_names at 0x00688a78, whose 0x31 strings are in exactly this order.
// The compiler treats 4..0x30 as the "real" value types (hs_add_global, hs_add_script and
// hs_compile_postprocess all reject anything outside that range), 0x25..0x2a as the object
// family and 0x2b..0x30 as the object-name family (hs_types_are_compatible @0x48ac90).
// ---------------------------------------------------------------------------
typedef enum hs_type {
    _hs_type_unparsed = 0,
    _hs_type_special_form = 1,
    _hs_type_function_name = 2,
    _hs_type_passthrough = 3,       // always compatible with everything
    _hs_type_void = 4,
    _hs_type_boolean = 5,
    _hs_type_real = 6,
    _hs_type_short = 7,
    _hs_type_long = 8,
    _hs_type_string = 9,
    _hs_type_script = 10,
    _hs_type_trigger_volume = 11,
    _hs_type_cutscene_flag = 12,
    _hs_type_cutscene_camera_point = 13,
    _hs_type_cutscene_title = 14,
    _hs_type_cutscene_recording = 15,
    _hs_type_device_group = 16,
    _hs_type_ai = 17,
    _hs_type_ai_command_list = 18,
    _hs_type_starting_profile = 19,
    _hs_type_conversation = 20,
    _hs_type_navpoint = 21,
    _hs_type_hud_message = 22,
    _hs_type_object_list = 23,
    _hs_type_sound = 24,
    _hs_type_effect = 25,
    _hs_type_damage = 26,
    _hs_type_looping_sound = 27,
    _hs_type_animation_graph = 28,
    _hs_type_actor_variant = 29,
    _hs_type_damage_effect = 30,
    _hs_type_object_definition = 31,
    _hs_type_game_difficulty = 32,
    _hs_type_team = 33,
    _hs_type_ai_default_state = 34,
    _hs_type_actor_type = 35,
    _hs_type_hud_corner = 36,
    _hs_type_object = 37,
    _hs_type_unit = 38,
    _hs_type_vehicle = 39,
    _hs_type_weapon = 40,
    _hs_type_device = 41,
    _hs_type_scenery = 42,
    _hs_type_object_name = 43,
    _hs_type_unit_name = 44,
    _hs_type_vehicle_name = 45,
    _hs_type_weapon_name = 46,
    _hs_type_device_name = 47,
    _hs_type_scenery_name = 48
} hs_type;
typedef int16_t hs_type_t;

// == ScenarioScriptType in types/tags.h; the names are hs_script_type_names at 0x00688b3c,
// and hs_add_script only accepts indices 0..4 out of that list.
typedef enum hs_script_type {
    _hs_script_startup = 0,
    _hs_script_dormant = 1,
    _hs_script_continuous = 2,
    _hs_script_static = 3,      // callable from an expression
    _hs_script_stub = 4         // callable, may be overridden by a static of the same type
} hs_script_type;
typedef int16_t hs_script_type_t;

// Indices into hs_function_definitions that the compiler special cases.
// hs_parse_nonprimitive rejects 0x13/0x14 when hs_blocking_forbidden is set and 4 when
// hs_set_forbidden is set; hs_compile_expression wraps a bare expression in index 0x16.
typedef enum hs_special_function_index {
    _hs_function_begin = 0,
    _hs_function_set = 4,
    _hs_function_sleep = 0x13,
    _hs_function_sleep_until = 0x14,
    _hs_function_inspect = 0x16
} hs_special_function_index;

// Bit index of a console command CONTEXT in hs_function_definition::gametype_flags,
// hs_global_definition::gametype_flags and hs_autocomplete_gametype_mask (R31). These are NOT
// multiplayer game types: they are the bits of the context word console_command_context_flags
// 0x4c69c0 builds, which console_autocomplete_command passes on (0x4c6b38 call 0x4c69c0 ->
// 0x4c6b4c mov edx,eax -> 0x483cc1 mov WORD ds:0x6b14ac,dx). 0x4c69c0 starts from bit 0; with a
// multiplayer engine loaded (0x006f1d20 != NULL) it uses 0x2009 (bits 0, 3; bit 5 forbidden),
// otherwise bit 4, plus bit 5 unless a profile flag byte (bit 2 of the byte 0x124 into the 0x2000
// block copied from 0x00712dd8) says otherwise; it always ORs bit 6; network_game_mode 1
// (client) forbids bits 1 and 2 (0x600), mode 2 (host) sets bit 1; then the caller's word is
// ORed in and every bit n whose bit n+8 is set is cleared. Bit 0 is handled separately by
// hs_gametype_flags_applicable @0x483600, which then tests bits 1..6 (0x4835b0).
typedef enum hs_gametype_flags {
    _hs_context_default_bit = 0,               // always seeded by 0x4c69c0
    _hs_context_host_bit = 1,                  // network_game_mode == 2 (host); forbidden on a client
    _hs_context_client_forbidden_bit = 2,      // forbidden on a client (0x400); only set by the caller
    _hs_context_multiplayer_engine_bit = 3,    // current_game_engine != NULL
    _hs_context_no_multiplayer_engine_bit = 4, // current_game_engine == NULL
    _hs_context_unknown_20_bit = 5,            // credits / profile-gated (UNSURE); forbidden with a
                                               //   multiplayer engine loaded
    _hs_context_always_bit = 6                 // ORed unconditionally (0x4c6a16)
} hs_gametype_flags;

// hs_autocomplete_gametype_mask (0x006b14ac) is the console command context word 0x4c69c0
// builds (R31): the low byte holds the context bits above that are present, the high byte the
// ones that are forbidden -- the two tests in hs_gametype_flag_satisfied @0x4835b0 (movzx
// edi,WORD ds:0x6b14ac) and hs_gametype_flags_applicable @0x483600 (bits 0..6).
typedef enum hs_autocomplete_gametype_mask_bits {
    k_hs_autocomplete_required_mask = 0x00ff,
    k_hs_autocomplete_forbidden_shift = 8
} hs_autocomplete_gametype_mask_bits;

// ---------------------------------------------------------------------------
// hs_syntax_node  (0x14)
// The compiled expression tree. One datum in hs_syntax_data; the same 20 bytes the scenario
// tag carries as ScenarioScriptNode inside script_syntax_data, which is why
// hs_allocate_script_node_table swaps the whole data_array in and out of Scenario+0x480 and
// stamps k_hs_syntax_node_table_size into Scenario+0x474.
// Offsets come from the *0x14 stride arithmetic in hs_parse_if, hs_parse_set,
// hs_parse_cond_recursive, hs_tokenize, hs_tokenize_nonprimitive, hs_parse_primitive,
// hs_parse_variable, hs_resolve_identifier_as_function_or_script, hs_compile_postprocess and
// hs_syntax_node_garbage_collect.
// ---------------------------------------------------------------------------
typedef enum hs_syntax_node_flags {
    _hs_syntax_node_primitive_bit = 1,           // set by hs_tokenize when the token is not "("
    _hs_syntax_node_script_call_bit = 2,         // index is a script index, not a function index
    _hs_syntax_node_global_bit = 4,              // set by hs_parse_variable; data is a global ref
    _hs_syntax_node_garbage_collectable_bit = 8, // hs_syntax_node_garbage_collect keeps only these
    _hs_syntax_node_local_variable_bit = 0x10
} hs_syntax_node_flags;

typedef union hs_syntax_node_data {
    uint8_t boolean_value;        // _hs_type_boolean
    int16_t short_value;          // _hs_type_short and every 2-byte scenario index
    int32_t long_value;           // _hs_type_long
    float real_value;             // _hs_type_real, written by hs_parse_real
    int32_t string_value;         // _hs_type_string, a char * into hs_compiled_source
    int16_t global_reference;     // written by hs_parse_variable when the global bit is set
    int16_t scenario_index;       // trigger volume / cutscene flag / object name / ...
    datum_index tag_reference;    // written by hs_parse_tag_reference from a ScenarioReference
    datum_index first_child;      // non-primitive: head of the child list
} hs_syntax_node_data;            // size 0x04

typedef struct hs_syntax_node {
    int16_t identifier;           // 0x00 datum_header
    int16_t index_union;          // 0x02 constant type, function index, or script index;
                                  //      hs_tokenize seeds it with 0xffff
    hs_type_t type;               // 0x04 0 until parsed; hs_parse writes the expected type here
    uint16_t flags;               // 0x06 hs_syntax_node_flags
    datum_index next_node;        // 0x08 next sibling, k_datum_index_none at the end of a list
    int32_t source_offset;        // 0x0c byte offset into hs_compiled_source, -1 when unknown
    hs_syntax_node_data data;     // 0x10
} hs_syntax_node;                 // size 0x14

// ---------------------------------------------------------------------------
// hs_function_definition  (0x1c + 2*parameter_count, packed in .rdata)
// One per script function. hs_function_definitions at 0x00688b58 holds 0x20a pointers into a
// run of these records starting at 0x00657660; the record stride there is exactly
// 0x1c + 2*parameter_count rounded up to 4.
// Field evidence: hs_format_function_signature @0x484300 (name at +4, param_info at +0x14,
// parameter_count at +0x1a, parameters at +0x1c), hs_help_print_function and hs_doc (info at
// +0x10), hs_parse_nonprimitive @0x486710 (return_type at +0, parse at +8),
// hs_thread_evaluate @0x48a370 (evaluate at +0xc), chimera__autocomplete_scan_globals /
// hs_gametype_flags_applicable (gametype_flags at +0x18), and a sweep of all 0x20a records.
// ---------------------------------------------------------------------------
typedef struct hs_function_definition {
    hs_type_t return_type;        // 0x00 _hs_type_passthrough for begin/if/cond/set
    int16_t unknown_02;           // 0x02 zero in all 0x20a records
    char *name;                   // 0x04
    void *parse;                  // 0x08 char (*)(int16_t function_index, datum_index node)
    void *evaluate;               // 0x0c void (*)(int16_t index, datum_index thread, char first)
                                  //      NULL for cond, which is desugared at parse time
    char *info;                   // 0x10 documentation sentence, never NULL
    char *param_info;             // 0x14 hand written argument list, NULL for 480 of 522;
                                  //      when NULL the signature is built from parameters[]
    int16_t gametype_flags;       // 0x18 hs_gametype_flags console-context bitmask (R31, not
                                  //      game types), 0 means always available
    int16_t parameter_count;      // 0x1a
    hs_type_t parameters[1];      // 0x1c parameter_count entries
} hs_function_definition;         // size 0x1c + 2*parameter_count

// ---------------------------------------------------------------------------
// hs_global_definition  (0x10)
// One per engine builtin global. hs_global_definitions at 0x0068b398 holds 0x1eb pointers.
// Proved by hs_global_read_value @0x48aec0 and hs_global_write_value @0x48b030 (type at +4
// selects the switch, address at +8 is the engine variable) plus a sweep of all 0x1eb records:
// only types 5..8 occur, 102 of them carry a non-NULL address, and +0x0c is nonzero on six.
// ---------------------------------------------------------------------------
typedef struct hs_global_definition {
    char *name;                   // 0x00 hs_global_get_name @0x483450 returns exactly this
    hs_type_t type;               // 0x04 only boolean/real/short/long occur
    int16_t pad_06;               // 0x06 zero in all 0x1eb records
    void *address;                // 0x08 the engine variable, NULL for 389 of the 491
    uint32_t gametype_flags;      // 0x0c hs_gametype_flags console-context bitmask (R31), 0 on
                                  //      all but six records
                                  //      (0x5f on five, 0x15 on sv_public)
} hs_global_definition;           // size 0x10

// ---------------------------------------------------------------------------
// hs_global  (0x08)
// The runtime storage datum, one per global. hs_globals_data holds k_hs_global_maximum_count
// of them; hs_runtime_initialize reserves indices 0..0x1ea for the builtins with
// datum_new_at_index_with_salt, and a scenario global with index i lives at 0x1eb + i
// (hs_global_get_value @0x48a720). Stride 8 and the value at +4 come from that function,
// hs_global_read_value and hs_global_write_value.
// ---------------------------------------------------------------------------
typedef union hs_global_value {
    uint8_t boolean_value;        // _hs_type_boolean
    int16_t short_value;          // every 2-byte type; 0xffff when the engine address is NULL
    int32_t long_value;
    float real_value;
    char *string_value;           // defaults to hs_empty_string
    datum_index datum_value;      // object / object_list / ...; -1 when the address is NULL
} hs_global_value;                // size 0x04

typedef struct hs_global {
    int16_t identifier;           // 0x00 datum_header
    int16_t unknown_02;           // 0x02 never read or written by this module
    hs_global_value value;        // 0x04
} hs_global;                      // size 0x08

// ---------------------------------------------------------------------------
// hs_stack_frame  (0x10 header, scratch follows)
// One activation record per syntax node under evaluation, bump allocated inside the owning
// the owning thread stack. hs_thread_push @0x48a560 does
//   new_frame = (uint8 *)frame + 0x10 + frame->size;
//   new_frame->previous = frame; thread->stack = new_frame; new_frame->size = 0;
// while every evaluate handler carves scratch with
//   p = (uint8 *)frame + 0x0e + frame->size; frame->size += bytes;
// so the scratch area starts at +0x0e and the next frame is placed two bytes past its end.
// hs_thread_pop_frame @0x48a770 restores thread->stack = frame->previous.
// ---------------------------------------------------------------------------
typedef struct hs_stack_frame {
    struct hs_stack_frame *previous; // 0x00 the root frame stores 0 here
    datum_index syntax_node;         // 0x04 the node this frame is evaluating
    void *result_address;            // 0x08 where the child of THIS frame stores its result.
                                     //      hs_thread_push @0x48a560 writes it on the frame it is
                                     //      suspending, just before creating the frame for the child,
                                     //      and hs_thread_return @0x48a640 stores through
                                     //      thread->stack->previous->result_address -- so a
                                     //      parent with several children rewrites this one slot
                                     //      before each of them. The +8 of a freshly pushed frame
                                     //      is left uninitialized until it pushes a child.
    int16_t size;                    // 0x0c bytes of scratch handed out so far
    uint8_t scratch[2];              // 0x0e scratch area, size bytes long
} hs_stack_frame;                    // size 0x10 header including two bytes of scratch

// ---------------------------------------------------------------------------
// hs_thread  (0x218)
// Stride 0x218 in every runtime function. hs_thread_new @0x48a2f0 writes type/script_index/
// flags/wake_tick and points stack at &stack_data; hs_thread_evaluate @0x48a370 compares
// thread->stack against &stack_data to detect an empty stack; hs_evaluate_expression
// @0x48a250 reads the result from +0x14 (lea ebx,[esi+0x14] at 0x48a293 is the destination it
// hands hs_thread_push); hs_evaluate_sleep writes wake_tick at +8; hs_thread_restart
// @0x48a790 swaps +8 with +0xc under flag bit 1.
// ---------------------------------------------------------------------------
typedef enum hs_thread_type {
    _hs_thread_script = 0,      // script_index names a Scenario::scripts entry
    _hs_thread_global = 1,      // the transient thread that evaluates global initializers
    _hs_thread_command = 2      // console or hs_evaluate_expression; deleted when it finishes
} hs_thread_type;

typedef enum hs_thread_flags {
    _hs_thread_pushed_frame_bit = 1, // set by hs_thread_push, cleared before each evaluate step
    _hs_thread_wake_saved_bit = 2    // hs_thread_restart restores wake_tick from saved_wake_tick
} hs_thread_flags;

typedef struct hs_thread {
    int16_t identifier;           // 0x00 datum_header
    uint8_t type;                 // 0x02 hs_thread_type
    uint8_t flags;                // 0x03 hs_thread_flags
    int32_t script_index;         // 0x04 index into Scenario::scripts, -1 when there is none
    int32_t wake_tick;            // 0x08 game tick to resume at; 0 runs now, -1 parks the thread
                                  //      forever, -2 is the initial state of a dormant script
    int32_t saved_wake_tick;      // 0x0c stashed deadline, restored under the wake_saved flag
    hs_stack_frame *stack;        // 0x10 current frame; == &stack_data when the thread is idle
    int32_t result;               // 0x14 where the root frame writes the finished value
    uint8_t stack_data[512];      // 0x18 k_hs_thread_stack_size bytes of frames
} hs_thread;                      // size 0x218

// ---------------------------------------------------------------------------
// Scratch layouts carved out of the current hs_stack_frame by the evaluate handlers.
// These are not separately allocated objects: each field is a successive
// "p = frame + 0x0e + frame->size; frame->size += n" in the order shown, so the struct is
// packed and its size is the total the handler adds to frame->size. They are listed as structs
// because every re-entrant call recomputes the same addresses in the same order.
// ---------------------------------------------------------------------------
// hs_evaluate_variadic_arguments @0x48ad60 (and the identical collector at 0x489d50, which
// orders the first two fields the other way round and has no argument_count).
typedef struct hs_variadic_arguments_state {
    uint32_t evaluated_count;     // 0x00 index of the next slot in values
    int32_t values[32];           // 0x04 k_hs_maximum_arguments results
    int16_t argument_count;       // 0x84 stops the collector at k_hs_maximum_arguments
    datum_index next_node;        // 0x86 sibling still to evaluate, -1 when the list is done
} hs_variadic_arguments_state;    // size 0x8a
// Note: neither collector keeps the child value in its frame scratch. Both hand hs_thread_push
// the address of their own second/third STACK PARAMETER (0x48ae4e and 0x489e18) -- a slot that has
// already served as the `first` flag -- and read it back after the push. That only works because
// the C frame layout of the call chain repeats exactly on every resumption; see the OPEN QUESTION note
// in src/hs/hs_evaluate_variadic_arguments.c.

// FUN_00489d50, the 32-slot collector used by the fixed-shape variadic built-ins.
typedef struct hs_argument_list_state {
    datum_index next_node;        // 0x00
    int32_t count;                // 0x04
    int32_t values[32];           // 0x08
} hs_argument_list_state;         // size 0x88

// hs_evaluate_random @0x488c60. chosen is a bitmap of children already picked, so a random
// form is limited to k_hs_maximum_random_children alternatives.
typedef struct hs_random_state {
    int16_t child_count;          // 0x00 counted once on the first call
    uint32_t chosen;              // 0x02 bitmap of children already picked, indexed [i >> 5]
    int32_t child_value;          // 0x06 where the result of the picked child is written, and what
                                  //      hs_thread_return hands back once every child is used.
                                  //      The bitmap clear on the first call zeroes
                                  //      ((child_count + 31) / 32) words from +0x02, so a form
                                  //      with more than 32 alternatives clears this slot too.
} hs_random_state;                // size 0x0a

// hs_evaluate_boolean_and_or @0x489120. opcode 5 is "and", 6 is "or".
typedef struct hs_boolean_state {
    datum_index next_node;        // 0x00
    int32_t child_value;          // 0x04 only the low byte is read back
    uint8_t result;               // 0x08 seeded to 1 for and, 0 for or
} hs_boolean_state;               // size 0x09

// hs_evaluate_arithmetic_reduce @0x489250, shared by + - * / min max (opcodes 7..12).
typedef struct hs_arithmetic_state {
    int16_t term_count;           // 0x00 the first term is assigned, later ones are combined
    datum_index next_node;        // 0x02
    float child_value;            // 0x06 result slot handed to the child expression
    float accumulator;            // 0x0a
} hs_arithmetic_state;            // size 0x0e

// hs_evaluate_sleep @0x489800, shared by sleep and sleep_until.
typedef struct hs_sleep_state {
    int32_t condition;            // 0x00 only the low byte is read; the sleep_until predicate
    int32_t ticks;                // 0x04 only the low 16 bits are used; seeded to 30 (one second)
    int32_t timeout_ticks;        // 0x08 optional cap, -1 when the form has no second argument
    int32_t start_tick;           // 0x0c snapshot of the current game tick
    int16_t stage;                // 0x10 0 evaluate the count, 1 evaluate the condition
} hs_sleep_state;                 // size 0x12

// FUN_0048a850, the fixed-arity typed argument evaluator. results is parameter_count entries
// wide, so the record size depends on the function being called.
typedef struct hs_typed_arguments_state {
    int32_t results[1];           // 0x00 parameter_count entries
    /* int16_t index; */          // 0x00 + 4*parameter_count
    /* datum_index next_node; */  // 0x02 + 4*parameter_count
} hs_typed_arguments_state;       // size 4*parameter_count + 6

// ---------------------------------------------------------------------------
// object lists  (object_lists_initialize @0x48b250 creates both data_arrays)
// The backing store for the _hs_type_object_list value type. A list is a header datum with a
// singly linked chain of reference datums hanging off it.
// Layout evidence: object_list_reference_add @0x48b2a0 raw bytes
//   mov [ecx+4],edx / mov edx,[esi+8] / mov [ecx+8],edx / mov [esi+8],eax / inc word [esi+6]
// and object_lists_dispose_empty @0x48b340 raw bytes
//   cmp word [ecx+eax*4+4],0
// which is a different field from the one reference_add increments; hs_scenario_scripts_initialize
// @0x489ef0 is the one that increments +4, once per hs global that holds the list, which makes
// +4 a holder count and dispose_empty a sweep of lists no global refers to any more.
// ---------------------------------------------------------------------------
typedef struct object_list_header {
    int16_t identifier;           // 0x00 datum_header
    int16_t unknown_02;           // 0x02 never read or written by this module
    int16_t reference_count;      // 0x04 bumped by hs_scenario_scripts_initialize per holder;
                                  //      object_lists_dispose_empty deletes the list at 0
    int16_t count;                // 0x06 number of reference datums in the chain
    datum_index first_reference;  // 0x08 head of the chain, -1 when empty
} object_list_header;             // size 0x0c

typedef struct object_list_reference {
    int16_t identifier;           // 0x00 datum_header
    int16_t unknown_02;           // 0x02 never read or written by this module
    datum_index object_index;     // 0x04 the object this node refers to
    datum_index next;             // 0x08 next node, -1 at the end
} object_list_reference;          // size 0x0c

// Iterator state written by object_list_get_first @0x48b2f0 and walked by every list consumer
// (FUN_00487820, FUN_004879b0, FUN_00487ad0, FUN_00488570, FUN_00488740): a single
// datum_index holding the next reference node.
typedef datum_index object_list_iterator;

// ---------------------------------------------------------------------------
// hs_enum_definition  (0x08)
// The keyword tables the enum-ish value types parse against, used by hs_parse_enum @0x486dc0
// as (&hs_enum_definitions[-0x20])[type], i.e. the record for _hs_type_game_difficulty sits at
// 0x0065b634 and the compiler folds the -0x20 bias into the base address.
// Only five records exist, one per type in 32..36; the bytes below 0x0065b634 belong to the
// hs_function_definition run, so the array must not be indexed outside that window.
// ---------------------------------------------------------------------------
typedef struct hs_enum_definition {
    int32_t count;                // 0x00
    char **names;                 // 0x04 count strings, matched case-insensitively
} hs_enum_definition;             // size 0x08

// ---------------------------------------------------------------------------
// hs_file_enumeration  (hs_rebuild_source @0x483e20)
// hs_rebuild_source enumerates data\scripts, collecting up to eight entries of the 0x10c-byte
// file_reference record the files module owns, qsorts them with the comparator at 0x00483d20
// and concatenates every .hsc file. The record itself belongs to the files module and is not
// defined here; only the limit is an hs fact.
// ---------------------------------------------------------------------------
typedef enum hs_source_limits {
    k_hs_maximum_source_files = 8,       // the loop bound in hs_rebuild_source
    k_hs_file_reference_size = 0x10c     // the qsort element width it passes
} hs_source_limits;

// ===========================================================================
// Slices of records other, not-yet-type-recovered modules own
// ===========================================================================
// Everything below this line belongs to the objects, units, game, effects or files module.
// None of those modules has a types/*.h yet, so the hs rewrite needs *some* declaration for the
// records it reaches into. These are deliberately partial: only the offsets that an hs function
// actually touches are named, the rest is explicit padding, and the total size is the stride the
// hs code itself proves (or, where it proves no stride, the largest offset it touches).
// They carry the hs_ prefix precisely so that when objects/units/game are recovered properly,
// their real object_datum / player_datum / damage_data / game_time_globals definitions can land
// in their own headers without colliding, and these can be deleted.
// ---------------------------------------------------------------------------

// The object_headers datum owned by the objects module (data_array *object_headers @0x008603b0, stride 0x0c).
// hs_object_list_any_angle_match @0x4879b0 and hs_object_list_any_angle_match_gated @0x487ad0
// inline the validity check datum_get does over this record instead of calling datum_get.
typedef struct hs_object_header_entry {
    int16_t identifier;       // 0x00 datum_header
    uint8_t unknown_02;       // 0x02
    uint8_t type_flag;        // 0x03 object-type bitmask; the two list walkers accept bits 0|1
    uint8_t unknown_04[4];    // 0x04
    void *data;               // 0x08 the hs_object_record below
} hs_object_header_entry;     // size 0x0c

// The runtime object record owned by the objects module, the block object_headers[i].data points at.
// Only the fields the hs module reads are named:
//   0x00 / 0x04  hs_objects_delete_by_type @0x4887d0, hs_object_runtime_cleanup @0x487dd0
//   0xb4         hs_object_hierarchy_test @0x487c10 (object type, masked with 0x1c)
//   0xdc / 0xe4  hs_object_set_health_fraction @0x488600
//   0x114..0x11c hs_object_hierarchy_test, hs_object_runtime_cleanup,
//                hs_object_detach_and_place_at_location @0x487f50 (word index 0x47 == 0x11c)
//   0x1f4        hs_object_hierarchy_test
// The real record is larger than 0x1f5; this slice stops at the last offset hs touches, so it
// must only ever be used through a pointer, never by value and never as an array stride.
typedef struct hs_object_record {
    uint32_t tag_id;                     // 0x00 UNSURE: the tag reference of the object; inferred
                                         //      from the match test in hs_objects_delete_by_type only
    int32_t unknown_04;                  // 0x04 placement kind; 0 and 3 are dispatched on
    uint8_t unknown_08[0xb4 - 0x08];     // 0x08
    uint8_t type;                        // 0xb4
    uint8_t unknown_b5[0xdc - 0xb5];     // 0xb5
    float maximum_health;                // 0xdc
    uint8_t unknown_e0[0xe4 - 0xe0];     // 0xe0 unexplained gap between the two health fields
    float current_health;                // 0xe4
    uint8_t unknown_e8[0x114 - 0xe8];    // 0xe8
    datum_index sibling;                 // 0x114 next object attached to the same parent
    datum_index child;                   // 0x118 head of the attachment list
    datum_index parent;                  // 0x11c
    uint8_t unknown_120[0x1f4 - 0x120];  // 0x120
    uint8_t flags_1f4;                   // 0x1f4 bit 0 gates the type test in hs_object_hierarchy_test
} hs_object_record;                      // partial: 0x1f5 named, real record is larger

// The iterator object_iterator_next @0x4f6f20 walks. NOT a data_iterator: hs_object_runtime_cleanup
// and hs_objects_delete_by_type both seed the first word with -1 rather than a data_array pointer,
// so the shape below is inferred from those two call sites only.
typedef struct hs_object_iterator_state {
    int32_t type_filter;   // 0x00 -1 means "any object type"
    int32_t next_index;    // 0x04
    datum_index index;     // 0x08 handle of the object last returned
} hs_object_iterator_state; // size 0x0c

// The player record owned by the game module (data_array *players @0x0087a480, stride 0x200).
// hs_object_list_collect_player_units @0x487630 and
// hs_reposition_players_outside_trigger_volume @0x4884b0 read only the unit handle.
typedef struct hs_player_record {
    uint8_t unknown_00[0x34];  // 0x00
    datum_index unit;          // 0x34
} hs_player_record;            // partial: stride is 0x200

// The time globals owned by the game module (game_time_globals *game_time @0x006f1d6c) are
// types/game.h game_time_globals (0x20 bytes; game.h parses before hs.h). R32 removed the partial
// hs_game_time_globals copy that used to live here: its +0x1c "seconds_per_tick" is really
// game_time_globals.leftover_time, the fractional-tick accumulator (0x470b30: fadd [ecx+0x1c]
// at 0x470b67, fst [edx+0x1c] at 0x470bca, mov [edx+0x1c],0 at 0x470bdc), and +0x18 is the time
// scale (speed). Old name -> game.h name: initialized -> initialized, budget_flag_1 -> active,
// budget_flag_2 -> paused, current_tick -> game_time, tick_delta (int32) -> ticks_this_frame
// (int16) + unknown_12, seconds_per_tick -> leftover_time.
// hs_evaluate_sleep, hs_runtime_update, hs_thread_evaluate_step and
// hs_object_detach_and_place_at_location read the tick counter; hs_thread_evaluate_step also
// gates its per-tick budget check on bytes +0x01 (active) and +0x02 (paused).

// The area-effect request owned by the effects/damage module, built on the stack and handed to
// damage_apply_area_effect @0x4edd30 by hs_damage_apply_at_location @0x488960 and
// hs_damage_apply_with_sound @0x488a40. Both zero all 0x54 bytes first, so the size is proved;
// the field names are inferred from which offsets those two functions write.
typedef struct hs_damage_request {
    uint32_t damage_effect;          // 0x00 the jpt! tag reference
    uint8_t unknown_04[0x08 - 0x04]; // 0x04
    uint32_t causer;                 // 0x08 UNSURE, set to -1 by both callers
    uint32_t attacker;               // 0x0c UNSURE, set to -1 by both callers
    uint16_t unknown_10;             // 0x10 set to 0xffff by both callers
    uint16_t unknown_12;             // 0x12 FIXED: explicit padding. hs.h is #pragma pack(1), so
                                     //      without it every field from sound_impulse on sat 2
                                     //      bytes low and damage_apply_area_effect read a garbage
                                     //      cluster (the a10 crash in cluster_flood_fill_within_radius)
    int32_t sound_impulse;           // 0x14
    uint16_t sound_index;            // 0x18 ScenarioStructureBSPLeaf.cluster of the leaf found for
                                     //      the location (global_structure_bsp->leaves.pointer at
                                     //      +0xe4, stride 0x10, cluster at +0x08; 0x488a0a), 0xffff
                                     //      when absent. This is damage_data.location_cluster_index.
    uint16_t unknown_1a;             // 0x1a FIXED: explicit padding (pack(1) header, see unknown_12)
    Point3D position;                // 0x1c
    Point3D direction;               // 0x28 UNSURE: written with the same value as position
    uint8_t unknown_34[0x40 - 0x34]; // 0x34
    float scale_a;                   // 0x40 both callers write 1.0
    float scale_b;                   // 0x44 both callers write 1.0
    uint8_t unknown_48[0x4c - 0x48]; // 0x48
    uint16_t unknown_4c;             // 0x4c
    uint8_t unknown_4e[0x54 - 0x4e]; // 0x4e
} hs_damage_request;                 // size 0x54

// The file_reference owned by the files module (stride k_hs_file_reference_size, proved by the element width
// hs_rebuild_source @0x483e20 hands qsort). Opaque here: hs never names a field inside it, it only
// passes whole records to file_reference_exists / file_enumerate_find_next / the 0x483d20
// comparator, and stamps the two raw words 'ofil' and 0xffff into the head of a fresh one.
typedef struct file_reference {
    uint8_t opaque[0x10c];
} file_reference;                    // size 0x10c == k_hs_file_reference_size

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// runtime and compiler datum arrays (all five are plain pointers to data_array)
// global 0x0087a474: data_array *hs_syntax_data       "script node", 0x4a39 x 0x14; swapped
//                      into Scenario::script_syntax_data.pointer by hs_allocate_script_node_table
// global 0x0087a470: data_array *hs_thread_data       "hs thread", 0x100 x 0x218
// global 0x0087a46c: data_array *hs_globals_data      "hs globals", 0x400 x 0x08
// global 0x0087a464: data_array *object_list_header_data      "object list header", 0x30 x 0x0c
// global 0x0087a468: data_array *object_list_reference_data   "list object reference", 0x80 x 0x0c
// global 0x007102fc: uint8_t hs_syntax_data_is_local  1 when hs owns the GlobalAlloc block
//                      rather than the scenario tag (hs_allocate_script_node_table / hs_scripts_free)
// global 0x007102fd: uint8_t hs_preserve_token_case   hs_tokenize_primitive skips the lowercasing

// compiler state
// global 0x006b14b8: int32_t hs_compiling             set around hs_compile_source and
//                      chimera__execute_script
// global 0x006b14bc: int32_t hs_compiled_source_length  bytes in hs_compiled_source
// global 0x006b14c0: char *hs_compiled_source         every syntax node source_offset is
//                      relative to this; during postprocess it aliases Scenario+0x494
// global 0x006b14c4: uint32_t unknown_006b14c4[3]     0x006b14c4..0x006b14cf, never referenced
//                      by any of the 125 functions in this module
// global 0x006b14d0: uint8_t hs_syntax_data_dirty     set when hs_compile bailed part way
// global 0x006b14d4: char *hs_compile_error           NULL means no error yet
// global 0x006b14d8: int32_t hs_compile_error_offset  -1 when the error has no source position
// global 0x006b14dc: char hs_compile_error_buffer[0x100]  sprintf target for formatted errors
// global 0x006b15dc: uint8_t hs_compiled_source_owned  GlobalFree the buffer when done
// global 0x006b15dd: uint8_t hs_compile_release_source  release the buffer at the end of the pass
// global 0x006b15de: uint8_t hs_blocking_forbidden    rejects sleep / sleep_until
// global 0x006b15df: uint8_t hs_set_forbidden         rejects set
// global 0x006b15e0: uint8_t hs_postprocessing        hs_parse_primitive and hs_parse_variable
//                      report unknown names as errors only while this is set
// global 0x006b15e1: uint8_t unknown_006b15e1[7]      0x006b15e1..0x006b15e7, never referenced

// runtime state
// global 0x006b15e8: uint8_t hs_runtime_active        gate on hs_runtime_update and
//                      hs_evaluate_expression
// global 0x006b15ea: int16_t hs_current_thread_index  low half of the running thread handle,
//                      0xffff when nothing is running
// global 0x006b14a8: uint8_t hs_reload_pending        chimera__execute_script rebuilds and
//                      reloads all scripts when this is set

// console autocomplete state (chimera__autocomplete_gather @0x483c90 owns all six)
// global 0x006b14a0: int16_t hs_autocomplete_maximum_count
// global 0x006b14a4: char *hs_autocomplete_prefix     points at hs_empty_string when NULL
// global 0x006b14ac: uint16_t hs_autocomplete_gametype_mask  console command context word (R31),
//                      written by 0x483cc1 from 0x4c69c0's result
// global 0x006b14b0: int16_t hs_autocomplete_count
// global 0x006b14b4: char **hs_autocomplete_results   hs_autocomplete_maximum_count entries

// .rdata / .data tables
// global 0x00688a78: char *hs_type_names[0x31]                indexed by hs_type
// global 0x00688b3c: char *hs_script_type_names[5]            indexed by hs_script_type
// global 0x00688b50: char *hs_empty_string                    -> "" at 0x0065512c
// global 0x00688b58: hs_function_definition *hs_function_definitions[0x20a]
// global 0x00689380: void *hs_autocomplete_procedures[0x12]   void (*)(void), one per category
// global 0x0068b398: hs_global_definition *hs_global_definitions[0x1eb]
// global 0x0068bc10: void *hs_type_conversion_procedures[0x31][0x31]
//                      indexed [destination_type][source_type]; 70 of the 2401 slots are
//                      non-NULL and a non-NULL slot is also what hs_types_are_compatible
//                      uses as the compatibility test outside the object families
// global 0x0065b668: void *hs_parse_primitive_procedures[0x31] char (*)(datum_index node),
//                      indexed by hs_type; 0..4 are NULL, 0x0b is hs_parse_trigger_volume,
//                      0x18..0x1f are hs_parse_tag_reference, 0x20..0x24 are hs_parse_enum,
//                      0x2b..0x30 are hs_parse_object_name
// global 0x0065b634: hs_enum_definition hs_enum_definitions[5]  types 0x20..0x24, see above
// global 0x00657538: uint16_t hs_object_type_masks[6]        ffff 0003 0002 0004 0380 0040,
//                      indexed by type - _hs_type_object or type - _hs_type_object_name;
//                      the values are object-type bitmasks (biped 0, vehicle 1, weapon 2,
//                      scenery 6, machine 7, control 8, light_fixture 9)
// global 0x00657544: uint32_t hs_tag_group_for_type[8]       types 0x18..0x1f:
//                      snd! effe jpt! lsnd antr actv jpt! obje
// global 0x00657568: int16_t hs_type_value_sizes[0x31]       1/2/4 per hs_type, 0 for 0..4;
//                      adjacent to the two tables above, not referenced by any recovered
//                      function in this module (0x00487440 is the likely reader)
// global 0x0065b660: char hs_space_characters[2]             " " and a tab
// global 0x0065b664: char hs_newline_characters[2]           newline and carriage return

// ---------------------------------------------------------------------------
// globals this module reads but does not own
// ---------------------------------------------------------------------------
// global 0x00746f8c: Scenario *global_scenario           (tags / cache)
// global 0x0069e8d4: datum_index global_scenario_index   -1 when no scenario is loaded
// global 0x0087bc14: tag_instance *tag_instances         types/cache.h
// global 0x008603b0: data_array *object_headers          objects, stride 0x0c, data at +8
// global 0x006b8cb8: datum_index *object_names_to_objects  objects, 0x200 entries
// global 0x006b8cb4: memory_pool *object_memory_pool     types/memory.h
// global 0x0087a480: data_array *players                 game, stride 0x200, unit at +0x34
// global 0x006f1d6c: game_time_globals *game_time       game (types/game.h), tick at +0x0c
// global 0x00719cd0: random_seed random_seed_global      types/math.h, hs_evaluate_random
// global 0x00746f90: ModelCollisionGeometryBSP *global_collision_bsp   scenario.h: the resident
//                    bsp's collision BSP (ScenarioStructureBSP +0xb4), written with 0x00746f98 on
//                    every switch (0x53ef68..0x53ef78, 0x54103c..0x541048); damage request
//                    construction. Not global_globals: that name is the matg globals at 0x00746fa0
// global 0x00746f9c: ScenarioStructureBSP *global_structure_bsp   scenario.h, the resident
//                    structure BSP (formerly global_matg_multiplayer; the matg globals are
//                    0x00746fa0). The damage requests read its leaf block at +0xe4.
// global 0x0065512c: char hs_empty_string_storage[1]     the shared "" literal

#pragma pack(pop)
