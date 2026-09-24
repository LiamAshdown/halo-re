// Blam devices module (halo.exe 1.0.10 retail, 0x44a930..0x44c220, 19 functions).
// The device layer of the object hierarchy: the shared device_group value table every
// machine / control / light_fixture reads and writes, the device extension those three
// object types carry on top of the common object record, and the per-tick power and
// position state machines (automatic door opening, elevator rider transport, the
// interpolation toward a group value, and the effect or sound played on a state change).
//
// Offsets in comments are byte offsets from the OBJECT base for the device_* data structs
// (the same numbering the decompiled module uses) and from the struct base otherwise.
// Where the binary itself carries the layout it is used in preference to the decompiler
// and the fact is called out:
//   - The object_type_definition rows reached through the pointer table at 0x0069bfdc fix
//     every size in this header. Reading .data (PE image base 0x400000, .data VA 0x676000
//     maps to file offset 0x276000, so file offset == VA - 0x400000) gives, at +0x08 of
//     each row, the runtime object_size and, at +0x0a / +0x0c / +0x0e, the scenario
//     placement block offset, palette offset and placement stride:
//       0x0069bcc0 "machine"        mach  size 0x228  placement 0x294 / 0x2a0, stride 0x40
//       0x0069bd88 "control"        ctrl  size 0x21c  placement 0x2ac / 0x2b8, stride 0x40
//       0x0069be50 "light_fixture"  lifi  size 0x22c  placement 0x2c4 / 0x2d0, stride 0x58
//     The common object record is 0x1f4, so device_data is 0x24 bytes at object+0x1f4 and
//     each concrete type appends its own tail at object+0x218: 0x10 bytes for machine,
//     0x04 for control, 0x14 for light_fixture. Those three placement offsets are
//     Scenario.machines / Scenario.controls / Scenario.light_fixtures, and the strides are
//     sizeof(ScenarioMachine) == sizeof(ScenarioControl) == 0x40 and
//     sizeof(ScenarioLightFixture) == 0x58, all already in types/tags.h.
//   - Every tag-side layout this module touches already exists in types/tags.h and was
//     verified against the arithmetic here rather than redefined. The Device tag in
//     particular is confirmed field for field: device_a_in at 0x198 is the four-entry
//     function selector array FUN_0044ba10 walks, automatic_activation_radius at 0x21c is
//     the sphere radius device_machine_update passes to object_find_in_sphere,
//     inverse_power_acceleration_time 0x274 / inverse_power_transition_time 0x278 /
//     inverse_depowered_position_acceleration_time 0x27c /
//     inverse_depowered_position_transition_time 0x280 /
//     inverse_position_acceleration_time 0x284 / inverse_position_transition_time 0x288 /
//     delay_time_ticks 0x28c are exactly the seven rate fields device_update_change_values
//     reads, and Device is 0x290 long, which is why DeviceMachine.machine_type and
//     DeviceControl.type both sit at 0x290 and are the value the two 0x290 switches in
//     this module dispatch on.
//   - UnitFlags bit 14 is spelled cannot_open_doors_automatically in types/tags.h. That is
//     the 0x4000 test device_machine_update applies to the derived part of a nearby object
//     tag (tag data + 0x17c) before letting that object open the door, which is what
//     identifies the whole block as the automatic-activation proximity scan.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   data_array (the device_group table is a plain data_array of 8-byte
//                    elements: data at +0x34, element stride 8), datum_index, data_iterator
//   types/math.h     real_point3d (the cached elevator node position)
//   types/cache.h    tag_instance (the 0x20-byte table at 0x0087bc14; group tag at +0x00,
//                    which device_play_state_change_effect compares against 0x65666665 and
//                    0x736e6421, and tag data at +0x14)
//   types/objects.h  object (0x1f4; device_data starts immediately after it), object_header,
//                    object_type (_object_type_device_machine 7, _object_type_device_control
//                    8, _object_type_device_light_fixture 9 -- the int16 at object 0xb4 that
//                    FUN_0044c090 switches on), object_type_mask
//                    (_object_mask_device_control 0x100 is what device_frontfacing passes to
//                    object_try_and_get, and _object_mask_device 0x380 is the mask both
//                    device_group_set_value and device_group_set_value_immediate hand to
//                    object_iterator_next), object.function_in_values at 0x124 (the four
//                    floats FUN_0044ba10 fills), object.position at 0x5c, object.forward at
//                    0x74, object.bounding_center at 0xa0, object.bounding_radius at 0xac,
//                    object.vitality_flags at 0x106, object.nodes at 0x1f0 (count 0x1f0,
//                    byte offset 0x1f2, node stride 0x34)
//   types/tags.h     Object (0x17c), Device (0x290), DeviceMachine (0x324; machine_type
//                    0x290, machine_flags 0x292, collision_response 0x2e8, elevator_node
//                    0x2ea, door_open_time_ticks 0x320), DeviceControl (0x318; type 0x290,
//                    triggers_when 0x292, call_value 0x294), DeviceLightFixture (0x2d0),
//                    DeviceIn (the 0..6 selector enum FUN_0044ba10 switches on),
//                    DeviceFlags, ScenarioDeviceGroup (0x34; initial_value 0x20, flags
//                    0x24), ScenarioDeviceFlags, ScenarioMachineFlags, ScenarioControlFlags,
//                    ScenarioMachine / ScenarioControl / ScenarioLightFixture (power_group
//                    0x28, position_group 0x2a, device_flags 0x2c in all three),
//                    Scenario (device_groups 0x288, machines 0x294, controls 0x2ac,
//                    light_fixtures 0x2c4, recorded_animations 0x36c)
//
// Not part of this module. The four functions at 0x44a930, 0x44aa90, 0x44acc0 and 0x44ad20
// were assigned to devices by the address-run heuristic but are recorded-animation
// playback, not devices: 0x44a930 indexes Scenario.recorded_animations (count at scenario
// +0x36c, pointer +0x370, stride 0x40 == sizeof(ScenarioRecordedAnimation)), reads that
// record version 0x20 / unit_control_data_version 0x22 / length_of_animation 0x24 /
// recorded_animation_event_stream.pointer 0x38, and drives the two-entry decoder vtables at
// 0x00686fd8 and 0x00686fe0 whose bodies live at 0x44a550, 0x44a590, 0x44a890 and 0x44a8b0,
// inside the preceding address run that modules.json assigns to cutscene. Their playback
// record (0x64 bytes, in the data_array at 0x006b0a10) is documented in
// out/phase4/devices_types_notes.md and deliberately not defined here. 0x44ad80 is a bare
// ret used as a no-op table entry and has no types.
//
// Note on sizes: like types/memory.h, structs holding pointers only measure to the
// documented size under a 32-bit data organization.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum device_constants {
    k_device_group_index_none = -1,          // every group index in this module is a raw
                                             // int16 slot index, compared against 0xffff,
                                             // never a datum_index with a salt
    k_maximum_device_functions = 4,          // FUN_0044ba10 loops Device.device_a_in..d_in
                                             // four times into object.function_in_values
    k_device_machine_activation_period = 4,  // the proximity scan runs on the ticks where
                                             // (game_time + object_index) & 3 == 0
    k_device_machine_activation_maximum = 0x10,   // object_find_in_sphere result buffer for
                                                  // the automatic-open scan
    k_device_machine_rider_maximum = 0x800,       // object_find_in_sphere result buffer for
                                                  // the elevator rider sweep
    k_device_machine_open_grace_ticks = -3,       // ticks_since_fully_open is reset to -3,
                                                  // not 0, when the scan opens the door
    k_device_state_change_tag_effect = 0x65666665,  // tag_instance group tag the state-change
                                                    // dispatcher treats as an effect
    k_device_state_change_tag_sound = 0x736e6421    // and the one it treats as a sound
} device_constants;
// Device group values are always clamped to the closed range 0.0 .. 1.0; both setters do
// the clamp themselves before touching the table. 0.5 is the threshold
// device_change_power_state uses to decide which way a toggle switch flips and which of the
// two state-change tags device_update_change_values plays.

