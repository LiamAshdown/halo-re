/**
 * @file include/halo/objects/object_ref.hpp
 * Object system API: object ref.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Handle to one object in the object data array: transforms, markers, hierarchy, animation and placement.
 */
class ObjectRef {
public:
    explicit ObjectRef(uint32_t handle) : handle(handle) {}

    /**
     * Returns the object's bounding centre and bounding radius.
     *
     * Original register convention: real_point3d *out_center in EAX (in_EAX), object index in ECX.
     *
     * @address 0x004088e0
     */
    void get_center_of_mass_and_scale(real_point3d *out_center, float *out_radius);

    /**
     * Adds a velocity change to a parentless object and gives it a randomised angular velocity proportional to the
     * impulse.
     *
     * @address 0x004bef80
     */
    void apply_impulse_and_spin(real_vector3d *delta_velocity);

    /**
     * Walks the child chain of an object and prunes children recursively.
     *
     * Original register convention: datum_index object_index on the stack (param_1).
     *
     * @address 0x004edc10
     */
    void children_recurse_prune();

    /**
     * Returns the index of the player controlling the object or its occupant, or none.
     *
     * Original register convention: datum_index object_index in EAX (in_EAX).
     *
     * @address 0x004ee2e0
     */
    int32_t get_controlling_player_index();

    /**
     * Notifies an object that a player picked it up or refreshes its probe.
     *
     * Original register convention: datum_index player_index in EDI (unaff_EDI); the single stack argument is the
     * object handle (0x4ee415 mov ebx,[esp+0x14] / mov ecx,ebx into object_try_and_get).
     *
     * @address 0x004ee3c0
     */
    void notify_pickup_or_refresh_probe(datum_index player_index);

    /**
     * Zeroes the object's velocities and wakes it from rest.
     *
     * Original register convention: stack -> object_index.
     *
     * @address 0x004f5160
     */
    void reset_velocity_and_wake();

    /**
     * Sets the object's forward, up and position and relinks it.
     *
     * @address 0x004f51c0
     */
    void set_position_and_orientation(real_vector3d *forward, real_vector3d *up, real_point3d *position);

    /**
     * Sets the object's position and recalculates its derived state.
     *
     * Original register convention: position vector pointer in ESI (unaff_ESI), object index in EDI (unaff_EDI).
     *
     * @address 0x004f52c0
     */
    void set_position_and_recalculate(real_point3d *position);

    /**
     * Sets the object's position and relinks it into the given BSP location.
     *
     * Original register convention: position vector pointer in ESI (unaff_ESI), object index in EDI (unaff_EDI).
     *
     * @address 0x004f5350
     */
    void set_position_and_relink(real_point3d *position, bsp_leaf_reference *location);

    /**
     * Links the object into the cluster of a BSP location and its parent's lists.
     *
     * Original register convention: stack -> object_index, location.
     *
     * @address 0x004f5c30
     */
    void set_cluster_and_parent(bsp_leaf_reference *location);

    /**
     * Unlinks the object from its cluster, or notifies its parent when it is attached.
     *
     * Original register convention: object index in EAX (in_EAX).
     *
     * @address 0x004f5de0
     */
    void unlink_cluster_or_notify_parent();

    /**
     * Fills a placement cursor from the root parent of the object.
     *
     * @address 0x004f5f70
     */
    int16_t get_root_parent_placement(object_placement_cursor *out_cursor);

    /**
     * Returns the address of a node's matrix in the object's node array.
     *
     * Original register convention: EAX -> object_index, stack -> node_index.
     *
     * @address 0x004f6000
     */
    real_matrix4x3 * get_node_marker_address(int16_t node_index);

    /**
     * Returns the marker name of one of the object's attachments.
     *
     * Original register convention: object index in EAX (in_EAX), attachment index in EDX (in_DX).
     *
     * @address 0x004f6030
     */
    char * get_attachment_marker_name(int16_t attachment_index);

    /**
     * Looks up a named marker on the object and writes its local transforms; returns the number found.
     *
     * Original register convention: stack -> object_index, marker_name, marker, maximum_markers.
     *
     * @address 0x004f6080
     */
    int32_t get_node_local_transform(char *marker_name, object_marker *marker, uint32_t maximum_markers);

