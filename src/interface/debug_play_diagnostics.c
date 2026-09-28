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
#include "objects.h"
#include "units.h"

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

        player_team = ((unit_object *)unit)->base.owner_team;
    }
    standalone_log("DIAG fp attached=%d unit=%08x weapon=%08x state=%d anim=%d frame=%d weapon_hud=%d device_hud=%d "
                   "anim14=%d",
        raw[0], fp->unit_index, fp->weapon_index, fp->state, ((struct first_person_weapon_interface *)raw)->unknown_16, *(int16_t *)(raw + 0x18),
        raw[0x1d8c], raw[0x1e0e], ((struct first_person_weapon_interface *)raw)->animation_index);
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
                    *(int16_t *)&((struct Shader *)shader)->shader_type, part->base.flags, part->base.triangle_count);
            }
        }
    }
}

// TEMPORARY (2026-09-27): clip-space position of a world point through the CURRENT rasterizer view (0x7c1290,
// 4 rows of 3) and projection (0x7c13c0, 4x4) -- the same pair chimera__rasterizer_set_frustum_z_func uploads as
// c0..c3 -- plus the effect type (1 = active camouflage path) and the camera near/far.
static int32_t debug_fp_clip_count;
static float debug_fp_node0[13]; // TEMPORARY: scale, forward, left, up, position of the first FP node

void debug_fp_clip_note(const float *world, int32_t effect_type)
{
    const float *view = (const float *)0x007c1290;
    const float *projection = (const float *)0x007c13c0;
    float v[3], clip[4];
    int32_t i;

    if (world != 0) {
        for (i = 0; i < 13; i++) {
            debug_fp_node0[i] = world[i - 10]; // the whole first node matrix (world points at its position)
        }
    }
    if ((debug_fp_clip_count++ % 90) != 0 || world == 0) {
        return;
    }
    for (i = 0; i < 3; i++) {
        v[i] = world[0] * view[0 * 3 + i] + world[1] * view[1 * 3 + i] + world[2] * view[2 * 3 + i] + view[3 * 3 + i];
    }
    for (i = 0; i < 4; i++) {
        clip[i] = v[0] * projection[0 * 4 + i] + v[1] * projection[1 * 4 + i] + v[2] * projection[2 * 4 + i] +
                  projection[3 * 4 + i];
    }
    standalone_log("DIAG fpclip effect_type=%d view=(%.3f %.3f %.3f) clip=(%.3f %.3f %.3f %.3f) ndc=(%.3f %.3f %.3f) "
                   "proj22=%.5f proj32=%.5f", effect_type, v[0], v[1], v[2], clip[0], clip[1], clip[2], clip[3],
        clip[3] != 0.0f ? clip[0] / clip[3] : 0.0f, clip[3] != 0.0f ? clip[1] / clip[3] : 0.0f,
        clip[3] != 0.0f ? clip[2] / clip[3] : 0.0f, projection[2 * 4 + 2], projection[3 * 4 + 2]);
}

// TEMPORARY (2026-09-27): D3D state at each first-person model draw call. render_model arms
// debug_fp_state_armed around model_render_parts for the first-person flags (8); the two indexed-draw wrappers
// call this with the DrawIndexedPrimitive HRESULT. Logs the first 3 armed frames' draws (capped at 40 lines).
extern void *rasterizer_device; // 0x0071d174
int32_t debug_fp_state_armed;
static int32_t debug_fp_state_lines;

typedef int32_t (__stdcall *debug_get_render_state_fn)(void *self, uint32_t state, uint32_t *value);
typedef int32_t (__stdcall *debug_get_viewport_fn)(void *self, uint32_t *viewport);
typedef int32_t (__stdcall *debug_get_pointer_fn)(void *self, void **out);
typedef int32_t (__stdcall *debug_get_texture_fn)(void *self, uint32_t stage, void **out);

