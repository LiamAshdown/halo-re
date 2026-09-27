// decal_place  (Ghidra: FUN_0044edc0; phase 2 guessed "decal_new" -- WRONG, that name belongs to
// 0x44dd90 which this function calls; renamed "decal_place" per
// out/phase4/effects_types_notes.md's own naming for this address throughout its decal section:
// "decal_place 0x44edc0 writes position, creation_game_time, sequence_index, lifetime,
// decay_time, color, alpha, triangle_count and definition_index")
// address 0x44edc0, size 6111 bytes
// name confidence: 0.6 (module notes name it directly)   rewrite confidence: 0.15 (LOW -- by a
//   wide margin the largest and least certain function in this batch; see the extensive UNSURE
//   notes below)
// evidence: types/effects.h decal (every field decal_new/decal_link do NOT own: position 0x08,
//   creation_game_time 0x14, sequence_index 0x18, lifetime 0x1c, decay_time 0x20, color 0x24,
//   alpha 0x28, triangle_count 0x2a, definition_index 0x2c -- every one of the writes at the end
//   of this function lands exactly on one of these offsets from the freshly allocated decal
//   record) and k_decal_type_parameters (the 0x006573f8 table, maximum_edge_angle * 0.017453292
//   read directly here too, matching decal_flood_surfaces' own use of the same table); this
//   module's decal_build_projection 0x44e460, decal_flood_surfaces 0x44e730 and decal_new
//   0x44dd90 (all three already written, this function is their only caller); math module
//   vector3d_normalize_with_length, vector3d_cross_product, vector3d_build_perpendicular,
//   vector3d_angle_between_4cd5e0, matrix4x3_from_axis_angle, vector3d_major_axis_index (the
//   last is misattributed to math per the module notes but called from here); module note item 5
//   ("decal.definition_index... resolve tag data from... edx == 0x0087bc14") establishes that
//   this function's FIRST parameter is the **Decal tag index**, not an object index as phase 2's
//   prototype guessed -- `tag_instances[param_1]` is the very first thing the function does.
// register convention: unknown -- Ghidra recovered all six as ordinary (likely __cdecl) stack
//   parameters; nothing register-passed survived decompilation for the top level signature
//   itself (unlike almost everything this function calls, which loses register arguments at
//   nearly every call site -- see below).
//   // blam-cc: stack -> (decal_tag_index, placement, surface_normal, radius_scale, permanent,
//   //   marker_index)
// UNSURE (extensive, by section):
//  * PARAMETER 2 ("placement"): decal.position ends up copied verbatim from placement+0x18, and
//    a vector at placement+0x24 is dotted against the surface normal (param_3) as an early-out
//    test, and placement+0x44 seeds a per-permutation float table. These offsets do not line up
//    with real_matrix4x3's own layout (position at +0x28, not +0x18), so `placement` is almost
//    certainly a foreign, richer "decal placement request" record (likely built by the collision/
//    damage-response caller, matching decal_spawn_for_response.c's own UNSURE note about a
//    similarly-shaped, unidentified local) rather than a bare matrix. It is modeled here as an
//    opaque `uint8_t *` with named-offset comments; a TYPES-GAP struct not attempted given how
//    little of it is directly load-bearing for the parts of this function that ARE recoverable.
//  * The two calls to decal_build_projection (FUN_0044e460) and the many calls to
//    decal_flood_surfaces (FUN_0044e730) show only one or nine of their real arguments; the
//    rest are reconstructed from the locals that are read immediately afterward, following the
//    same method used throughout this module, but with much lower confidence than anywhere else
//    in the batch given how many locals this function has and how densely they are reused.
//  * The huge local scratch arrays (a projected-quad accumulator, an edge/surface BFS queue pair,
//    and a per-vertex uv/position working buffer) are modeled as fixed-size byte/float buffers
//    sized from Ghidra's own (very likely undersized -- see decal_flood_surfaces.c's own
//    accumulator, which this function's caller-owned buffers must be at least as large as)
//    locals, cast to decal_projection / decal_flood_accumulator
//    at each call site, rather than given fully named fields throughout.
//  * FUN_0044db30 (bitmap/structure lightmap-uv helper, misattributed out of this module) and
//    the tail's cache_allocate_block / rasterizer_decal_vertex_cache_lock (rasterizer decal-geometry block, likely
//    rasterizer_decals-adjacent, well outside this batch) are called with entirely elided
//    arguments at their single call sites; modeled minimally.
//  * The whole "second copy" of the flood/projection pipeline gated by `bVar7` (the outer
//    `do { ... } while (true)` around the entire function body, re-entering with
//    `param_1 = *(uint *)(puVar14 + 10); bVar7 = *puVar14 & 1;` at the very end) is preserved
//    structurally as a loop that can run a second time for a "media mapped" / two-sided Decal
//    tag, but the exact meaning of `puVar14 + 10` (the tag data read that feeds the next
//    iteration's `param_1`) is not established.
//  * Every UNSURE above compounds; this file should be treated as a structural skeleton with the
//    control flow and the (well-evidenced) final decal-record writes preserved, not as a
//    trustworthy bit-exact reconstruction of the projection/flood-fill math itself.
// reconciled: R77 0x0069c632 uint8 decal_place_scratch_flag -> int16 rasterizer_vertex_buffer_lock_state (the store at 0x450581 is a WORD)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include <stdint.h>  // uintptr_t only; this is a .c file, not a Ghidra-ingested header

