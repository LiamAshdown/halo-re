// debug_play_diagnostics  (STANDALONE-ONLY diagnostic, not a function of halo.exe)
// address 0x000000, size 0 bytes
// rewrite confidence: n/a -- temporary play-test logging (2026-09-27); remove once the first-person weapon and
//   friendly-AI hostility bugs are resolved. Called from first_person_weapon_update every tick; logs through the
//   standalone loader's standalone_log every 90 ticks: the first-person weapon interface fields, the player unit's
//   team, the team relationship bits for (player unit team, human 2), and every prop tracking the player unit.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"
#include "interface.h"
#include "cache.h"

extern void __cdecl standalone_log(const char *format, ...);
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data;  // 0x008603b0
extern data_array *prop_data;    // 0x008802c0
extern data_array *actor_data;   // 0x00880360
extern team_pair_globals *team_pair_data; // 0x006b0b84
extern tag_instance *tag_instances; // 0x0087bc14

static int32_t debug_play_tick;

void debug_play_diagnostics(void)
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[0];
    uint8_t *raw = (uint8_t *)fp;
    int16_t player_team = -1;
    int32_t i;

    if ((++debug_play_tick % 90) != 0) {
        return;
    }
    if (fp->unit_index != (datum_index)0xffffffff) {
        uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + 8 + (fp->unit_index & 0xffff) * 0xc);

        player_team = *(int16_t *)(unit + 0xb8);
    }
    standalone_log("DIAG fp attached=%d unit=%08x weapon=%08x state=%d anim=%d frame=%d weapon_hud=%d device_hud=%d "
                   "anim14=%d",
        raw[0], fp->unit_index, fp->weapon_index, fp->state, *(int16_t *)(raw + 0x16), *(int16_t *)(raw + 0x18),
        raw[0x1d8c], raw[0x1e0e], *(int16_t *)(raw + 0x14));
    if (player_team >= 0 && player_team < 10) {
        int32_t ab = player_team * 10 + 2;
        int32_t ba = 2 * 10 + player_team;

        standalone_log("DIAG teams player_team=%d allied(p,2)=%d allied(2,p)=%d secondary(p,2)=%d overrides=%d",
            player_team, (team_pair_data->enemy_bits[ab >> 5] >> (ab & 31)) & 1,
            (team_pair_data->enemy_bits[ba >> 5] >> (ba & 31)) & 1,
            (team_pair_data->secondary_bits[ab >> 5] >> (ab & 31)) & 1, team_pair_data->override_count);
    } else {
        standalone_log("DIAG teams player_team=%d (out of range)", player_team);
    }
    standalone_log("DIAG bits enemy_bits=%08x %08x %08x %08x secondary=%08x %08x", team_pair_data->enemy_bits[0],
        team_pair_data->enemy_bits[1], team_pair_data->enemy_bits[2], team_pair_data->enemy_bits[3],
        team_pair_data->secondary_bits[0], team_pair_data->secondary_bits[1]);
    for (i = 0; i < team_pair_data->override_count && i < 8; i++) {
        team_pair_override *o = &team_pair_data->overrides[i];

        standalone_log("DIAG override %d a=%d b=%d threshold=%d timer_reset=%d u08=%d u09=%d active=%d status=%d u0c=%d "
                       "refcount=%d timer=%d", i, o->index_a, o->index_b, o->threshold, o->timer_reset, o->unknown_08,
            o->unknown_09, o->active, o->status, o->unknown_0c, o->refcount, o->timer);
    }
    {
        extern real_point3d camera_position; // 0x007c3114
        float *node0 = (float *)(raw + 0x108c);

        standalone_log("DIAG gun node0 pos=(%.3f %.3f %.3f) scale=%.3f fwd=(%.3f %.3f %.3f) camera=(%.3f %.3f %.3f)",
            node0[10], node0[11], node0[12], node0[0], node0[1], node0[2], node0[3],
            camera_position.x, camera_position.y, camera_position.z);
    }
    for (i = 0; i < prop_data->maximum_count; i++) {
        uint8_t *p = (uint8_t *)prop_data->data + i * prop_data->size;

        if (*(int16_t *)p == 0) {
            continue;
        }
        if (*(datum_index *)(p + 0x18) == fp->unit_index && fp->unit_index != (datum_index)0xffffffff) {
            datum_index owner = *(datum_index *)(p + 0x04);
            int16_t actor_team = -99;

            if (owner != (datum_index)0xffffffff) {
                actor_team = *(int16_t *)((uint8_t *)actor_data->data + (owner & 0xffff) * actor_data->size + 0x3e);
            }
            standalone_log("DIAG prop %d actor=%08x actor_team=%d prop_team(+12)=%d enemy(+60)=%d kind(+24)=%d "
                           "u61=%d u62=%d", i, owner, actor_team, *(int16_t *)(p + 0x12), p[0x60],
                *(int16_t *)(p + 0x24), p[0x61], p[0x62]);
        }
    }
}