void debug_fp_draw_state_note(const char *site, int32_t hresult, uint32_t primitive_type, uint32_t vertex_count,
    uint32_t primitive_count)
{
    void **vtable;
    uint32_t rs[16];
    static const uint32_t states[16] = {
        7, 14, 23, 22, 27, 19, 20, 15, 24, 25, 52, 56, 57, 58, 59, 168
    }; // ZENABLE ZWRITE ZFUNC CULL ABLEND SRC DST ATEST AREF AFUNC STENCIL SFUNC SREF SMASK SWMASK COLORWRITE
    uint32_t viewport[6];
    void *vs = 0;
    void *ps = 0;
    void *tex0 = 0;
    int32_t i;

    if (rasterizer_device == 0) {
        return;
    }
    {
        // pixel shader constants c0..c7 for the first 6 first-person draws and, for comparison, the first 6 model
        // draws that are not first-person (the chain-draw site carries the model parts)
        static int32_t ps_fp_lines;
        static int32_t ps_other_lines;
        int32_t *count = debug_fp_state_armed ? &ps_fp_lines : &ps_other_lines;

        if (*count < 6 && (debug_fp_state_armed || site[0] == 'c')) {
            typedef int32_t (__stdcall *debug_get_ps_constants_fn)(void *self, uint32_t start, float *data,
                uint32_t count);
            float c[32];
            int32_t k;

            (*count)++;
            for (k = 0; k < 32; k++) {
                c[k] = -999.0f;
            }
            ((debug_get_ps_constants_fn)(*(void ***)rasterizer_device)[0x1b8 / 4])(rasterizer_device, 0, c, 8);
            standalone_log("DIAG fpps %s %s count=%u c0=(%.2f %.2f %.2f %.2f) c1=(%.2f %.2f %.2f %.2f) c2=(%.2f %.2f %.2f "
                           "%.2f) c3=(%.2f %.2f %.2f %.2f) c4=(%.2f %.2f %.2f %.2f) c5=(%.2f %.2f %.2f %.2f) c6=(%.2f %.2f "
                           "%.2f %.2f) c7=(%.2f %.2f %.2f %.2f)", debug_fp_state_armed ? "FP" : "other", site,
                primitive_count, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11], c[12], c[13],
                c[14], c[15], c[16], c[17], c[18], c[19], c[20], c[21], c[22], c[23], c[24], c[25], c[26], c[27], c[28],
                c[29], c[30], c[31]);
        }
    }
    if (!debug_fp_state_armed || debug_fp_state_lines >= 40) {
        return;
    }
    debug_fp_state_lines++;
    vtable = *(void ***)rasterizer_device;
    for (i = 0; i < 16; i++) {
        rs[i] = 0xdeadbeef;
        ((debug_get_render_state_fn)vtable[0xe8 / 4])(rasterizer_device, states[i], &rs[i]);
    }
    for (i = 0; i < 6; i++) {
        viewport[i] = 0;
    }
    ((debug_get_viewport_fn)vtable[0xc0 / 4])(rasterizer_device, viewport);
    ((debug_get_pointer_fn)vtable[0x174 / 4])(rasterizer_device, &vs);
    ((debug_get_pointer_fn)vtable[0x1b0 / 4])(rasterizer_device, &ps);
    ((debug_get_texture_fn)vtable[0x100 / 4])(rasterizer_device, 0, &tex0);
    if (debug_fp_state_lines <= 4) {
        // GetVertexShaderConstantF(0, c, 4): the view-projection rows the model vertex shaders use
        typedef int32_t (__stdcall *debug_get_vs_constants_fn)(void *self, uint32_t start, float *data, uint32_t count);
        float c[16];
        float point[3][3];
        int32_t k;
        int32_t j;

        for (k = 0; k < 16; k++) {
            c[k] = 0.0f;
        }
        ((debug_get_vs_constants_fn)vtable[0x17c / 4])(rasterizer_device, 0, c, 4);
        standalone_log("DIAG fpvs c0=(%.3f %.3f %.3f %.3f) c1=(%.3f %.3f %.3f %.3f) c2=(%.3f %.3f %.3f %.3f) "
                       "c3=(%.3f %.3f %.3f %.3f)", c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10],
            c[11], c[12], c[13], c[14], c[15]);
        for (k = 0; k < 3; k++) {
            float along = (k == 0) ? 0.0f : ((k == 1) ? 0.3f : -0.3f);

            for (j = 0; j < 3; j++) {
                point[k][j] = debug_fp_node0[10 + j] + along * debug_fp_node0[1 + j];
            }
            standalone_log("DIAG fpvs point%d (node0 %+.1f fwd)=(%.3f %.3f %.3f) clip=(%.3f %.3f %.3f %.3f)", k, along,
                point[k][0], point[k][1], point[k][2],
                c[0] * point[k][0] + c[1] * point[k][1] + c[2] * point[k][2] + c[3],
                c[4] * point[k][0] + c[5] * point[k][1] + c[6] * point[k][2] + c[7],
                c[8] * point[k][0] + c[9] * point[k][1] + c[10] * point[k][2] + c[11],
                c[12] * point[k][0] + c[13] * point[k][1] + c[14] * point[k][2] + c[15]);
        }
    }
    standalone_log("DIAG fpstate %s hr=%08x prim=%u verts=%u count=%u z=%u zw=%u zf=%u cull=%u ab=%u src=%u dst=%u "
                   "at=%u aref=%u af=%u st=%u sf=%u sref=%u smask=%x swmask=%x cw=%x vp=(%u %u %u %u %.3f %.3f) "
                   "vs=%p ps=%p tex0=%p",
        site, (uint32_t)hresult, primitive_type, vertex_count, primitive_count,
        rs[0], rs[1], rs[2], rs[3], rs[4], rs[5], rs[6], rs[7], rs[8], rs[9], rs[10], rs[11], rs[12], rs[13], rs[14],
        rs[15], viewport[0], viewport[1], viewport[2], viewport[3], *(float *)&viewport[4], *(float *)&viewport[5],
        vs, ps, tex0);
    // GetVertexShader / GetPixelShader / GetTexture AddRef what they return
    if (vs != 0) ((int32_t (__stdcall *)(void *))(*(void ***)vs)[2])(vs);
    if (ps != 0) ((int32_t (__stdcall *)(void *))(*(void ***)ps)[2])(ps);
    if (tex0 != 0) ((int32_t (__stdcall *)(void *))(*(void ***)tex0)[2])(tex0);
}