// device_group.flags, the uint16 at device_group + 0x02.
typedef enum device_group_flags {
    _device_group_can_change_only_once_bit = 0,  // seeded from ScenarioDeviceGroup.flags bit
                                                 // 0 by device_groups_initialize, and from
                                                 // ScenarioDeviceFlags can_change_only_once
                                                 // (bit 2) by device_new
    _device_group_changed_bit = 1,               // set by device_group_set_value on every
                                                 // accepted change. The pair
                                                 // (can_change_only_once AND changed) is the
                                                 // locked test device_group_set_value,
                                                 // FUN_0044c0c0 and FUN_0044ba10 all apply
    _device_group_object_created_bit = 2         // set only by device_new, on both groups it
                                                 // allocates for a device placed without a
                                                 // scenario group. UNSURE: no function in
                                                 // this module reads it back, so the meaning
                                                 // is inferred from who sets it
} device_group_flags;

// device_data.flags, the uint32 at object + 0x1f4. Bits 0 and 1 are copied out of
// ScenarioDeviceFlags by device_new; bit 2 is pure runtime bookkeeping.
typedef enum device_flags {
    _device_position_reversed_bit = 0,           // ScenarioDeviceFlags bit 3
    _device_not_usable_from_any_side_bit = 1,    // ScenarioDeviceFlags bit 4; FUN_0044c0c0
                                                 // refuses a position change while it is set
    _device_position_changed_bit = 2             // raised by device_update_change_values,
                                                 // device_group_set_value_immediate and the
                                                 // gear branch of device_machine_update
                                                 // whenever a cached value moved; cleared at
                                                 // the end of device_machine_update after it
                                                 // pushes object.position back out
} device_flags;

