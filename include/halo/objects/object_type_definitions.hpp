/**
 * @file include/halo/objects/object_type_definitions.hpp
 * Object system API: object type definitions.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Dispatch over the twelve object type definitions and their sub-definition chains: broadcast notifications, first-
 * answer queries and last-override-wins calls, each addressed by its vtable slot offset.
 */
class ObjectTypeDefinitions {
public:
    /**
     * Builds the chain list of object type definitions from their sub-definitions.
     *
     * @address 0x004f3db0
     */
    static void chain_build();

    /**
     * Broadcasts the creation notification (vtable slot 0x24) to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX -- read only for the type lookup, never passed to
     * the callee, so Ghidra could not see it as a declared parameter).
     *
     * @address 0x004f3e30
     */
    static void notify_0x24(uint32_t object_index, uint32_t argument);

    /**
     * Queries vtable slot 0x28 across the object's type chain and returns the first answer.
     *
     * @address 0x004f3ea0
     */
    static uint8_t query_0x28(uint32_t object_index);

    /**
     * Broadcasts vtable slot 0x2c with an event argument to the object's type chain.
     *
     * Original register convention: stack -> object_index, event_argument.
     *
     * @address 0x004f3f20
     */
    static void notify_two_args_0x2c(uint32_t object_index, uint32_t event_argument);

    /**
     * Broadcasts vtable slot 0x30 to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f3f90
     */
    static void notify_0x30(uint32_t object_index);

    /**
     * Queries vtable slot 0x34 across the object's type chain.
     *
     * Original register convention: stack -> object_index.
     *
     * @address 0x004f4000
     */
    static uint8_t query_0x34(uint32_t object_index);

    /**
     * Broadcasts vtable slot 0x38 to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f4080
     */
    static void notify_0x38(uint32_t object_index);

    /**
     * Broadcasts vtable slot 0x3c with an argument to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f40f0
     */
    static void notify_0x3c(uint32_t object_index, uint32_t argument);

    /**
     * Broadcasts the region damage notification to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f4160
     */
    static void notify_region_damage(uint32_t object_index, uint32_t argument_1, uint32_t argument_2);

    /**
     * Queries vtable slot 0x44 across the object's type chain.
     *
     * Original register convention: stack -> object_index.
     *
     * @address 0x004f41d0
     */
    static uint8_t query_0x44(uint32_t object_index);

    /**
     * Broadcasts vtable slot 0x48 with an event argument to the object's type chain.
     *
     * Original register convention: stack -> object_index, event_argument.
     *
     * @address 0x004f4250
     */
    static void notify_two_args_0x48(uint32_t object_index, uint32_t event_argument);

    /**
     * Broadcasts vtable slot 0x4c with an argument to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f42c0
     */
    static void notify_0x4c(uint32_t object_index, uint32_t argument);

    /**
     * Broadcasts vtable slot 0x50 to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f4330
     */
    static void notify_0x50(uint32_t object_index);

    /**
     * Broadcasts vtable slot 0x54 to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f43a0
     */
    static void notify_0x54(uint32_t object_index);

    /**
     * Broadcasts vtable slot 0x5c to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f4410
     */
    static void notify_0x5c(uint32_t object_index);

    /**
     * Broadcasts vtable slot 0x58 with two arguments to the object's type chain.
     *
     * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
     *
     * @address 0x004f4480
     */
    static void notify_0x58(uint32_t object_index, uint32_t argument_1, uint32_t argument_2);

    /**
     * Calls the last override of vtable slot 0x64 to fill a buffer.
     *
     * Original register convention: ESI -> object_index; stack -> buffer, buffer_size.
     *
     * @address 0x004f44f0
     */
    static int override_get_0x64(uint32_t object_index, void *buffer, int32_t buffer_size);

    /**
     * Calls the last override of vtable slot 0x68 in the object's type chain.
     *
     * Original register convention: none (void); operates on the "current object".
     *
     * @address 0x004f4560
     */
    static void override_call_0x68(uint32_t object_index);

    /**
     * Calls the last override of vtable slot 0x6c with a buffer and bit budget.
     *
     * Original register convention: object index in EDI (unaff_EDI), resolved directly through object_data rather
     * than through object_try_and_get.
     *
     * @address 0x004f45b0
     */
    static int override_call_0x6c(uint32_t object_index, void *buffer, int32_t bit_budget, int32_t full_update);

    /**
     * Calls the last override of vtable slot 0x70 with two arguments.
     *
     * Original register convention: ESI -> object_index, EDI -> edi_argument; stack -> stack_argument.
     *
     * @address 0x004f4620
     */
    static void override_call_0x70(uint32_t object_index, uint32_t edi_argument, uint32_t stack_argument);

    /**
     * Releases a pooled node record for a client through slot 0x70 handling.
     *
     * Original register convention: EAX -> record, stack -> param_1.
     *
     * @address 0x004f4680
     */
    static void override_call_0x70_release_node(int32_t *record, uint32_t client);

    /**
     * Calls the last override of vtable slot 0x74 and returns its answer.
     *
     * Original register convention: object index in EDI (unaff_EDI).
     *
     * @address 0x004f4700
     */
    static uint8_t override_call_0x74(uint32_t object_index);

    /**
     * Calls the last override of vtable slot 0x7c.
     *
     * Original register convention: object index in ESI (unaff_ESI).
     *
     * @address 0x004f4760
     */
    static void override_call_0x7c(uint32_t object_index);
};

}  // namespace halo::objects