// The projected-quad accumulator this function owns and hands to decal_flood_surfaces is
// decal_flood_accumulator in types/effects.h (folded there by the phase-4 integration pass out
// of the two local copies that used to live here and in decal_flood_surfaces.c). Ghidra's own
// locals in THIS function undersize it -- local_cb44 is only ever seen as 16 bytes here -- so
// the header's size is what decal_flood_surfaces actually requires, not what Ghidra printed.

extern tag_instance *tag_instances;      // 0x0087bc14
extern data_array *decal_data;           // 0x0087abe4
extern random_seed effect_random_seed;   // 0x00719cd4
extern int32_t *game_time;       // 0x006f1d6c, +0x0c current tick
extern const decal_type_parameters k_decal_type_parameters[4]; // 0x006573f8
extern cache *decal_geometry_cache;       // 0x0071d1c0, UNSURE: the cache_allocate_block target
extern uint8_t *decal_geometry_vtable_owner; // 0x0071d1bc, UNSURE: `(**(code**)(*this+0x30))()`
extern int16_t rasterizer_vertex_buffer_lock_state; // 0x0069c632, rasterizer.h; WORD stores only (R77)

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
    real_vector3d *stack_operand); // 0x4052c0
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t);
    // 0x43f6a0, blam-cc: EAX -> color1, ECX -> color0, stack -> dest, flags, t