// device_data.type_flags, the uint32 at object + 0x214, for a device_machine. Same layout
// as ScenarioMachineFlags in types/tags.h, which is where it is copied from.
typedef enum device_machine_flags {
    _device_machine_does_not_operate_automatically_bit = 0,  // gates the whole proximity scan
    _device_machine_one_sided_bit = 1,                       // when set and the door is shut,
                                                             // only objects in front of the
                                                             // device forward vector count
    _device_machine_never_appears_locked_bit = 2,            // forces the locked function
                                                             // output back to 0.0
    _device_machine_opened_by_melee_attack_bit = 3           // the only bit FUN_0044b5d0 tests
                                                             // before slamming the position
                                                             // group to 1.0
} device_machine_flags;

// device_data.type_flags for a device_control. Same layout as ScenarioControlFlags.
typedef enum device_control_flags {
    _device_control_usable_from_both_sides_bit = 0           // device_frontfacing skips the
                                                             // front-marker dot product test
                                                             // entirely when it is set
} device_control_flags;

// ---------------------------------------------------------------------------
// the shared device group table
// ---------------------------------------------------------------------------
// One element of the data_array at 0x0087abf0. Element stride 8 is fixed by every indexing
// expression in the module, which is uniformly
// *(*(int *)(device_groups + 0x34) + group_index * 8 + field). Field 0x00 is the datum
// header identifier datum_new writes; nothing in this module reads it, because device code
// addresses groups by raw slot index rather than by datum_index.
typedef struct device_group {
    int16_t identifier;             // 0x00 datum salt, written by datum_new
    uint16_t flags;                 // 0x02 device_group_flags
    float value;                    // 0x04 clamped to 0.0 .. 1.0 by both setters
} device_group;                     // size 0x08

// The three device placement records in the scenario (ScenarioMachine, ScenarioControl and
// ScenarioLightFixture) all carry this same 8-byte slice at offset 0x28, and device_new is
// handed a pointer straight at it rather than at the whole placement record.
typedef struct device_placement_data {
    int16_t power_group;            // 0x00 ScenarioMachine 0x28, -1 means allocate one
    int16_t position_group;         // 0x02 ScenarioMachine 0x2a, -1 means allocate one
    uint32_t flags;                 // 0x04 ScenarioMachine 0x2c, ScenarioDeviceFlags.
                                    //      Spelled flags rather than device_flags so the member
                                    //      does not shadow the device_flags enum above when
                                    //      the Ghidra CParser ingests this file
} device_placement_data;            // size 0x08

// ---------------------------------------------------------------------------
// the per-object device extension
// ---------------------------------------------------------------------------
// object + 0x1f4, shared by device_machine, device_control and device_light_fixture. Each
// of power and position is a (group index, current value, current rate) triple: the group
// holds the target that scripts and activations write, the object holds the interpolated
// value that model functions and node animation read. device_update_change_values is what
// walks the object value toward the group value.
typedef struct device_data {
    uint32_t flags;                 // 0x1f4 device_flags
    int16_t power_group;            // 0x1f8 index into the device_group table, -1 for none
    int16_t unknown_1fa;            // 0x1fa never read; alignment ahead of the float
    float power;                    // 0x1fc cached copy of device_group[power_group].value
    float power_change;             // 0x200 signed rate; zeroed by the immediate setter
    int16_t position_group;         // 0x204 index into the device_group table, -1 for none
    int16_t unknown_206;            // 0x206 never read; alignment ahead of the float
    float position;                 // 0x208 cached copy of device_group[position_group].value
    float position_change;          // 0x20c signed rate; zeroed by the immediate setter
    int16_t delay_ticks;            // 0x210 counts up while the position is held back,
                                    //       compared against Device.delay_time_ticks 0x28c;
                                    //       reset to 0 once the position settles
    int16_t unknown_212;            // 0x212 never read; alignment ahead of the flags word
    uint32_t type_flags;            // 0x214 device_machine_flags for a machine,
                                    //       device_control_flags for a control, unused for a
                                    //       light fixture. Copied from the concrete
                                    //       placement record outside this module
} device_data;                      // size 0x24, object + 0x1f4 .. 0x218

