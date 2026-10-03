/**
 * @file include/halo/objects/object_manager.hpp
 * Object system API: object manager.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Module-wide lifecycle of the object system: game-state allocation, per-tick update, garbage collection, cluster
 * membership and diagnostics.
 */
class ObjectManager {
public:
    /**
     * Deletes unparented scenery and light fixtures.
     *
     * Original register convention: none (void).
     *
     * @address 0x004f47c0
     */
    static void delete_unparented_of_type_mask();

    /**
     * Allocates the object game state, data arrays and cluster tables.
     *
     * Original register convention: none (void), matches Ghidra's __cdecl void(void) signature.
     *
     * @address 0x004f4ad0
     */
    static void initialize();

    /**
     * Resets the object system for a new map.
     *
     * Original register convention: none (void), matches Ghidra's __cdecl void(void) signature.
     *
     * @address 0x004f4bb0
     */
    static void reset();

    /**
     * Flushes pending dirty state of every object.
     *
     * Original register convention: none (void), matches the callers seen elsewhere in this batch.
     *
     * @address 0x004f4cc0
     */
    static void flush_dirty_state();

    /**
     * Disposes the object system and its game state.
     *
     * Original register convention: none (void), matches Ghidra's __cdecl void(void) signature.
     *
     * @address 0x004f4db0
     */
    static void dispose();

    /**
     * Per-tick update of every object and the system around it.
     *
     * Original register convention: none (void), matches the other module init/update entry points.
     *
     * @address 0x004f4e90
     */
    static void update();

    /**
     * Refreshes the cluster membership of every object.
     *
     * Original register convention: no parameters. Confirmed against objdump -d -M intel bin/halo.exe: the whole
     * function reads nothing from the incoming stack or registers before building its own local iterator.
     *
     * @address 0x004f74f0
     */
    static void sweep_refresh_cluster_membership();

    /**
     * Recomputes the cluster membership of every object.
     *
     * Original register convention: no parameters (matches functions.md: a bulk per-tick sweep).
     *
     * @address 0x004f7570
     */
    static void recompute_cluster_membership();

    /**
     * Writes the object count statistics.
     *
     * Original register convention: out pointer in EDX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f7953
     * mov ecx,edx at entry. // blam-cc: EDX -> out.
     *
     * @address 0x004f7950
     */
    static void get_statistics(object_statistics *out);

    /**
     * Overrides the ambient cluster for a local player.
     *
     * Original register convention: a local-player index in AX. Confirmed against objdump -d -M intel bin/halo.exe:
     * 0x4f79d0 cmp ax,0xffff at entry, no stack access. // blam-cc: AX -> local_player_index.
     *
     * @address 0x004f79d0
     */
    static void set_ambient_cluster_override(int16_t local_player_index);

    /**
     * Returns the cluster used for ambient lighting.
     *
     * Original register convention: no parameters. Confirmed against objdump -d -M intel bin/halo.exe: the whole
     * function reads only object_globals_pointer before branching on ambient_cluster_mode.
     *
     * @address 0x004f7a50
     */
    static int16_t get_ambient_cluster();

    /**
     * Deletes objects marked for deletion and objects that fell out of the world.
     *
     * Original register convention: no parameters (matches functions.md: a periodic sweep with no caller inputs).
     *
     * @address 0x004f9c60
     */
    static void garbage_collection();

    /**
     * Writes a per-type object memory usage dump.
     *
     * Original register convention: no parameters (matches functions.md: a one-shot debug dump).
     *
     * @address 0x004fa500
     */
    static void dump_memory();
};

/**
 * One row of the per-type object memory dump.
 */
class ObjectMemoryDumpRecordView {
public:
    explicit ObjectMemoryDumpRecordView(object_memory_dump_record *self) : self(self) {}

    /**
     * Comparison callback ordering dump records by total size.
     *
     * Original register convention: __cdecl, both parameters on the stack (Ghidra already recovered this fully,
     * including the calling convention).
     *
     * @address 0x004fa3a0
     */
    int compare_by_total_size(const object_memory_dump_record *b);

    /**
     * Adds an object's memory use to its type's dump record.
     *
     * @address 0x004fa3d0
     */
    void accumulate_stats(uint32_t object_index);

    /**
     * Writes one dump record as a line to the open file.
     *
     * Original register convention: the record in EAX, the output FILE* as the sole stack parameter. Confirmed
     * against objdump -d -M intel bin/halo.exe: 0x4fa490 mov edx,[eax] at entry. // blam-cc: EAX -> record, stack ->
     * file.
     *
     * @address 0x004fa490
     */
    void write(void *file);

private:
    object_memory_dump_record *self;
};

}  // namespace halo::objects