// TEMPORARY (2026-09-27): first-person model draw note, called from render_model when flags == 8 (only the
// first-person weapon/hands draw uses that flag). Every 90th call logs the model, the LOD cutoff test, the LOD,
// the first node matrix after the inverse-bind multiply, the bounding centre, the rasterizer camera and, for each
// region, the geometry picked and its parts' shader types -- to find why the first-person gun is not visible.
static int32_t debug_fp_draw_count;

void debug_fp_render_model_note(uint32_t model_tag, float pixels, int32_t lod, const float *node0,
    const float *center, int32_t early_out)
{
    uint8_t *model = (uint8_t *)tag_instances[model_tag & 0xffff].data;
    int32_t r;

    if ((debug_fp_draw_count++ % 90) != 0) {
        return;
    }
    standalone_log("DIAG fpdraw model=%08x pixels=%.3f cutoff8=%.3f early_out=%d lod=%d nodes=%d node0 pos=(%.3f %.3f %.3f) "
                   "left=(%.3f %.3f %.3f) center=(%.3f %.3f %.3f)",
        model_tag, pixels, *(float *)(model + 0x8), early_out, lod, ((GBXModel *)model)->nodes.count,
        node0 ? node0[10] : 0.0f, node0 ? node0[11] : 0.0f, node0 ? node0[12] : 0.0f,
        node0 ? node0[4] : 0.0f, node0 ? node0[5] : 0.0f, node0 ? node0[6] : 0.0f,
        center ? center[0] : 0.0f, center ? center[1] : 0.0f, center ? center[2] : 0.0f);
    if (early_out) {
        return;
    }
    for (r = 0; r < (int32_t)((GBXModel *)model)->regions.count && r < 4; r++) {
        ModelRegion *region = &((ModelRegion *)((GBXModel *)model)->regions.pointer)[r];
        ModelRegionPermutation *perm = (ModelRegionPermutation *)region->permutations.pointer;
        int16_t geometry_index = (int16_t)(&perm->super_low)[lod];
        int32_t p;

        if (geometry_index < 0) {
            standalone_log("DIAG fpdraw region %d geometry=-1", r);
            continue;
        }
        {
            GBXModelGeometry *geometry = &((GBXModelGeometry *)((GBXModel *)model)->geometries.pointer)[geometry_index];

            standalone_log("DIAG fpdraw region %d geometry=%d parts=%d", r, geometry_index, geometry->parts.count);
            for (p = 0; p < (int32_t)geometry->parts.count && p < 6; p++) {
                GBXModelGeometryPart *part = &((GBXModelGeometryPart *)geometry->parts.pointer)[p];
                ModelShaderReference *ref =
                    &((ModelShaderReference *)((GBXModel *)model)->shaders.pointer)[(int16_t)part->base.shader_index];
                uint8_t *shader = (uint8_t *)tag_instances[ref->shader.tag_id.index].data;

                standalone_log("DIAG fpdraw   part %d shader_type=%d part_flags=%x triangles=%d", p,
                    *(int16_t *)(shader + 0x24), part->base.flags, part->base.triangle_count);
            }
        }
    }
}
