// render_sky  (Ghidra: sky_render_lights_and_lens_flares, phase-2 name; CEA render_sky(void),
// hint only; renamed)
// address 0x510c50, size 1325 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: objdump -d -M intel 0x510c50..0x511184 (__chkstk 0x1660 frame), every stack slot
//   followed through the pushes. Types from types/tags.h Scenario.skies (+0x30, TagDependency
//   stride 0x10), Sky (model +0x0c, animation_graph +0x1c, shader_functions +0xac, animations
//   +0xb8 stride 0x24, lights +0xc4 stride 0x74), SkyLight (lens_flare id +0x0c, marker name
//   +0x10, direction +0x68 / +0x6c), GBXModel.nodes (+0xb8), ModelAnimations.animations (+0x74,
//   stride 0xb4; frame_count +0x22, node_count +0x2c).
//   - only when the render cluster has a sky (render_cluster_has_sky 0x007c334d, sky index
//     0x007c334e, types/structures.h). The sky tag lookup has no NULL check after it.
//   - model_nodes_get_default_transforms 0x4d7610 (ESI model, stack nodes); for each sky
//     animation with a valid animation index and a non zero period whose animation graph entry
//     has as many nodes as the model, sky_animation_times[i] = fmod(frame time (0x007c3110) /
//     period + it, 1.0), then 0x4d53f0 (EDI = animation, stack frame_count * time, nodes). The
//     animation entry is picked with the *sky animation* index i (imul esi,0xb4 at 0x510d49),
//     not with the animation_index the bounds check just used; reproduced.
//   - 0x4d7690 builds the node matrices: EAX = *global_zero_vector3d_pointer (0x006966f8), ECX =
//     *global_forward3d_pointer (0x00696718), stack (model, matrices, nodes, *global_up3d_pointer 0x00696720).
//   - function values: 1.0 for every shader function (at most 8 floats on the stack).
//   - each light with a lens flare: direction from the Euler angles (cos pitch cos yaw,
//     cos pitch sin yaw, sin pitch) when the marker name is empty, else from the camera to the
//     first marker of that name (model_markers_get_by_name 0x4d7850: ECX model tag, EAX name,
//     stack (0, 0, matrices, 0, &marker, 1); position at marker +0x60), normalized unless
//     shorter than 0.0001 (double 0x672bd8). light_transient_add 0x4f1600 then gets EAX = the
//     lens flare tag, EDX = *global_white_color (0x00686b04 -> {1,1,1}), stack (camera +
//     direction * 1023.875, -direction, normalize(perpendicular(-direction)), 1.0).
//   - the sky is drawn at 1/1024 scale around 0.99902344 * camera position: every node matrix
//     is premultiplied by that matrix through the function pointer at 0x00696664
//     (matrix4x3_multiply 0x4cc0d0, cdecl (a, b, out) with b == out), then render_model 0x4d6fc0
//     (EAX model tag, ECX matrices, stack (0, 0, 0, function values, lighting, camera position,
//     0, 0, 0, 0, flags 1)) with a zeroed render_lighting whose ambient is white.
//   - the 0x006893ec debug toggle wraps the draw (0x0069c74c / 0x0071d1fa = 1 before;
//     D3DRS_LIGHTING (0x89) 0 after on pre 0xffff0101 devices), as in render_objects 0x50e930.
// register convention: none (void).
//   // blam-cc: void
// UNSURE: the names of 0x4d53f0 (functions.txt model_vertices_get_interpolated_frame; it
//   samples an animation into the node array) and 0x4d7690 (unnamed; builds node matrices from
//   nodes and a root transform); that light_transient_add is really the lens flare / sun light
//   registration (it takes the SkyLight lens flare tag).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "render.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t render_cluster_has_sky;               // 0x007c334d, structures module
extern int16_t render_cluster_sky_index;             // 0x007c334e, structures module
extern Scenario *global_scenario;                    // 0x00746f8c
extern tag_instance *tag_instances;                  // 0x0087bc14
extern float render_time_since_frame;                // 0x007c3110, this module
extern float sky_animation_times[9];                 // 0x006b923c, this module
extern render_camera render_camera_global;           // 0x007c3114, this module
extern real_point3d *global_zero_vector3d_pointer;                // 0x006966f8 -> {0,0,0}
extern real_vector3d *global_forward3d_pointer;              // 0x00696718 -> {1,0,0}
extern real_vector3d *global_up3d_pointer;                   // 0x00696720 -> {0,0,1}
extern real_matrix4x3 *k_render_identity_matrix_ptr;           // 0x0069673c -> identity at 0x0065c208
extern ColorRGB *global_white_color;              // 0x00686b04 -> {1,1,1} at 0x0065513c
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b,
                                          real_matrix4x3 *out); // 0x00696664 -> 0x4cc0d0