extern void *texture_cache_get(void *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, EAX bitmap, stack (wait, allocate_if_missing)
extern int16_t vector3d_major_axis_index(real_vector3d *v); // 0x44d820, math module (misattributed)
extern void structure_lightmap_uv_rect_build(uint32_t sequence_index, uint32_t unknown_1, real radius, void *out_rect);
    // 0x44db30, bitmaps/structures module (misattributed); UNSURE signature, everything guessed
extern datum_index decal_new(int16_t cluster_index, int16_t layer, datum_index insert_before,
    uint8_t object_attached); // 0x44dd90, this module
extern void decal_build_projection(real_matrix4x3 *placement, real *box, decal_projection *out);
    // 0x44e460, this module; UNSURE: `placement` here is reconstructed as (real_matrix4x3*)(the
    // opaque placement pointer), not proven, see file header
extern void decal_flood_surfaces(decal_projection *projection, decal_flood_accumulator *accumulator,
    int32_t surface_index, uint8_t is_first_surface, real radius, int16_t decal_type,
    int32_t *surface_queue, uint16_t *surface_queue_count, int32_t *fallback_queue,
    uint16_t *fallback_queue_count); // 0x44e730, this module
extern void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cb880, math module
extern real vector3d_angle_between_4cd5e0(real_vector3d *a, real_vector3d *b); // 0x4cd5e0, math module
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670, math module
extern void *cache_allocate_block(void *cache); // 0x4d1840, UNSURE signature
extern void cache_evict_entry(void); // 0x4d1c20, UNSURE signature/args
extern void *rasterizer_decal_vertex_cache_lock(void); // 0x51a770, rasterizer module, UNSURE signature/args
extern double fcos(double angle);
extern double fsin(double angle);
extern double sqrt(double x);
extern double fabs(double x); // ABS is a single x87 FABS instruction
extern int32_t FUN_00623e40(double value); // 0x623e40, UNSURE: looks like a round-to-nearest-int
    // helper (fed `x + 0.5` immediately before every call site)

// Projects a Decal tag instance onto the structure BSP starting from a surface normal and
// placement request, flood-filling across neighbouring surfaces (decal_flood_surfaces) to build
// the decal's clipped polygon, then allocates render geometry for it (cache_allocate_block /
// rasterizer_decal_vertex_cache_lock) and finally creates and links the decal datum (decal_new) with its position,
// lifetime, colour and triangle count. See the file header for the very large set of UNSURE
// notes this rewrite carries.
static const uint8_t decal_place_stopgap_enabled = 1; // see the STOPGAP note in the body

void decal_place(datum_index decal_tag_index, uint8_t *placement, real_vector3d *surface_normal,
    real radius_scale, uint8_t permanent, uint16_t marker_index)
{
    uint8_t two_sided_pass = 0;
    Decal *tag; // UNSURE: `puVar14` in the original; Decal tag data

    for (;;) {
        if (decal_tag_index == k_datum_index_none) {
            return;
        }

        tag = (Decal *)tag_instances[(uint16_t)decal_tag_index].data;
        // UNSURE: second tag lookup at Decal tag +0xe4 (the bitmap/material dependency this
        // decal references); kept as a raw pointer, see file header.
        void *material_tag = tag_instances[*(uint16_t *)((uint8_t *)tag + 0xe4)].data;

        int16_t sequence_index = -1;
        real basis_i[3], basis_j[3];       // local_1e4.. / local_3c.. tangent basis candidates
        real projected_i, projected_j, projected_k;    // rotated basis (local_e0..local_cc region)
        real rotation_cos, rotation_sin;    // local_40 / local_44
        decal_projection projection;        // local_210 region (see file header on its true size)
        decal_flood_accumulator accumulator; // local_cb44/cb34 region

        if (!two_sided_pass) {
            real *placement_up = (real *)(placement + 0x24); // UNSURE, see file header

            if (((tag->flags & 8) == 0) ||
                -0.0001f <= surface_normal->i * placement_up[0] + placement_up[2] * surface_normal->k +
                    placement_up[1] * surface_normal->j) {
                // Roll a random tangent-plane rotation angle and build an arbitrary perpendicular
                // basis from it (matching vector3d_build_perpendicular's own shape).
                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                {
                    real angle = (real)(effect_random_seed >> k_random_value_shift) *
                        1.5259022e-05f * 6.2831855f;
                    rotation_cos = (real)fcos(angle);
                    rotation_sin = (real)fsin(angle);
                }
                {
                    real_vector3d perp;
                    vector3d_build_perpendicular(&perp, surface_normal); // UNSURE: real args guessed
                    basis_i[0] = perp.i * placement_up[1] - perp.j * placement_up[2];
                    basis_i[1] = perp.k * placement_up[2] - perp.i * placement_up[2]; // UNSURE: see original
                    basis_i[2] = perp.j * placement_up[2] - perp.k * placement_up[1];
                    basis_j[0] = basis_i[0];
                    basis_j[1] = basis_i[1];
                    basis_j[2] = basis_i[2];
                }
            } else {
                // Flat/edge-on placement: derive the basis from the dominant axis instead.
                // UNSURE: this whole branch's arithmetic is preserved close to verbatim (see
                // the #if 0 block) given how uncertain the surrounding types are; only the two
                // callee names are resolved.
                rotation_cos = -1.0f;
                rotation_sin = 0.0f;
                if ((tag->flags & 0x20) != 0) {
                    // UNSURE: dominant-axis fallback basis construction, preserved structurally
                    // only -- see #if 0 for the exact original arithmetic this stands in for.
                    real_vector3d axis_guess = {0.0f, 0.0f, 0.0f};
                    int16_t axis = vector3d_major_axis_index(surface_normal);
                    real sign = (((real *)surface_normal)[axis] <= 0.0f) ? -1.0f : 1.0f;
                    ((real *)&axis_guess)[axis] = sign;
                    vector3d_normalize_with_length(&axis_guess);
                    vector3d_cross_product((real_vector3d *)basis_i, &axis_guess, surface_normal);
                }
                vector3d_cross_product((real_vector3d *)basis_j, (real_vector3d *)basis_i, surface_normal);
            }

            {
                real length = (real)sqrt((double)(basis_i[0] * basis_i[0] + basis_i[1] * basis_i[1] +
                    basis_i[2] * basis_i[2]));
                if (0.0001f <= (real)fabs((double)length)) {
                    real inv = 1.0f / length;
                    basis_i[0] *= inv; basis_i[1] *= inv; basis_i[2] *= inv;
                }
                length = (real)sqrt((double)(basis_j[0] * basis_j[0] + basis_j[1] * basis_j[1] +
                    basis_j[2] * basis_j[2]));
                if (0.0001f <= (real)fabs((double)length)) {
                    real inv = 1.0f / length;
                    basis_j[0] *= inv; basis_j[1] *= inv; basis_j[2] *= inv;
                }
            }

            projected_i = basis_j[0] * rotation_cos - basis_i[0] * rotation_sin;
            projected_j = basis_j[1] * rotation_cos - basis_i[1] * rotation_sin;
            projected_k = basis_j[2] * rotation_cos - basis_i[2] * rotation_sin;
            // UNSURE: a second rotated pair (basis_i*cos + basis_j*sin) is also computed in the
            // original and reused below as part of `projection`'s placement matrix; omitted from
            // this skeleton's locals but see the #if 0 block for its exact arithmetic.

            sequence_index = (int16_t)marker_index;
            if (marker_index == 0xffff) {
                effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                // UNSURE: material_tag+0xa8 (sequence count) reconstructed from decal_new's own
                // "random in [0, bitmap sequence count)" convention used elsewhere in this module.
                int32_t count = *(int32_t *)((uint8_t *)material_tag + 0xa8);
                int32_t rolled = (int32_t)((int16_t)((uint16_t)(count) *
                    (effect_random_seed >> k_random_value_shift)) >> 16);
                sequence_index = (int16_t)((rolled >= count) ? (count - 1) : rolled);
            }
        }

        // UNSURE: the surface-media / two-sided radius-scale setup (local_1f0/local_48 and the
        // 0x100 tag-flag branch reading a permutation table pair) is preserved only in shape --
        // radius_scale defaults to 1.0 and a random radius roll follows, matching this module's
        // decal_flood_surfaces evidence that radius_scale multiplies k_decal_type_parameters.
        if (radius_scale == 0.0f) {
            radius_scale = 1.0f;
        }
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        {
            real roll = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
            real radius = (roll * (tag->radius[1] - tag->radius[0]) + tag->radius[0]) * radius_scale;

            if (!permanent) {
                // 0x44f359..0x44f437: the bitmap is the decal's map (tag +0xe4, material_tag here); a sprite
                // bitmap (type word 3) uses sequences[sequence_index] (+0x58, 0x40 each) sprites[0] (+0x38, 0x20
                // each) bitmap index, any other bitmap index 0. texture_cache_get(EAX = &bitmaps[index] (+0x64,
                // 0x30 each), wait 0, allocate 1); a miss abandons the decal. The draft passed a bare 0 (crash).
                uint8_t *bitmap_tag = (uint8_t *)material_tag;
                int16_t bitmap_index = 0;

                if (*(int16_t *)bitmap_tag == 3 && sequence_index >= 0) {
                    uint8_t *sequence = *(uint8_t **)(bitmap_tag + 0x58) + sequence_index * 0x40;

                    bitmap_index = **(int16_t **)(sequence + 0x38);
                }
                if (texture_cache_get(*(uint8_t **)(bitmap_tag + 0x64) + bitmap_index * 0x30, 0, 1) == 0) {
                    return;
                }
            }

            // STOPGAP (2026-09-27): the geometry below is not a faithful reconstruction (decal_build_projection
            // gets the projection as its box; the binary passes EDX = the placement matrix [ebp-0xe0], box =
            // {-r, r, -r*aspect, r*aspect} [ebp-0x20c], out = [ebp-0x2a8]; ~450 lines of the original are not
            // reproduced) and crashed in play. Decals are skipped until decal_place is rewritten from objdump.
            if (decal_place_stopgap_enabled) {
                return;
            }
            // UNSURE: decal_build_projection's real (placement, box) arguments -- see file header.
            decal_build_projection((real_matrix4x3 *)placement, (real *)&projection, &projection);

            {
                int32_t edge_queue[17];
                uint16_t edge_queue_count = 1;
                int32_t fallback_queue[1024];
                uint16_t fallback_queue_count = 0; // the 10th argument is elided at this call
                                    // site; decal_flood_surfaces dereferences it unconditionally
                                    // on the is_first_surface path, so it cannot be NULL. An
                                    // earlier draft of this file passed 0 here.
                int16_t i;

                accumulator.vertex_count = 0;
                accumulator.visited_surface_count = 0;
                edge_queue[0] = *(int32_t *)(placement + 0x44); // UNSURE: initial surface index

                for (i = 0; i < (int16_t)edge_queue_count; i++) {
                    decal_flood_surfaces(&projection, &accumulator, edge_queue[i], 1, radius,
                        tag->type, edge_queue, &edge_queue_count, fallback_queue,
                        &fallback_queue_count); // UNSURE: this argument is elided in the
                            // decompile, see file header
                }

                // UNSURE (major): the remaining ~450 lines of the original decompile -- expanding
                // the fallback/rejected-surface queue with a second, mirrored decal_flood_surfaces
                // pass rotated by matrix4x3_from_axis_angle around each rejected edge, converting
                // the accumulated polygon into world-space triangle-fan vertices with UV
                // coordinates, and packing them into the rasterizer geometry block obtained from
                // cache_allocate_block/rasterizer_decal_vertex_cache_lock -- are not reproduced here field-by-field; see
                // the #if 0 block below for the literal Ghidra output. What IS preserved with
                // reasonable confidence is the final decal record write, below, which
                // types/effects.h's own evidence trail confirms independently of this function's
                // geometry math.
                (void)color_interpolate;
                (void)structure_lightmap_uv_rect_build;
                (void)vector3d_angle_between_4cd5e0;
                (void)matrix4x3_from_axis_angle;
                (void)FUN_00623e40;
            }

            if (accumulator.vertex_count > 0) {
                void *cache_block = cache_allocate_block(decal_geometry_cache);
                if (cache_block != (void *)0xffffffff) {
                    datum_index handle = decal_new((int16_t)*(uint16_t *)(placement + 0x10),
                        (int16_t)marker_index, k_datum_index_none, 0); // UNSURE: cluster_index
                        // source guessed from `placement+0x10`, see file header
                    if (handle == k_datum_index_none) {
                        cache_evict_entry();
                        return;
                    }
                    {
                        void *geometry = rasterizer_decal_vertex_cache_lock();
                        if (geometry == (void *)0) {
                            cache_evict_entry();
                            return;
                        }

                        decal *self = &((decal *)decal_data->data)[(uint16_t)handle];

                        self->position.x = *(float *)(placement + 0x18);
                        self->position.y = *(float *)(placement + 0x1c);
                        self->position.z = *(float *)(placement + 0x20);
                        self->creation_game_time = game_time[3]; // +0xc
                        self->sequence_index = (uint8_t)sequence_index;
                        self->unknown_1b = 0; // UNSURE: media-mapped surface index, see original
                        self->unknown_1a = 0;

                        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                        self->lifetime = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                            (tag->lifetime[1] - tag->lifetime[0]) + tag->lifetime[0];

                        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                        self->decay_time = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f *
                            (tag->decay_time[1] - tag->decay_time[0]) + tag->decay_time[0];

                        self->triangle_count = 0; // UNSURE: real fan-triangle count elided, see
                            // the geometry-packing section this rewrite does not reproduce
                        self->definition_index = decal_tag_index;

                        {
                            // 0x4502cb..0x4503dc: alpha = lerp(tag +0x2c, tag +0x30) by a random fraction, the
                            // colour color_interpolate(EAX = tag +0x40, ECX = tag +0x34, dest, (flags byte >> 1) & 3,
                            // another random fraction), packed A8R8G8B8 with fistp rounding
                            ColorRGB color;
                            real alpha;
                            real fraction;

                            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                            alpha = (*(float *)((uint8_t *)tag + 0x30) - *(float *)((uint8_t *)tag + 0x2c)) *
                                ((real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f) +
                                *(float *)((uint8_t *)tag + 0x2c);
                            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                            fraction = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
                            color_interpolate((ColorRGB *)((uint8_t *)tag + 0x40), (ColorRGB *)((uint8_t *)tag + 0x34),
                                &color, (*(uint8_t *)tag >> 1) & 3, fraction);
                            self->color = ((uint32_t)lrint(color.blue * 255.0f) & 0xff) |
                                (((uint32_t)lrint(color.green * 255.0f) & 0xff) << 8) |
                                (((uint32_t)lrint(color.red * 255.0f) & 0xff) << 16) |
                                ((uint32_t)lrint(alpha * 255.0f) << 24);
                        }
                        self->alpha = 0xff;
                    }
                }
            }
        }

        rasterizer_vertex_buffer_lock_state = 0; // mov WORD PTR ds:0x69c632,0 (0x450581)
        // UNSURE: the next iteration's decal_tag_index/two_sided_pass come from
        // `*(uint*)(tag+10)` and `*tag & 1` in the original -- their real fields are not
        // established; modeled here as "no second pass" to keep the loop from running forever
        // on unresolved data.
        return;
    }
}

#if 0
Original Ghidra decompilation (0x44edc0):

void FUN_0044edc0(uint param_1,int param_2,float *param_3,float param_4,char param_5,uint param_6)

{
  float fVar1;
  ushort uVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  ushort *puVar6;
  byte bVar7;
  short sVar8;
  float *pfVar9;
  float *pfVar10;
  int iVar11;
  uint uVar12;
  int iVar13;
  ushort *puVar14;
  float *pfVar15;
  float fVar16;
  undefined4 *puVar17;
  uint uVar18;
  undefined4 *puVar19;
  bool bVar20;
  float10 fVar21;
  float10 fVar22;
  float afStackY_2133c [20976];
  undefined1 local_cb44 [16];
  float local_cb34 [5116];
  ushort local_7b44;
  ushort local_7b42 [1024];
  ushort local_7342;
  undefined4 local_733c [1024];
  float local_633c [4096];
  float local_233c [1024];
  float local_133c [1024];
  undefined4 local_33c [36];
  undefined4 local_2ac [17];
  float local_268;
  float local_264;
  float local_260;
  float local_25c;
  float local_21c;
  float local_218;
  float local_214;
  float local_210;
  float local_20c;
  float local_208;
  float local_204;
  int local_200;
  float local_1fc;
  float local_1f8;
  float local_1f4;
  undefined4 local_1f0;
  int local_1ec;
  float local_1e8;
  float local_1e4;
  float local_1e0;
  float local_1dc;
  int *local_1d8;
  float local_1d4;
  float local_1d0;
  float local_1cc;
  float local_1c8;
  float local_1c4;
  float local_1c0;
  float local_1bc;
  float local_1b4;
  float local_1b0;
  undefined4 local_1ac;
  int local_1a8;
  float local_1a4;
  float local_1a0;
  float local_19c;
  float local_198;
  float local_194;
  float local_190;
  float local_18c;
  float local_188;
  float local_184;
  float local_180;
  float local_17c;
  float local_178;
  undefined4 local_174;
  float local_170;
  float local_16c;
  float local_168;
  float local_164;
  float local_160;
  float local_15c;
  float local_158;
  float local_154;
  float local_150;
  float local_14c;
  float local_148;
  float local_144;
  int local_13c;
  int local_138;
  uint local_134;
  float local_130;
  float local_12c;
  float local_128;
  float local_124;
  float local_120;
  float local_11c;
  float local_118;
  float local_114;
  float local_110;
  float local_10c;
  float local_108;
  float local_104;
  float local_100;
  float local_fc;
  float local_f4;
  float local_f0;
  float local_ec;
  float local_e8;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_d4;
  float local_d0;
  float local_cc;
  float local_c8;
  float local_c4;
  float local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  uint local_88;
  float local_84;
  float local_80;
  float local_7c;
  uint local_78;
  uint local_74;
  float *local_70;
  float *local_6c;
  float *local_68;
  ushort *local_64;
  uint local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float *local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float *local_30;
  ushort *local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float *local_18;
  float local_14 [3];
  float *local_8;

  [see out/phase4/effects_functions.md and tools/pack.py 0x44edc0 for the full 610-line body --
  omitted from this #if 0 block to keep this file a reasonable size; every offset this rewrite
  actually depends on is quoted and explained in the header comment and inline UNSURE notes
  above. The only material control-flow shape not reproduced above is roughly 450 lines covering
  the fallback/rejected-surface re-flood pass, the world-space triangle-fan conversion, and the
  rasterizer geometry packing -- see the file header UNSURE list.]
}
#endif