// object + 0x1f4 for object type 7. Total object size 0x228, from the "machine" row.
typedef struct device_machine_data {
    device_data device;             // 0x1f4 type_flags holds device_machine_flags
    int32_t ticks_since_fully_open; // 0x218 counted up only while machine_type is door and
                                    //       position is exactly 1.0, compared against
                                    //       DeviceMachine.door_open_time_ticks 0x320 to
                                    //       decide when to close again; reset to 0 as soon
                                    //       as the door leaves 1.0, and to -3 when the
                                    //       proximity scan reopens it
    real_point3d last_elevator_position; // 0x21c world position of the node named by
                                    //       DeviceMachine.elevator_node 0x2ea as of the end
                                    //       of the previous tick. The delta against this
                                    //       tick is what every rider whose parent handle
                                    //       matches this object gets translated by. Only
                                    //       maintained while MachineFlags bit 2 (elevator)
                                    //       is set and elevator_node is not -1
} device_machine_data;              // size 0x34, object + 0x1f4 .. 0x228

// object + 0x1f4 for object type 8. Total object size 0x21c, from the "control" row.
typedef struct device_control_data {
    device_data device;             // 0x1f4 type_flags holds device_control_flags
    uint32_t unknown_218;           // 0x218 no function in this module touches it
} device_control_data;              // size 0x28, object + 0x1f4 .. 0x21c

// object + 0x1f4 for object type 9. Total object size 0x22c, from the "light_fixture" row.
// No function in this module reads the tail; the light fixture branch of the engine lives
// elsewhere, and ScenarioLightFixture carries a ColorRGB, an intensity and two cone angles
// (0x34 .. 0x48) that are the obvious candidates for these 0x14 bytes.
typedef struct device_light_fixture_data {
    device_data device;             // 0x1f4 type_flags unused
    uint8_t unknown_218[0x14];      // 0x218 .. 0x22c
} device_light_fixture_data;        // size 0x38, object + 0x1f4 .. 0x22c

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// global 0x0087abf0: data_array *device_groups   the shared group value table; elements are
//                                                device_group, stride 8, reached as
//                                                *(int *)(device_groups + 0x34) + index * 8.
//                                                Filled at level load by
//                                                device_groups_initialize from
//                                                Scenario.device_groups (count at scenario
//                                                +0x288, pointer +0x28c, stride 0x34), and
//                                                extended at runtime by device_new for
//                                                devices placed without a group. The
//                                                data_new call that creates it is outside
//                                                this module, so the maximum element count
//                                                is not established here.
//
// Globals this module reads but does not own:
// 0x008603b0  data_array *objects           object table; element 0x0c, object pointer +0x08
// 0x0087bc14  tag_instance *tag_instances   types/cache.h
// 0x00746f8c  Scenario *global_scenario     the Scenario tag data. Spelled Scenario * to match
//                                           the six declarations in src/hs at this address
// 0x006f1d6c  game time globals             +0x0c is the tick counter that staggers the
//                                           automatic-activation scan
// 0x006f1d20  int32_t                       nonzero selects the multiplayer team test in the
//                                           one-sided door check. device_machine_update reads
//                                           all 32 bits (0x44b290 `mov edx,DWORD PTR
//                                           ds:0x6f1d20; test edx,edx`); src/items and
//                                           src/objects declare the same address as a uint8_t,
//                                           so the two are narrower and wider reads of one
//                                           global rather than a conflict
// 0x006b0b84  game globals                  +0xa4 is the per-team bitmask the campaign path
//                                           of that same test indexes. src/objects spells this
//                                           address `uint32_t *g_006b0b84` and src/units
//                                           spells the +0xa4 run `friendly_fire_matrix`; all
//                                           three are the same bytes under different names
// 0x006b0a10  data_array *                  recorded-animation playback, see the note above
// 0x00686fe8  void **                       recorded-animation decoder vtables, same note
// 0x00696718  void *effect_creation_origin  constant pointer to .rdata 0x0065c20c, loaded into
//                                           EAX at 0x44c1de and handed straight to the sound
//                                           creation routine
// 0x006966f8  void *sound_creation_origin   constant pointer to .rdata 0x0065c230, loaded into
//                                           ECX at the same site. Both are the two-argument
//                                           position/orientation pair the sound path passes;
//                                           device_play_state_change_effect never dereferences
//                                           either
// 0x00672abc, 0x00672ac0, 0x00672ac4 and 0x00672bbc are .rdata float literals 0.5, 0.0, 1.0
// and 0.0001, not globals.

#pragma pack(pop)