extern uint8_t console_debug_toggle_6893ec;          // 0x006893ec
extern uint8_t rasterizer_render_states_dirty;       // 0x0069c74c
extern uint8_t unknown_0071d1fa;                     // 0x0071d1fa UNSURE
extern uint32_t rasterizer_device_version;           // 0x007c118c
extern void *rasterizer_device;                      // 0x0071d174

extern void model_nodes_get_default_transforms(GBXModel *model, void *nodes);
    // 0x4d7610, models; blam-cc: ESI -> model, stack -> nodes
extern void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame,
                                                             void *out_orientations);
    // 0x4d53f0, models (Ghidra: model_vertices_get_interpolated_frame); blam-cc: EDI -> animation, stack -> (frame, nodes)
extern void model_nodes_build_matrices(real_point3d *position, real_vector3d *forward, GBXModel *model,
                         real_matrix4x3 *matrices, void *nodes, real_vector3d *up);
    // 0x4d7690, models; blam-cc: EAX -> position, ECX -> forward, stack -> (model, matrices,
    // nodes, up); ECX is handed on to matrix4x3_from_forward_up
extern int16_t model_markers_get_by_name(datum_index model_tag, const char *name,
    uint8_t *permutations, uint32_t reserved, real_matrix4x3 *node_matrices, uint32_t flags,
    object_marker *out, int32_t maximum_count);
    // 0x4d7850, models; blam-cc: ECX -> model_tag, EAX -> name, stack -> the other six
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir);
    // 0x4cd670, math; blam-cc: ECX -> out, EDX -> dir
extern void light_transient_add(datum_index light_tag, ColorRGB *color, real_point3d *position,
    real_vector3d *direction, real_vector3d *up, float intensity);
    // 0x4f1600, objects module; blam-cc: EAX=light_tag, EDX=color, stack=(position, direction,
    // up, intensity); src/objects types the two middle slots as opaque uint32_t
extern void render_model(TagID model_tag_id, void *node_matrices, float level_of_detail_pixels,
                         uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values,
                         render_lighting *lighting, real_point3d *bounding_center, float bounding_radius,
                         render_model_effect *effect, datum_index object_index,
                         uint16_t forced_shader_permutation, uint32_t flags);
    // 0x4d6fc0, models; blam-cc: EAX -> model_tag_id, ECX -> node_matrices, stack -> the rest
    // (CEA render_model(model_index, level_of_detail_pixels, node_matrices, ...))
extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod
extern double sqrt(double x);           // x87 FSQRT
extern double fabs(double x);           // x87 FABS
extern double sin(double x);            // x87 FSIN
extern double cos(double x);            // x87 FCOS

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

static datum_index tag_id_of(TagID id)
{
    return *(datum_index *)&id;
}