    /**
     * Reorients an object relative to a named marker of its parent.
     *
     * Original register convention: parent object index and parent marker name are the two stack arguments
     * ([ebp+0x8], [ebp+0xc]); the object being reoriented is in ESI and its own marker name in EDI. Both registers
     * are read before ever being written.
     *
     * @address 0x004f6180
     */
    void reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name, char *object_marker_name);

    /**
     * Attaches a child object to a marker of a parent object.
     *
     * @address 0x004f6440
     */
    void attach_to_object(uint32_t child_index, int16_t marker_index);

    /**
     * Snaps the object to its parent's marker and detaches it.
     *
     * Original register convention: the object index is a plain STACK argument (0x4f6619 mov edx,[ebp+0x8]), not a
     * register parameter, as the original prototype -- confirmed against objdump -d -M intel
     * bin/halo.exe at 0x4f6610.
     *
     * @address 0x004f6610
     */
    void snap_to_parent_marker_and_detach();

    /**
     * Sets or clears the flag marking the object as inside the potentially visible set.
     *
     * Original register convention: object index in EAX (in_EAX), boolean in BL (unaff_BL). Confirmed against objdump
     * -d -M intel bin/halo.exe: 0x4f67e1 mov ecx,eax / and ecx,0xffff, 0x4f67ff test bl,bl. // blam-cc: EAX ->
     * object_index, BL -> in_pvs.
     *
     * @address 0x004f67e0
     */
    void set_in_pvs_pass_flag(uint8_t in_pvs);

    /**
     * Enables or disables collision for an object.
     *
     * @address 0x004f6850
     */
    void set_collision_enabled(uint8_t enable);

    /**
     * Writes the object's world position, transforming through the parent's marker node when the object is attached.
     *
     * @address 0x004f6900
     */
    void get_position(real_point3d *out);

    /**
     * Writes the object's forward and up vectors, transformed through its parent when attached.
     *
     * @address 0x004f6970
     */
    void get_orientation(real_vector3d *out_forward, real_vector3d *out_up);

    /**
     * Writes the object's world matrix and returns it.
     *
     * @address 0x004f6a20
     */
    real_matrix4x3 * get_world_matrix(real_matrix4x3 *out);

    /**
     * Returns the linear and angular velocity of the root of the parent chain.
     *
     * @address 0x004f6aa0
     */
    void get_root_object_velocities(real_vector3d *out_velocity, real_vector3d *out_angular_velocity);

    /**
     * Writes the BSP location of the root object of the parent chain.
     *
     * @address 0x004f6b10
     */
    void get_root_location(int32_t *out);

    /**
     * Copies the default node transforms of the object's model into its node array.
     *
     * @address 0x004f6b70
     */
    void copy_default_node_transforms(int16_t requested_count);

    /**
     * Adds a translation to every node of the object.
     *
     * Original register convention: object index in EAX, delta vector in EDX. Confirmed against objdump -d -M intel
     * bin/halo.exe: 0x4f6c36 fld [eax+ecx+0x10] then 0x4f6c3c fadd [edx]. // blam-cc: EAX -> object_index, EDX ->
     * delta.
     *
     * @address 0x004f6c10
     */
    void offset_node_translation(real_vector3d *delta);

    /**
     * Solves a two bone inverse kinematics chain so a node reaches a marker of another object.
     *
     * @address 0x004f6d60
     */
    void solve_two_bone_ik_to_marker(char *marker_a_name, uint32_t marker_b_object_index, char *marker_b_name,
    uint8_t *node_base);

    /**
     * Reads the value of an object function selector for an object; returns whether it is valid.
     *
     * @address 0x004f6e70
     */
    uint8_t function_get_value(int16_t selector, float *out_value);

    /**
     * Returns the handle of the root of the object's parent chain.
     *
     * Original register convention: object index in ECX, returns the root index in EAX. Confirmed against objdump -d
     * -M intel bin/halo.exe: 0x4f6fb3 cmp ecx,0xffffffff at entry, no stack access. // blam-cc: ECX -> object_index.
     *
     * @address 0x004f6fb0
     */
    uint32_t get_root_object_index();

    /**
     * Adds an object to or removes it from the global object list.
     *
     * Original register convention: object index in ECX, add/remove flag as the sole stack parameter. Confirmed
     * against objdump -d -M intel bin/halo.exe: 0x4f7458 mov eax,ecx / and eax,0xffff at entry. // blam-cc: ECX ->
     * object_index, stack -> add.
     *
     * @address 0x004f7450
     */
    void list_membership_set(char add);

    /**
     * Returns whether the object lies inside an atmosphere zone.
     *
     * Original register convention: object index in EAX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f76ee
     * mov esi,eax at entry, then esi is masked as the object index immediately. // blam-cc: EAX -> object_index.
     *
     * @address 0x004f76e0
     */
    uint8_t test_in_atmosphere_zone();

    /**
     * Notifies each child of an object, recursively.
     *
     * Original register convention: object index is the sole, genuinely-stack, parameter. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f7b00 mov eax,[esp+0x4]
     * at entry.
     *
     * @address 0x004f7b00
     */
    void notify_children_recursive();

    /**
     * Moves the object to a spawn position, avoiding collisions with another object; returns success.
     *
     * @address 0x004f7b70
     */
    uint8_t reposition_to_spawn_location(real_point3d *target_position, uint32_t ignore_object_index);

    /**
     * Moves the object by its velocity and writes the new position; returns whether it moved.
     *
     * Original register convention: object index in EAX, out point on the stack. Confirmed against objdump -d -M
     * intel bin/halo.exe: 0x4f7c4c and eax,0xffff at entry, no stack access before that. // blam-cc: EAX ->
     * object_index.
     *
     * @address 0x004f7c40
     */
    uint8_t nudge_position_by_velocity(real_point3d *out);

    /**
     * Notifies the type definition that the node array changed when the object is animated.
     *
     * Original register convention: object index in EAX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f8b1a
     * mov ebx,eax at entry (ebx then carries the object index across the call). // blam-cc: EAX -> object_index.
     *
     * @address 0x004f8b10
     */
    void notify_node_array_if_animated();

    /**
     * Removes the target object from a sibling chain starting at the given slot.
     *
     * @address 0x004f8fe0
     */
    void remove_from_sibling_list(datum_index *slot);

    /**
     * Sets the object's scale over a number of ticks and refreshes its nodes.
     *
     * Original register convention: object index in EAX, new scale as the sole stack parameter. Confirmed against the
     * disassembly: EAX -> object_index, stack -> scale.
     *
     * @address 0x004f96a0
     */
    void set_scale_and_refresh_nodes(float scale, int16_t ticks);

    /**
     * Detaches an object from the map's cluster lists; returns whether it was connected.
     *
     * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
     * "object_disconnect_from_map(uint param_1)").
     *
     * @address 0x004f96f0
     */
    uint8_t disconnect_from_map();

    /**
     * Reserves a render cache slot for the object.
     *
     * Original register convention: object index in EDX, slot index in CX. Confirmed against objdump -d -M intel
     * bin/halo.exe: 0x4f9aca mov eax,edx at entry, 0x4f9adf movsx eax,cx. // blam-cc: EDX -> object_index, CX ->
     * slot.
     *
     * @address 0x004f9ac0
     */
    void reserve_render_cache_slot(int16_t slot);

    /**
     * Releases the object's render cache slot.
     *
     * Original register convention: object index in EDI. Confirmed against objdump-consistent pattern: Ghidra shows
     * only "unaff_EDI", no stack access. // blam-cc: EDI -> object_index.
     *
     * @address 0x004f9b00
     */
    void release_render_cache_slot();

    /**
     * Starts a named animation from a graph tag at the requested frame.
     *
     * @address 0x004fa8d0
     */
    void start_animation(datum_index graph_tag, char *name, int16_t requested_frame);

    /**
     * Returns the number of animation frames left in the object's current animation.
     *
     * Original register convention: object index in EAX. Consistent with Ghidra's own "in_EAX" and no other input. //
     * blam-cc: EAX -> object_index.
     *
     * @address 0x004fa9b0
     */
    uint32_t animation_get_frames_remaining();

    /**
     * Returns the blended marker transform of an attachment instance, stored in the module's scratch marker.
     *
     * @address 0x004fe740
     */
    uint8_t * attachment_get_blended_marker(uint8_t *instance);

private:
    uint32_t handle;
};

/**
 * The common object record header.
 */
class ObjectView {
public:
    explicit ObjectView(object *self) : self(self) {}

    /**
     * Rebuilds an orientation matrix from the difference between the object and a marker.
     *
     * Original register convention: object* in EAX (in_EAX), object_marker* on the stack ([ebp+0x8]), output matrix
     * pointer on the stack ([ebp+0xc]). // blam-cc: EAX -> obj; stack -> marker, output_matrix.
     *
     * @address 0x004f62f0
     */
    void recompute_basis_from_marker_delta(object_marker *marker, real_matrix4x3 *output_matrix);

private:
    object *self;
};

}  // namespace halo::objects
