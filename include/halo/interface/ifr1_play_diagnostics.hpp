#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"
#include "interface.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Standalone-build diagnostics that log first-person weapon, team and rasterizer state every 90th call.
 */
class PlayDiagnostics {
public:
    static void run(void);
    static void fp_render_model_note(uint32_t model_tag, float pixels, int32_t lod, const float *node0, const float *center, int32_t early_out);
    static void fp_clip_note(const float *world, int32_t effect_type);
    static void fp_draw_state_note(const char *site, int32_t hresult, uint32_t primitive_type, uint32_t vertex_count, uint32_t primitive_count);
    static void fp_state_arm(int32_t armed);
    static void fp_dispatch_note(int32_t toggle, int32_t mode, int32_t shader_type, int32_t primitives, void *draw, void *draw_simple, void *overlay);
    static void fp_pre_draw(void);
};

}