// Animates and draws the sky model of the current render cluster (scaled to 1/1024 around the
// camera) and registers one transient light per sky light that has a lens flare, placed 1023.875
// units away along the light direction.
void render_sky(void)
{
    datum_index sky_tag;
    Sky *sky;
    GBXModel *model;
    uint8_t nodes[0x800];
    real_matrix4x3 matrices[64];
    float function_values[8];
    render_lighting lighting;
    real_matrix4x3 sky_transform;
    int16_t i;

    if (!render_cluster_has_sky) {
        return;
    }
    sky_tag = 0xffffffff;
    if (render_cluster_sky_index >= 0 &&
        (int32_t)render_cluster_sky_index < (int32_t)global_scenario->skies.count) {
        sky_tag = tag_id_of(((ScenarioSky *)global_scenario->skies.pointer)[render_cluster_sky_index].sky.tag_id);
    }
    sky = 0;
    if (sky_tag != 0xffffffff) {
        sky = (Sky *)tag_instances[(uint16_t)sky_tag].data;
    }
    model = (GBXModel *)tag_instances[sky->model.tag_id.index].data;
    model_nodes_get_default_transforms(model, nodes);

    if (tag_id_of(sky->animation_graph.tag_id) != 0xffffffff) {
        ModelAnimations *graph =
            (ModelAnimations *)tag_instances[sky->animation_graph.tag_id.index].data;

        for (i = 0; (int32_t)i < (int32_t)sky->animations.count; i++) {
            SkyAnimation *entry = &((SkyAnimation *)sky->animations.pointer)[i];

            if (entry->animation_index >= 0 &&
                (int32_t)entry->animation_index < (int32_t)graph->animations.count &&
                entry->period != 0.0f) {
                ModelAnimationsAnimation *animation =
                    &((ModelAnimationsAnimation *)graph->animations.pointer)[i];

                if ((int32_t)(int16_t)animation->node_count == (int32_t)model->nodes.count) {
                    float time = (float)fmod(render_time_since_frame / entry->period +
                                             sky_animation_times[i], 1.0);

                    sky_animation_times[i] = time;
                    animation_overlay_interpolated_frame_orientations(animation,
                        (float)(int32_t)(int16_t)animation->frame_count * time, nodes);
                }
            }
        }
    }

    model_nodes_build_matrices(global_zero_vector3d_pointer, global_forward3d_pointer, model, matrices, nodes, global_up3d_pointer);

    for (i = 0; (int32_t)i < (int32_t)sky->shader_functions.count; i++) {
        function_values[i] = 1.0f;
    }

    for (i = 0; (int32_t)i < (int32_t)sky->lights.count; i++) {
        SkyLight *light = &((SkyLight *)sky->lights.pointer)[i];
        real_vector3d direction;
        real_point3d position;
        real_vector3d toward_camera;
        real_vector3d up;
        real length;

        if (tag_id_of(light->lens_flare.tag_id) == 0xffffffff) {
            continue;
        }
        if (light->lens_flare_marker_name.string[0] == '\0') {
            real cos_pitch = (real)cos(light->direction.pitch);

            direction.i = (real)cos(light->direction.yaw) * cos_pitch;
            direction.j = (real)sin(light->direction.yaw) * cos_pitch;
            direction.k = (real)sin(light->direction.pitch);
        } else {
            object_marker marker;

            if (model_markers_get_by_name(tag_id_of(sky->model.tag_id), light->lens_flare_marker_name.string, 0, 0,
                                          matrices, 0, &marker, 1) == 0) {
                continue;
            }
            direction.i = marker.node_transform.position.x - render_camera_global.position.x;
            direction.j = marker.node_transform.position.y - render_camera_global.position.y;
            direction.k = marker.node_transform.position.z - render_camera_global.position.z;
            length = (real)sqrt(direction.k * direction.k + direction.j * direction.j +
                                direction.i * direction.i);
            if (fabs(length) >= 0.0001) {
                real inverse = 1.0f / length;

                direction.i = inverse * direction.i;
                direction.j = direction.j * inverse;
                direction.k = direction.k * inverse;
            }
        }

        position.x = direction.i * 1023.875f + render_camera_global.position.x;
        position.y = 1023.875f * direction.j + render_camera_global.position.y;
        position.z = direction.k * 1023.875f + render_camera_global.position.z;
        toward_camera.i = -direction.i;
        toward_camera.j = -direction.j;
        toward_camera.k = -direction.k;
        vector3d_build_perpendicular(&up, &toward_camera);
        length = (real)sqrt(up.k * up.k + up.j * up.j + up.i * up.i);
        if (fabs(length) >= 0.0001) {
            real inverse = 1.0f / length;

            up.i = up.i * inverse;
            up.j = up.j * inverse;
            up.k = up.k * inverse;
        }
        light_transient_add(tag_id_of(light->lens_flare.tag_id), global_white_color, &position,
                            &toward_camera, &up, 1.0f);
    }

    sky_transform = *k_render_identity_matrix_ptr;
    sky_transform.position.x = render_camera_global.position.x * 0.99902344f;
    sky_transform.position.y = render_camera_global.position.y * 0.99902344f;
    sky_transform.position.z = render_camera_global.position.z * 0.99902344f;
    sky_transform.scale = 0.0009765625f; // 0x3a800000, 1/1024
    for (i = 0; (int32_t)i < (int32_t)model->nodes.count; i++) {
        matrix4x3_multiply_procedure(&sky_transform, &matrices[i], &matrices[i]);
    }

    if (console_debug_toggle_6893ec) {
        rasterizer_render_states_dirty = 1;
        unknown_0071d1fa = 1;
    }
    {
        uint8_t *raw = (uint8_t *)&lighting;
        uint32_t k;

        for (k = 0; k < sizeof(lighting); k++) {
            raw[k] = 0;
        }
    }
    lighting.ambient_color = *global_white_color;
    render_model(sky->model.tag_id, matrices, 0.0f, 0, 0, function_values, &lighting,
                 &render_camera_global.position, 0.0f, 0, 0, 0, 1);

    if (console_debug_toggle_6893ec && rasterizer_device_version < 0xffff0101) {
        d3d_set_render_state_fn set_render_state =
            (d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4];

        set_render_state(rasterizer_device, 0x89, 0);
    }
}

