/**
 * @file include/halo/objects/object_lifetime.hpp
 * Object system API: object lifetime.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Deletion paths, pending-delete bookkeeping and attachment creation for one object.
 */
class ObjectLifetime {
public:
    explicit ObjectLifetime(uint32_t handle) : handle(handle) {}

    /**
     * Runs the common teardown before an object datum is freed.
     *
     * Original register convention: uint32_t object_index in EAX (in_EAX).
     *
     * @address 0x004edc80
     */
    void delete_teardown();

    /**
     * Clears the object header's pending flag and returns whether it was set.
     *
     * Original register convention: object index in ECX (in_ECX).
     *
     * @address 0x004f46b0
     */
    uint8_t datum_consume_pending_flag();

    /**
     * Marks an object for deletion at the next garbage collection.
     *
     * Original register convention: object index in EAX (in_EAX).
     *
     * @address 0x004f50f0
     */
    void mark_pending_delete();

    /**
     * Clears the pending-delete flag of an object header.
     *
     * Original register convention: object index in EAX (in_EAX).
     *
     * @address 0x004f5130
     */
    void clear_pending_delete_flag();

    /**
     * Deletes an object together with its children and optionally its siblings.
     *
     * @address 0x004f59d0
     */
    void delete_recursive(uint8_t recurse_siblings);

    /**
     * Deletes an object that has no parent.
     *
     * Original register convention: object index in EDI (unaff_EDI).
     *
     * @address 0x004f5aa0
     */
    void delete_unparented();

    /**
     * Deletes the object referenced by a pooled network node record.
     *
     * Original register convention: some caller-owned record pointer in EAX (in_EAX, same shape as
     * object_type_override_call_0x70_release_node's first argument), pooled node id in ECX (in_ECX).
     *
     * @address 0x004f5b50
     */
    static void delete_by_pooled_node_id(int32_t **record);

    /**
     * Deletes an object according to its type's deletion category.
     *
     * @address 0x004f5bd0
     */
    void destroy();

    /**
     * Returns whether the object is marked for deletion.
     *
     * Original register convention: object index in EAX (in_EAX).
     *
     * @address 0x004f5c10
     */
    uint8_t is_delete_pending();

    /**
     * Clears every reference other objects hold to an object that is about to die.
     *
     * Original register convention: the dying object index is the sole, genuinely-stack, parameter, confirmed against objdump 0x4f7415 mov esi,[esp+0x20].
     *
     * @address 0x004f73e0
     */
    void clear_references_to_object();

    /**
     * Deletes an object and optionally its siblings, through the second deletion path.
     *
     * Original register convention: both parameters are plain stack arguments (Ghidra's own
     * "object_delete_4f9030(uint param_1,char param_2)"). Confirmed against objdump -d -M intel bin/halo.exe:
     * 0x4f9031 mov ebx,[esp+0x8] at entry.
     *
     * @address 0x004f9030
     */
    void delete_4f9030(char recurse_siblings);

    /**
     * Creates the attachments (lights, effects, widgets) defined by the object's tag.
     *
     * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
     * "object_create_attachments(uint param_1)").
     *
     * @address 0x004f9750
     */
    void create_attachments();

    /**
     * Deletes the attachments of an object.
     *
     * Original register convention: object index in EBX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f9912
     * mov eax,ebx at entry, with EBX never otherwise assigned in the function body. // blam-cc: EBX -> object_index.
     *
     * @address 0x004f9900
     */
    void delete_attachments();

private:
    uint32_t handle;
};

}  // namespace halo::objects
