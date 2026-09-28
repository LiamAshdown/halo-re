// players_structure_bsp_switch_regroup  (not a Ghidra function; structure bsp activate slot 10)
// address 0x4762f0, size 759 bytes
// name confidence: 0.4  rewrite confidence: 0.8
// evidence: structure_bsp_activate_procedures[10] (0x0069e904) holds 0x4762f0; it is the only reader of
//   player_globals +0x12 besides its seeding, and hands its result to game_engine_reattach_player_unit_unused
//   0x475270.
// objdump 0x4762f0..0x4765e6: with player_globals (0x0087a478) +0x12 set (a scenario bsp switch trigger
//   volume, scenario +0x3a0, 8 bytes each) and more than one player (+0x0c > 1):
//   - when the volume's +0x06 names a cutscene flag (scenario +0x4e8, 0x5c each, position +0x24), that
//     position is raised in 0.05 steps while object_collision_test_cluster_group (EDI point, stack 0x4029,
//     -1) hits, for at most 0.3; it is usable only when the rise stayed below 0.3 (x87 compares of the
//     unrounded running sum);
//   - the first player (data_iterator over player_data 0x0087a480) whose unit's center (object +0xa0) is in
//     the trigger volume (scenario_trigger_volume_contains_point, EAX volume, ECX point) and whose crouch
//     probe point (unit_get_crouch_height_offset, EAX out, ECX unit, stack height, EBX radius) lies in a
//     cluster supplies the target: the probe point, or the flag point raised by the unit's radius;
//   - then every local player whose unit is not that unit is reattached to it at the target
//     (game_engine_reattach_player_unit_unused, stack player, unit, &target) and gets +0x3c = -1, and
//     +0x12 is cleared.
//   Always, every player's +0x3c word ends -1.
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "game.h"
#include "units.h"

extern player_globals *local_player_globals;            // 0x0087a478
extern data_array *player_data;                         // 0x0087a480
extern data_array *object_data;                         // 0x008603b0
extern Scenario *global_scenario;
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern uint8_t object_collision_test_cluster_group(uint32_t flags, real_point3d *position,
    uint32_t exclude_object_index); // 0x505490, EDI position, stack
extern uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point); // 0x53f020, EAX, ECX
extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out); // 0x55a2e0, EAX, ECX, stack, EBX
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
extern void game_engine_reattach_player_unit_unused(uint32_t player_index, uint32_t target_object,
    void *local_offset); // 0x475270

static void players_clear_bsp_cluster(void)
{
    data_iterator iterator;
    player *entry;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)iterator.data ^ k_data_iterator_signature;
    while ((entry = (player *)data_iterator_next(&iterator)) != 0) {
        entry->bsp_cluster = -1;
    }
}

void players_structure_bsp_switch_regroup(void)
{
    int16_t volume = local_player_globals->unknown_12;
    real_point3d target;
    float offset = 0.0f;
    uint8_t have_flag = 0;
    uint8_t found = 0;
    datum_index chosen_unit = k_datum_index_none;
    int16_t flag_index;
    data_iterator iterator;
    player *entry;

    if (volume == -1 || local_player_globals->local_player_count <= 1) {
        players_clear_bsp_cluster();
        return;
    }

    flag_index = *(int16_t *)((uint8_t *)global_scenario->bsp_switch_trigger_volumes.pointer + volume * 8 + 6);
    if (flag_index != -1) {
        target = *(real_point3d *)((uint8_t *)global_scenario->cutscene_flags.pointer + flag_index * 0x5c + 0x24);
        offset = 0.0f;
        while (object_collision_test_cluster_group(0x4029, &target, 0xffffffff)) {
            double sum;

            target.z = target.z + 0.05f;
            sum = (double)offset + (double)0.05f;
            offset = (float)sum;
            if (!(sum < (double)0.3f)) {
                break;
            }
        }
        have_flag = 1;
        if (!(offset < 0.3f)) {
            have_flag = 0;
        }
    }

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)iterator.data ^ k_data_iterator_signature;
    while (!found && (entry = (player *)data_iterator_next(&iterator)) != 0) {
        uint8_t *unit_object;
        int16_t trigger_volume;
        real_point3d probe;
        float height;
        float radius;
        uint32_t leaf;

        if (entry->unit == k_datum_index_none || local_player_globals->unknown_12 == -1) {
            continue;
        }
        unit_object = (uint8_t *)((object_header *)object_data->data)[entry->unit & 0xffff].data;
        trigger_volume = *(int16_t *)((uint8_t *)global_scenario->bsp_switch_trigger_volumes.pointer + local_player_globals->unknown_12 * 8);
        if (!scenario_trigger_volume_contains_point(trigger_volume, (real_point3d *)(unit_object + 0xa0))) {
            continue;
        }
        unit_get_crouch_height_offset(&probe, entry->unit, &height, &radius);
        offset = radius;
        leaf = bsp3d_node_find_leaf(0, global_collision_bsp, &probe);
        if (leaf == 0xffffffff ||
            *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8) == -1) {
            continue;
        }
        if (!have_flag) {
            target = probe;
        } else {
            target.z = target.z + offset;
        }
        chosen_unit = entry->unit;
        found = 1;
    }

    if (found && local_player_globals->local_players[0] != k_datum_index_none) {
        datum_index player_index = local_player_globals->local_players[0];
        player *local = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200);

        if (local->unit != k_datum_index_none && local->unit != chosen_unit) {
            game_engine_reattach_player_unit_unused(player_index, chosen_unit, &target);
            ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->bsp_cluster = -1;
        }
    }
    local_player_globals->unknown_12 = -1;
    players_clear_bsp_cluster();
}