#if 0
Original Ghidra decompilation (0x510c50):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void sky_render_lights_and_lens_flares(void)

{
  short *psVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  short sVar6;
  uint uVar7;
  int iVar8;
  short sVar9;
  char *pcVar10;
  int iVar11;
  int iVar12;
  undefined4 *puVar13;
  undefined4 *puVar14;
  float10 fVar15;
  float10 fVar16;
  float10 fVar17;
  float fStack_1648;
  undefined1 auStack_1644 [60];
  float fStack_1608;
  float fStack_1604;
  float fStack_1600;
  float fStack_15f0;
  float fStack_15ec;
  float fStack_15e8;
  float fStack_15e4;
  float fStack_15e0;
  float fStack_15dc;
  undefined4 auStack_15d4 [10];
  float fStack_15ac;
  float fStack_15a8;
  float fStack_15a4;
  undefined4 auStack_159c [8];
  undefined4 auStack_157c [30];
  undefined1 local_1504 [2048];
  undefined1 local_d04 [3328];
  undefined4 uStack_4;
  
  iVar8 = DAT_0087bc14;
  uStack_4 = 0x510c5a;
  if (DAT_007c334d != '\0') {
    uVar7 = 0xffffffff;
    if ((-1 < DAT_007c334e) && ((int)DAT_007c334e < *(int *)(global_scenario + 0x30))) {
      uVar7 = *(uint *)(DAT_007c334e * 0x10 + 0xc + *(int *)(global_scenario + 0x34));
    }
    iVar11 = 0;
    if (uVar7 != 0xffffffff) {
      iVar11 = *(int *)((uVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    }
    iVar3 = *(int *)((*(uint *)(iVar11 + 0xc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    model_nodes_get_default_transforms(local_1504);
    if (*(uint *)(iVar11 + 0x1c) != 0xffffffff) {
      iVar8 = *(int *)((*(uint *)(iVar11 + 0x1c) & 0xffff) * 0x20 + 0x14 + iVar8);
      iVar12 = 0;
      sVar9 = 0;
      if (0 < *(int *)(iVar11 + 0xb8)) {
        do {
          psVar1 = (short *)(*(int *)(iVar11 + 0xbc) + iVar12 * 0x24);
          sVar6 = *psVar1;
          if ((((-1 < sVar6) && ((int)sVar6 < *(int *)(iVar8 + 0x74))) &&
              (*(float *)(psVar1 + 2) != 0.0)) &&
             (iVar4 = *(int *)(iVar8 + 0x78),
             (int)*(short *)(iVar12 * 0xb4 + 0x2c + iVar4) == *(int *)(iVar3 + 0xb8))) {
            fVar15 = (float10)FUN_00628cca();
            (&DAT_006b923c)[iVar12] = (float)fVar15;
            model_vertices_get_interpolated_frame
                      ((float)((float10)(int)*(short *)(iVar12 * 0xb4 + iVar4 + 0x22) * fVar15),
                       local_1504);
          }
          sVar9 = sVar9 + 1;
          iVar12 = (int)sVar9;
        } while (iVar12 < *(int *)(iVar11 + 0xb8));
      }
    }
    FUN_004d7690(iVar3,local_d04,local_1504,PTR_DAT_00696720);
    sVar9 = 0;
    if (0 < *(int *)(iVar11 + 0xac)) {
      iVar8 = 0;
      do {
        sVar9 = sVar9 + 1;
        auStack_159c[iVar8] = 0x3f800000;
        iVar8 = (int)sVar9;
      } while (iVar8 < *(int *)(iVar11 + 0xac));
    }
    sVar9 = 0;
    if (0 < *(int *)(iVar11 + 0xc4)) {
      iVar8 = 0;
      do {
        iVar8 = iVar8 * 0x74 + *(int *)(iVar11 + 200);
        if (*(int *)(iVar8 + 0xc) != -1) {
          pcVar10 = (char *)(iVar8 + 0x10);
          do {
            cVar2 = *pcVar10;
            pcVar10 = pcVar10 + 1;
          } while (cVar2 != '\0');
          if (pcVar10 == (char *)(iVar8 + 0x11)) {
            fVar17 = (float10)fcos((float10)*(float *)(iVar8 + 0x6c));
            fVar16 = (float10)fcos((float10)*(float *)(iVar8 + 0x68));
            fVar16 = fVar16 * (float10)(float)fVar17;
            fVar15 = (float10)fsin((float10)*(float *)(iVar8 + 0x68));
            fVar15 = fVar15 * (float10)(float)fVar17;
            fVar17 = (float10)fsin((float10)*(float *)(iVar8 + 0x6c));
            fStack_1648 = (float)fVar17;
          }
          else {
            sVar6 = model_markers_get_by_name(0,0,local_d04,0,auStack_1644,1);
            if (sVar6 == 0) goto LAB_00510ff4;
            fVar16 = (float10)fStack_15e4 - (float10)DAT_007c3114;
            fVar15 = (float10)fStack_15e0 - (float10)DAT_007c3118;
            fStack_1648 = (float)((float10)fStack_15dc - (float10)DAT_007c311c);
            fVar17 = SQRT(fVar16 * fVar16 +
                          fVar15 * fVar15 +
                          ((float10)fStack_15dc - (float10)DAT_007c311c) * (float10)fStack_1648);
            if ((float10)9.999999747378752e-05 <= ABS(fVar17)) {
              fVar17 = (float10)1.0 / fVar17;
              fVar16 = fVar17 * fVar16;
              fVar15 = fVar15 * fVar17;
              fStack_1648 = (float)((float10)fStack_1648 * fVar17);
            }
          }
          fStack_15e4 = (float)(fVar16 * (float10)1023.875 + (float10)DAT_007c3114);
          fStack_15e0 = (float)((float10)1023.875 * fVar15 + (float10)DAT_007c3118);
          fStack_15dc = fStack_1648 * 1023.875 + DAT_007c311c;
          fStack_1608 = (float)-fVar16;
          fStack_1604 = (float)-fVar15;
          fStack_1600 = -fStack_1648;
          vector3d_build_perpendicular();
          fVar5 = SQRT(fStack_15f0 * fStack_15f0 +
                       fStack_15ec * fStack_15ec + fStack_15e8 * fStack_15e8);
          if (0.0001 <= ABS(fVar5)) {
            fVar5 = 1.0 / fVar5;
            fStack_15f0 = fStack_15f0 * fVar5;
            fStack_15ec = fStack_15ec * fVar5;
            fStack_15e8 = fStack_15e8 * fVar5;
          }
          light_transient_add(&fStack_15e4,&fStack_1608,&fStack_15f0,0x3f800000);
        }
LAB_00510ff4:
        sVar9 = sVar9 + 1;
        iVar8 = (int)sVar9;
      } while (iVar8 < *(int *)(iVar11 + 0xc4));
    }
    puVar13 = (undefined4 *)PTR_DAT_0069673c;
    puVar14 = auStack_15d4;
    for (iVar8 = 0xd; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar14 = *puVar13;
      puVar13 = puVar13 + 1;
      puVar14 = puVar14 + 1;
    }
    fStack_15a8 = DAT_007c3118 * 0.99902344;
    fStack_15a4 = DAT_007c311c * 0.99902344;
    sVar9 = 0;
    auStack_15d4[0] = 0x3a800000;
    fStack_15ac = DAT_007c3114 * 0.99902344;
    if (0 < *(int *)(iVar3 + 0xb8)) {
      iVar8 = 0;
      do {
        (*(code *)PTR_matrix4x3_multiply_00696664)
                  (auStack_15d4,local_d04 + iVar8 * 0x34,local_d04 + iVar8 * 0x34);
        sVar9 = sVar9 + 1;
        iVar8 = (int)sVar9;
      } while (iVar8 < *(int *)(iVar3 + 0xb8));
    }
    if (DAT_006893ec != '\0') {
      DAT_0069c74c = 1;
      DAT_0071d1fa = 1;
    }
    puVar13 = auStack_157c;
    for (iVar8 = 0x1d; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar13 = 0;
      puVar13 = puVar13 + 1;
    }
    auStack_157c[0] = *(undefined4 *)PTR_DAT_00686b04;
    auStack_157c[1] = *(undefined4 *)(PTR_DAT_00686b04 + 4);
    auStack_157c[2] = *(undefined4 *)(PTR_DAT_00686b04 + 8);
    FUN_004d6fc0(0,0,0,auStack_159c,auStack_157c,&DAT_007c3114,0,0,0,0,1);
    if ((DAT_006893ec != '\0') && (DAT_007c118c < 0xffff0101)) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
