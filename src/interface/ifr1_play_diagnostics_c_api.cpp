#include "halo/interface/ifr1_play_diagnostics.hpp"

/**
 * C ABI entry point; forwards to halo::interface::PlayDiagnostics::run.
 */
extern "C" void debug_play_diagnostics(void)
{
    halo::interface::PlayDiagnostics::run();
}

/**
 * C ABI entry point; forwards to halo::interface::PlayDiagnostics::fp_render_model_note.
 */
extern "C" void debug_fp_render_model_note(uint32_t model_tag, float pixels, int32_t lod, const float *node0, const float *center, int32_t early_out)
{
    halo::interface::PlayDiagnostics::fp_render_model_note(model_tag, pixels, lod, node0, center, early_out);
}

/**
 * C ABI entry point; forwards to halo::interface::PlayDiagnostics::fp_clip_note.
 */
extern "C" void debug_fp_clip_note(const float *world, int32_t effect_type)
{
    halo::interface::PlayDiagnostics::fp_clip_note(world, effect_type);
}

/**
 * C ABI entry point; forwards to halo::interface::PlayDiagnostics::fp_draw_state_note.
 */
extern "C" void debug_fp_draw_state_note(const char *site, int32_t hresult, uint32_t primitive_type, uint32_t vertex_count, uint32_t primitive_count)
{
    halo::interface::PlayDiagnostics::fp_draw_state_note(site, hresult, primitive_type, vertex_count, primitive_count);
}

/**
 * C ABI entry point; forwards to halo::interface::PlayDiagnostics::fp_state_arm.
 */
extern "C" void debug_fp_state_arm(int32_t armed)
{
    halo::interface::PlayDiagnostics::fp_state_arm(armed);
}

/**
 * C ABI entry point; forwards to halo::interface::PlayDiagnostics::fp_dispatch_note.
 */
extern "C" void debug_fp_dispatch_note(int32_t toggle, int32_t mode, int32_t shader_type, int32_t primitives, void *draw, void *draw_simple, void *overlay)
{
    halo::interface::PlayDiagnostics::fp_dispatch_note(toggle, mode, shader_type, primitives, draw, draw_simple, overlay);
}

/**
 * C ABI entry point; forwards to halo::interface::PlayDiagnostics::fp_pre_draw.
 */
extern "C" void debug_fp_pre_draw(void)
{
    halo::interface::PlayDiagnostics::fp_pre_draw();
}