void debug_fp_state_arm(int32_t armed)
{
    debug_fp_state_armed = armed;
}

// TEMPORARY (2026-09-27): what rasterizer_shader_environment_draw_dispatch does with each first-person part
// (mode 0 draws through the procedure pointers, 1 queues a transparent group, anything else drops the part).
static int32_t debug_fp_dispatch_lines;

void debug_fp_dispatch_note(int32_t toggle, int32_t mode, int32_t shader_type, int32_t primitives, void *draw,
    void *draw_simple, void *overlay)
{
    if (!debug_fp_state_armed || debug_fp_dispatch_lines >= 40) {
        return;
    }
    debug_fp_dispatch_lines++;
    standalone_log("DIAG fpdispatch toggle=%d mode=%d shader_type=%d primitives=%d draw=%p draw_simple=%p overlay=%p",
        toggle, mode, shader_type, primitives, draw, draw_simple, overlay);
}

// TEMPORARY (2026-09-28) EXPERIMENT: turn D3DRS_ALPHATESTENABLE off for every first-person draw, to test whether the
// gun's pixels are being alpha-tested away (the first-person draws run with alpha test on, ref 127, GREATER).
void debug_fp_pre_draw(void)
{
    typedef int32_t (__stdcall *debug_set_render_state_fn)(void *self, uint32_t state, uint32_t value);

    if (!debug_fp_state_armed || rasterizer_device == 0) {
        return;
    }
    ((debug_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, 15, 0);
}
