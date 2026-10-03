// sound_driver_initialize  (Ghidra: missed_545e20; 0 callers -- reached only through the DirectSound
//   driver's initialize slot, sound_driver.initialize +0x04, from sound_initialize 0x5494a0)
// address 0x545e20, size 2283 bytes (0x545e20..0x54670b; Ghidra's first guess of 2210 bytes stops
//   short of the ret at 0x54670a. Five 4-entry jump tables follow at 0x54670c..0x54675b.)
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: out/phase4/sound_types_notes.md driver slot table "0x04 0x545e20 initialize" and
//   "0x545e20 fills [the binding table] and counts it into 0x007252e2"; types/sound.h
//   sound_driver.initialize (uint8_t (*)(sound_driver_parameters *)), sound_driver_parameters,
//   sound_channel_binding and the directsound_* globals. IDirectSound vtable: 0x0c
//   CreateSoundBuffer, 0x10 GetCaps, 0x18 SetCooperativeLevel; IDirectSoundBuffer 0x00
//   QueryInterface, 0x38 SetFormat; IDirectSound3DListener 0x2c SetDistanceFactor, 0x30
//   SetDopplerFactor, 0x3c SetRolloffFactor (DirectSound SDK order). 0x0064e22c is
//   IID_IDirectSound3DListener {279afa84-4981-11ce-a521-0020af0be560}; 0x4043126f is 3.048f
//   (metres per world unit); 0x0069f4c0 is 1.0f. 0x00746270 is the DirectSoundCreate8 pointer
//   engine_initialize_subsystems fetches with GetProcAddress (types/shell.h), 0x007196e4 is
//   shell_nosound, 0x007461c4 the game window.
// Rewritten from the disassembly (partly appended below), not from Ghidra's C: Ghidra could not
//   follow the jump tables of the EAX channel-budget search (its output ends in "Could not recover
//   jumptable at 0x0054617a" and drops the whole grow/roll-back loop body), removed the
//   reachable minimum-count test at 0x546213..0x546228 as "unreachable", and dropped the CX
//   register argument (the channel type flags) of sound_channel_create.
// register convention: plain __cdecl, one stack argument (parameters, [esp+4]); returns AL.
// blam-cc: stack -> parameters
// UNSURE: the four pools are the ones sound_directsound_probe_channel_pools names (mono 3D,
//   mono, stereo, 44k stereo) and are assumed to be channel types 0..3 in the same order; the
//   targets 51/10/8/8 and the minimums 16/2/2/2 are literal constants from the binary.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t shell_nosound;                 // 0x007196e4, foreign (shell)
extern void *shell_window;                    // 0x007461c4, HWND, foreign (shell)
extern void *direct_sound_create8;            // 0x00746270, FARPROC DirectSoundCreate8, foreign (shell)

extern uint8_t directsound_initialized;       // 0x007252e0
extern int16_t directsound_binding_count;     // 0x007252e2
extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4
extern int16_t directsound_channel_count;     // 0x00725428
extern int16_t directsound_first_channel_of_type[4]; // 0x00746028
extern uint8_t directsound_caps[0x60];        // 0x007460ac, DSCAPS
extern void *directsound;                     // 0x0074610c, IDirectSound8*
extern void *directsound_primary_buffer;      // 0x00746110, IDirectSoundBuffer*
extern void *directsound_listener;            // 0x00746114, IDirectSound3DListener*
extern uint8_t directsound_paused;            // 0x00746118
extern float directsound_fade;                // 0x0074611c
extern uint8_t directsound_eax_available;     // 0x00746120
extern uint8_t directsound_eax_enabled;       // 0x00746121
extern int16_t directsound_hardware_3d_channel_count; // 0x00746124
extern int32_t directsound_quality;           // 0x00746128, 0..2
extern int32_t directsound_hardware_mode;     // 0x0074612c
extern int16_t sound_effect_object_state;     // 0x00746130
extern const float directsound_rolloff_factor; // 0x0069f4c0, 1.0f
extern const uint16_t sound_channel_type_flag_table[4]; // 0x0069f528, { 9, 8, 0xa, 0xe }
extern uint8_t iid_directsound_3d_listener[16]; // 0x0064e22c, IID_IDirectSound3DListener
extern const SoundEnvironment k_default_sound_environment; // 0x0065e508
extern const real_vector3d *global_forward3d_pointer; // 0x00696718 -> (1,0,0)
extern const real_vector3d *global_up3d_pointer;      // 0x00696720 -> (0,0,1)

extern void sound_directsound_probe_channel_pools(int32_t *mono3d_count, uint32_t mono3d_requested,
    int32_t *mono_count, uint32_t mono_requested, int32_t *stereo_count, uint32_t stereo_requested,
    int32_t *stereo44k_count, uint32_t stereo44k_requested, uint32_t pool_mask); // 0x545a30
extern uint8_t sound_channel_create(int16_t channel_index, uint16_t type_flags); // 0x546760, blam-cc: stack, CX
extern void sound_driver_dispose(void);       // 0x546a60
extern void sound_listener_update(sound_listener_parameters *parameters); // 0x547070

typedef int32_t (__stdcall *direct_sound_create8_proc)(void *device_guid, void **direct_sound, void *outer);
typedef int32_t (__stdcall *directsound_set_cooperative_level_proc)(void *self, void *window, uint32_t level);
typedef int32_t (__stdcall *directsound_get_caps_proc)(void *self, void *caps);
typedef int32_t (__stdcall *directsound_buffer_set_format_proc)(void *self, sound_wave_format *format);
typedef int32_t (__stdcall *directsound_listener_set_factor_proc)(void *self, float value, uint32_t apply);

#define VTABLE_SLOT(object, offset) ((*(void ***)(object))[(offset) / 4])

#define k_sound_channel_type_count 4
#define k_probe_all_pools 0x116          // pool mask bits 0x02 | 0x04 | 0x10 | 0x100
#define k_channel_budget_iteration_limit 0x200

enum {
    _channel_pool_mono_3d = 0,
    _channel_pool_mono,
    _channel_pool_stereo,
    _channel_pool_stereo_44k
};

static void sound_driver_probe(int32_t counts[k_sound_channel_type_count], const int32_t requested[k_sound_channel_type_count])
{
    sound_directsound_probe_channel_pools(&counts[0], requested[0], &counts[1], requested[1],
        &counts[2], requested[2], &counts[3], requested[3], k_probe_all_pools);
}

static uint8_t sound_driver_pool_below(const int32_t counts[k_sound_channel_type_count],
    const uint8_t saturated[k_sound_channel_type_count], int32_t pool)
{
    // growth targets per pool: 51 mono 3D, 10 mono, 8 stereo, 8 44k stereo
    static const int32_t targets[k_sound_channel_type_count] = { 51, 10, 8, 8 };

    return counts[pool] < targets[pool] && !saturated[pool];
}

// blam-cc: stack -> parameters
// Brings up DirectSound: creates the device, takes priority cooperative level, reads the caps,
// creates the 3D primary buffer and sets its format (16-bit stereo at 44100 Hz on quality 2 when
// the hardware allows, else 22050 or 11025 Hz), then decides how many channels of each type to
// create. With EAX enabled and a healthy first probe, the counts come from an iterative search that
// grows one pool at a time and rolls back when a pool stops growing or another pool shrinks;
// otherwise they come from the quality level ({22,2,2,2} / {24,3,3,3} / {26,4,4,4}). Then the
// 3D listener is set up, the logical channel bindings are laid out by type and one hardware
// channel is created per slot (failures shrink both counts). Returns 1 when at least one
// hardware channel exists; on any failure the driver is disposed and 0 is returned.
uint8_t sound_driver_initialize(sound_driver_parameters *parameters)
{
    uint8_t success = 0;
    uint32_t caps[0x18];
    sound_buffer_description description;
    sound_wave_format format;
    uint32_t sample_rate;
    uint8_t probe_valid;
    int32_t counts[k_sound_channel_type_count];
    int32_t type;
    sound_listener_parameters listener;

    directsound_initialized = 0;
    directsound_paused = 0;
    directsound_fade = 1.0f;

    if (shell_nosound != 0 ||
        ((direct_sound_create8_proc)direct_sound_create8)((void *)0, &directsound, (void *)0) < 0 ||
        ((directsound_set_cooperative_level_proc)VTABLE_SLOT(directsound, 0x18))(directsound, shell_window, 2) < 0) {
        goto failed; // 2 = DSSCL_PRIORITY
    }

    caps[0] = 0x60; // DSCAPS.dwSize
    if (((directsound_get_caps_proc)VTABLE_SLOT(directsound, 0x10))(directsound, caps) < 0) {
        goto failed;
    }
    for (type = 0; type < 0x18; type++) {
        ((uint32_t *)directsound_caps)[type] = caps[type];
    }

    description.size = 0x24;
    description.flags = 0x11; // DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D
    description.buffer_bytes = 0;
    description.reserved = 0;
    description.format = (sound_wave_format *)0;
    description.algorithm_3d[0] = description.algorithm_3d[1] = 0;
    description.algorithm_3d[2] = description.algorithm_3d[3] = 0;
    if (((directsound_create_sound_buffer_proc)VTABLE_SLOT(directsound, 0x0c))(directsound, &description,
            &directsound_primary_buffer, (void *)0) < 0) {
        goto failed;
    }

    // DSCAPS.dwMaxSecondarySampleRate (+0x0c); the compares are unsigned
    counts[0] = counts[1] = counts[2] = counts[3] = 0;
    if (directsound_quality == 2 && caps[3] > 22050) {
        sample_rate = 44100;
    } else if (caps[3] >= 22050) {
        sample_rate = 22050;
    } else {
        sample_rate = (caps[3] < 11025) ? 0 : 11025;
    }

    // Only the first 16 bytes are written (a PCMWAVEFORMAT); extra_size is left as whatever the
    // reused stack slot holds (the low word of description.size, 0x24). PCM ignores it.
    format.format_tag = 1;
    format.channels = 2;
    format.samples_per_second = sample_rate;
    format.average_bytes_per_second = sample_rate * 4;
    format.block_align = 4;
    format.bits_per_sample = 16;
    if (((directsound_buffer_set_format_proc)VTABLE_SLOT(directsound_primary_buffer, 0x38))(directsound_primary_buffer, &format) < 0) {
        goto failed;
    }

    parameters->driver_index = 0;
    probe_valid = 1;
    directsound_eax_available = 1;
    directsound_hardware_3d_channel_count = 0;
    if (!directsound_eax_enabled) {
        sound_effect_object_state = 2;
        directsound_hardware_mode = -1;
    }

    {
        static const int32_t minimums[k_sound_channel_type_count] = { 16, 2, 2, 2 };
        sound_driver_probe(counts, minimums);
    }
    if (counts[0] < 16 || counts[1] + counts[2] + counts[3] < 6) {
        probe_valid = 0;
    }

    if (directsound_eax_enabled && probe_valid) {
        // EAX channel budget search (0x546056..0x54654b)
        int32_t requested[k_sound_channel_type_count] = { 16, 2, 2, 2 };
        int32_t previous[k_sound_channel_type_count];
        uint8_t saturated[k_sound_channel_type_count] = { 0, 0, 0, 0 };
        uint8_t shrank = 0;
        uint8_t done = 0;
        int32_t pool = _channel_pool_mono_3d;
        int32_t iterations = 0;
        int32_t i;

        // the first pass compares against the minimums
        counts[0] = 16;
        counts[1] = counts[2] = counts[3] = 2;

        for (;;) {
            iterations++;
            for (i = 0; i < k_sound_channel_type_count; i++) {
                previous[i] = counts[i];
                counts[i] = 0;
            }

            // grow the current pool by one (pool is always 0..3; the binary skips the probe
            // through its jump table's range check otherwise)
            requested[pool]++;
            sound_driver_probe(counts, requested);
            if (counts[pool] == previous[pool]) {
                saturated[pool] = 1;
            }

            if (previous[0] > counts[0] || previous[1] > counts[1] ||
                previous[2] > counts[2] || previous[3] > counts[3]) {
                // growing this pool cost another pool a channel
                shrank = 1;
                saturated[pool] = 1;
            }

            if (counts[0] < 16 || counts[1] < 2 || counts[2] < 2 || counts[3] < 2 || shrank) {
                // roll the last step back
                counts[0] = counts[1] = counts[2] = counts[3] = 0;
                if ((saturated[0] && saturated[1] && saturated[2] && saturated[3]) || !shrank) {
                    done = 1;
                }
                switch (pool) {
                case _channel_pool_mono_3d: requested[0]--; break;
                case _channel_pool_mono: requested[1]--; break;
                case _channel_pool_stereo: requested[2]--; break;
                case _channel_pool_stereo_44k:
                    // binary behaviour: the 44k stereo rollback also takes one from the stereo request
                    requested[3]--;
                    requested[2]--;
                    break;
                }
                sound_driver_probe(counts, requested);
                shrank = 0;
            }

            if (saturated[0] && saturated[1] && saturated[2] && saturated[3]) {
                break;
            }

            if (iterations >= k_channel_budget_iteration_limit) {
                // gave up: fall back to the minimum budget
                static const int32_t minimums[k_sound_channel_type_count] = { 16, 2, 2, 2 };

                counts[0] = counts[1] = counts[2] = counts[3] = 0;
                sound_driver_probe(counts, minimums);
                break;
            }

            if (counts[0] >= 22 || saturated[0]) {
                // move on to the next pool that is still below its target
                pool = (pool >= 3) ? 0 : pool + 1;
                switch (pool) {
                case _channel_pool_mono_3d:
                    if (sound_driver_pool_below(counts, saturated, 0)) break;
                    pool = _channel_pool_mono;
                    // fall through
                case _channel_pool_mono:
                    if (sound_driver_pool_below(counts, saturated, 1)) break;
                    pool = _channel_pool_stereo;
                    // fall through
                case _channel_pool_stereo:
                    if (sound_driver_pool_below(counts, saturated, 2)) break;
                    pool = _channel_pool_stereo_44k;
                    // fall through
                case _channel_pool_stereo_44k:
                    if (sound_driver_pool_below(counts, saturated, 3)) break;
                    if (sound_driver_pool_below(counts, saturated, 0)) {
                        pool = _channel_pool_mono_3d;
                    } else if (sound_driver_pool_below(counts, saturated, 1)) {
                        pool = _channel_pool_mono;
                    } else if (sound_driver_pool_below(counts, saturated, 2)) {
                        pool = _channel_pool_stereo;
                    } else {
                        goto store_counts; // every pool reached its target (ignores done)
                    }
                    break;
                }
            }

            if (done) {
                break;
            }
        }
    } else {
        if (!probe_valid) {
            directsound_eax_available = 0;
        }
        switch (directsound_quality) {
        case 0: counts[0] = 22; counts[1] = 2; counts[2] = 2; counts[3] = 2; break;
        case 1: counts[0] = 24; counts[1] = 3; counts[2] = 3; counts[3] = 3; break;
        case 2: counts[0] = 26; counts[1] = 4; counts[2] = 4; counts[3] = 4; break;
        default: break; // keep the first probe's counts
        }
    }

store_counts:
    for (type = 0; type < k_sound_channel_type_count; type++) {
        parameters->channel_counts[type] = (int16_t)counts[type];
    }
    for (type = 0; type < k_sound_channel_type_count; type++) {
        parameters->slot_counts[type] = (int16_t)counts[type];
    }

    if (((directsound_query_interface_proc)VTABLE_SLOT(directsound_primary_buffer, 0x00))(directsound_primary_buffer,
            iid_directsound_3d_listener, &directsound_listener) < 0 ||
        ((directsound_listener_set_factor_proc)VTABLE_SLOT(directsound_listener, 0x2c))(directsound_listener, 3.048f, 0) < 0 ||
        ((directsound_listener_set_factor_proc)VTABLE_SLOT(directsound_listener, 0x3c))(directsound_listener,
            directsound_rolloff_factor, 0) < 0) {
        goto failed; // SetDistanceFactor / SetRolloffFactor, DS3D_IMMEDIATE
    }
    ((directsound_listener_set_factor_proc)VTABLE_SLOT(directsound_listener, 0x30))(directsound_listener, 0.0f, 0); // SetDopplerFactor

    // logical channel slots, grouped by type
    {
        int16_t binding_index = 0;
        int16_t slot;

        directsound_binding_count = 0;
        for (type = 0; (int16_t)type < k_sound_channel_type_count; type++) {
            for (slot = 0; slot < parameters->slot_counts[type]; slot++) {
                directsound_binding_count++;
                // the binary guards this store with a flag that is always 1
                directsound_bindings[binding_index].channel_type = (int16_t)type;
                directsound_bindings[binding_index].hardware_channel_index = -1;
                binding_index++;
            }
        }
    }

    // hardware channels, grouped by type; a failed buffer shrinks both counts of its type
    {
        int32_t channel_index = 0;
        int16_t i;

        for (type = 0; type < k_sound_channel_type_count; type++) {
            directsound_first_channel_of_type[type] = (int16_t)channel_index;
            for (i = 0; i < parameters->channel_counts[type]; i++) {
                directsound_channel_count++;
                if (sound_channel_create((int16_t)channel_index, sound_channel_type_flag_table[type])) {
                    channel_index++;
                } else {
                    directsound_channel_count--;
                    parameters->channel_counts[type]--;
                    parameters->slot_counts[type]--;
                }
            }
        }
    }

    success = directsound_channel_count > 0;

    listener.position.x = listener.position.y = listener.position.z = 0.0f;
    listener.velocity.i = listener.velocity.j = listener.velocity.k = 0.0f;
    listener.forward.i = global_forward3d_pointer->i;
    listener.forward.j = global_forward3d_pointer->j;
    listener.forward.k = global_forward3d_pointer->k;
    listener.up.i = global_up3d_pointer->i;
    listener.up.j = global_up3d_pointer->j;
    listener.up.k = global_up3d_pointer->k;
    listener.environment = (SoundEnvironment *)&k_default_sound_environment;
    sound_listener_update(&listener);

    if (success) {
        directsound_initialized = 1;
        return success;
    }

failed:
    sound_driver_dispose();
    return success;
}

#if 0
Original Ghidra decompilation (0x545e20):


/* WARNING: Removing unreachable block (ram,0x00546213) */
/* WARNING: Removing unreachable block (ram,0x0054621a) */
/* WARNING: Removing unreachable block (ram,0x00546221) */
/* WARNING: Removing unreachable block (ram,0x00546228) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1 missed_545e20(short *param_1)

{
  bool bVar1;
  bool bVar2;
  bool bVar3;
  int iVar4;
  int iVar5;
  undefined1 uVar6;
  char cVar7;
  int iVar8;
  uint uVar9;
  short sVar10;
  short sVar11;
  int iVar12;
  int iVar13;
  short *psVar14;
  short sVar15;
  int iVar16;
  uint *puVar17;
  uint *puVar18;
  int iVar19;
  undefined4 *puVar20;
  int iStack_fc;
  int iStack_f8;
  int iStack_f4;
  int iStack_f0;
  char cStack_ea;
  char cStack_e9;
  uint uStack_e8;
  undefined1 local_e2;
  char cStack_e1;
  int iStack_e0;
  undefined2 *puStack_dc;
  int iStack_d8;
  int iStack_d4;
  int iStack_d0;
  int iStack_cc;
  undefined4 uStack_c8;
  int iStack_c4;
  int iStack_c0;
  undefined4 uStack_bc;
  undefined4 uStack_b8;
  undefined4 uStack_b4;
  undefined4 uStack_b0;
  undefined4 uStack_ac;
  undefined4 uStack_a8;
  undefined4 uStack_a4;
  undefined4 uStack_a0;
  undefined4 uStack_9c;
  undefined4 uStack_98;
  undefined4 auStack_94 [4];
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  undefined4 uStack_78;
  undefined4 uStack_74;
  undefined4 *puStack_64;
  uint auStack_60 [24];
  
  local_e2 = 0;
  DAT_007252e0 = 0;
  DAT_00746118 = 0;
  _DAT_0074611c = 0x3f800000;
  if (((_DAT_007196e4 != 0) || (iVar8 = (*DAT_00746270)(0,&DAT_0074610c,0), iVar8 < 0)) ||
     (iVar8 = (**(code **)(*DAT_0074610c + 0x18))(DAT_0074610c,DAT_007461c4,2), iVar8 < 0))
  goto LAB_005466f7;
  auStack_60[0] = 0x60;
  iVar8 = (**(code **)(*DAT_0074610c + 0x10))(DAT_0074610c,auStack_60);
  if (iVar8 < 0) goto LAB_005466f7;
  uStack_ac = 0;
  uStack_a4 = 0;
  uStack_a0 = 0;
  uStack_9c = 0;
  puVar17 = auStack_60;
  puVar18 = &DAT_007460ac;
  for (iVar8 = 0x18; iVar8 != 0; iVar8 = iVar8 + -1) {
    *puVar18 = *puVar17;
    puVar17 = puVar17 + 1;
    puVar18 = puVar18 + 1;
  }
  uStack_98 = 0;
  uStack_b8 = 0x24;
  uStack_b4 = 0x11;
  uStack_b0 = 0;
  uStack_a8 = 0;
  iVar8 = (**(code **)(*DAT_0074610c + 0xc))(DAT_0074610c,&uStack_b8,&DAT_00746110,0);
  if (iVar8 < 0) goto LAB_005466f7;
  iStack_f0 = 0;
  iStack_fc = 0;
  iStack_f8 = 0;
  iStack_f4 = 0;
  if ((DAT_00746128 == 2) && (0x5622 < auStack_60[3])) {
    iStack_c4 = 0xac44;
  }
  else if (auStack_60[3] < 0x5622) {
    iStack_c4 = (-(uint)(auStack_60[3] < 0x2b11) & 0xffffd4ef) + 0x2b11;
  }
  else {
    iStack_c4 = 0x5622;
  }
  iStack_c0 = iStack_c4 * 4;
  uStack_c8 = 0x20001;
  uStack_bc = 0x100004;
  iVar8 = (**(code **)(*DAT_00746110 + 0x38))(DAT_00746110,&uStack_c8);
  if (iVar8 < 0) goto LAB_005466f7;
  *param_1 = 0;
  bVar1 = true;
  DAT_00746120 = 1;
  DAT_00746124 = 0;
  if (DAT_00746121 == '\0') {
    DAT_00746130 = 2;
    DAT_0074612c = 0xffffffff;
  }
  sound_directsound_probe_channel_pools
            (&iStack_f0,0x10,&iStack_fc,2,&iStack_f8,2,&iStack_f4,2,0x116);
  if ((iStack_f0 < 0x10) || (iStack_f4 + iStack_f8 + iStack_fc < 6)) {
    bVar1 = false;
  }
  if (DAT_00746121 == '\0') {
    if (!bVar1) goto LAB_00546554;
  }
  else {
    if (bVar1) {
      iVar19 = 2;
      uVar9 = 0;
      cStack_e1 = '\0';
      cStack_ea = '\0';
      bVar1 = false;
      bVar2 = false;
      bVar3 = false;
      cStack_e9 = '\0';
      puStack_dc = (undefined2 *)0x0;
      uStack_e8 = 0;
      iStack_fc = 2;
      iStack_f8 = 2;
      iStack_f4 = 2;
      iStack_e0 = 0x10;
      iVar13 = 2;
      iVar16 = 2;
      iVar8 = 0x10;
LAB_005460a0:
      iVar5 = iStack_f4;
      iVar4 = iStack_f8;
      iVar12 = iStack_fc;
      puStack_dc = (undefined2 *)((int)puStack_dc + 1);
      iStack_d0 = iStack_fc;
      iStack_d4 = iStack_f4;
      iStack_d8 = iStack_f8;
      iStack_f4 = 0;
      iStack_f8 = 0;
      iStack_fc = 0;
      iStack_f0 = 0;
      iStack_cc = iVar8;
      switch(uVar9) {
      case 0:
        iStack_e0 = iStack_e0 + 1;
        break;
      case 1:
        iVar13 = iVar13 + 1;
        break;
      case 2:
        iVar19 = iVar19 + 1;
        break;
      case 3:
        iVar16 = iVar16 + 1;
        break;
      default:
        if ((((0 < iVar8) || (0 < iVar12)) || (0 < iVar4)) || (0 < iVar5)) {
          cStack_ea = '\x01';
          switch(uVar9) {
          case 0:
            bVar1 = true;
            break;
          case 1:
            bVar2 = true;
            break;
          case 2:
            bVar3 = true;
            break;
          case 3:
            cStack_e9 = '\x01';
          }
        }
        iStack_f4 = 0;
        iStack_f8 = 0;
        iStack_fc = 0;
        iStack_f0 = 0;
        if ((((bVar1) && (bVar2)) && ((bVar3 && (cStack_e9 != '\0')))) || (cStack_ea == '\0')) {
          cStack_e1 = '\x01';
        }
        switch(uVar9) {
        case 0:
          iStack_e0 = iStack_e0 + -1;
          break;
        case 1:
          iVar13 = iVar13 + -1;
          break;
        case 2:
          iVar19 = iVar19 + -1;
          break;
        case 3:
          iVar16 = iVar16 + -1;
          iVar19 = iVar19 + -1;
          break;
        default:
          goto switchD_00546282_default;
        }
        sound_directsound_probe_channel_pools
                  (&iStack_f0,iStack_e0,&iStack_fc,iVar13,&iStack_f8,iVar19,&iStack_f4,iVar16,0x116)
        ;
        uVar9 = uStack_e8;
switchD_00546282_default:
        if (cStack_ea != '\0') {
          cStack_ea = '\0';
        }
        iVar8 = iStack_f0;
        iVar12 = iStack_f8;
        if (((bVar1) && (bVar2)) && ((bVar3 && (uVar9 = uStack_e8, cStack_e9 != '\0'))))
        goto LAB_0054642f;
        if (0x1ff < (int)puStack_dc) {
          iStack_f4 = 0;
          iStack_f8 = 0;
          iStack_fc = 0;
          iStack_f0 = 0;
          sound_directsound_probe_channel_pools
                    (&iStack_f0,0x10,&iStack_fc,2,&iStack_f8,2,&iStack_f4,2,0x116);
          iVar8 = iStack_f0;
          iVar12 = iStack_f8;
          goto LAB_0054642f;
        }
        if ((iStack_f0 < 0x16) && (!bVar1)) goto LAB_0054641f;
        if ((int)uVar9 < 3) {
          uStack_e8 = uVar9 + 1;
          uVar9 = uStack_e8;
          if (3 < uStack_e8) goto LAB_0054641f;
        }
        else {
          uStack_e8 = 0;
        }
        switch(uStack_e8) {
        default:
          if ((iStack_f0 < 0x33) && (uVar9 = uStack_e8, !bVar1)) goto LAB_0054641f;
          uStack_e8 = 1;
          break;
        case 1:
          break;
        case 2:
          goto switchD_00546386_caseD_2;
        case 3:
          goto switchD_00546386_caseD_3;
        }
        if ((9 < iStack_fc) || (uVar9 = uStack_e8, bVar2)) {
          uStack_e8 = uStack_e8 + 1;
switchD_00546386_caseD_2:
          if ((7 < iStack_f8) || (uVar9 = uStack_e8, bVar3)) {
            uStack_e8 = uStack_e8 + 1;
switchD_00546386_caseD_3:
            if ((7 < iStack_f4) || (uVar9 = uStack_e8, cStack_e9 != '\0')) {
              if ((0x32 < iStack_f0) || (bVar1)) {
                if ((9 < iStack_fc) || (bVar2)) {
                  if ((7 < iStack_f8) || (bVar3)) goto LAB_0054642f;
                  uVar9 = 2;
                  uStack_e8 = uVar9;
                }
                else {
                  uVar9 = 1;
                  uStack_e8 = uVar9;
                }
              }
              else {
                uVar9 = 0;
                uStack_e8 = uVar9;
              }
            }
          }
        }
LAB_0054641f:
        if (cStack_e1 != '\0') goto LAB_0054642f;
        goto LAB_005460a0;
      }
      sound_directsound_probe_channel_pools
                (&iStack_f0,iStack_e0,&iStack_fc,iVar13,&iStack_f8,iVar19,&iStack_f4,iVar16,0x116);
                    /* WARNING: Could not recover jumptable at 0x0054617a. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      uVar6 = (*(code *)(&PTR_LAB_0054671c)[uStack_e8])();
      return uVar6;
    }
LAB_00546554:
    DAT_00746120 = 0;
  }
  if (DAT_00746128 == 0) {
    iStack_fc = 2;
    iStack_f4 = 2;
    iVar8 = 0x16;
    iVar12 = 2;
  }
  else if (DAT_00746128 == 1) {
    iStack_fc = 3;
    iStack_f4 = 3;
    iVar8 = 0x18;
    iVar12 = 3;
  }
  else {
    iVar8 = iStack_f0;
    iVar12 = iStack_f8;
    if (DAT_00746128 == 2) {
      iStack_fc = 4;
      iStack_f4 = 4;
      iVar8 = 0x1a;
      iVar12 = 4;
    }
  }
LAB_0054642f:
  param_1[1] = (short)iVar8;
  param_1[2] = (short)iStack_fc;
  param_1[3] = (short)iVar12;
  param_1[4] = (short)iStack_f4;
  psVar14 = param_1 + 5;
  *psVar14 = (short)iVar8;
  param_1[6] = (short)iStack_fc;
  param_1[7] = (short)iVar12;
  param_1[8] = (short)iStack_f4;
  iVar8 = (**(code **)*DAT_00746110)(DAT_00746110,&DAT_0064e22c,&DAT_00746114);
  if (((-1 < iVar8) &&
      (iVar8 = (**(code **)(*DAT_00746114 + 0x2c))(DAT_00746114,0x4043126f,0), -1 < iVar8)) &&
     (iVar8 = (**(code **)(*DAT_00746114 + 0x3c))(DAT_00746114,DAT_0069f4c0,0), -1 < iVar8)) {
    (**(code **)(*DAT_00746114 + 0x30))(DAT_00746114,0,0);
    sVar10 = 0;
    _DAT_007252e2 = 0;
    sVar11 = 0;
    do {
      sVar15 = 0;
      if (0 < *psVar14) {
        do {
          _DAT_007252e2 = _DAT_007252e2 + 1;
          (&DAT_007252e6)[sVar10 * 2] = sVar11;
          (&DAT_007252e4)[sVar10 * 2] = 0xffff;
          sVar10 = sVar10 + 1;
          sVar15 = sVar15 + 1;
        } while (sVar15 < *psVar14);
      }
      sVar11 = sVar11 + 1;
      psVar14 = psVar14 + 1;
    } while (sVar11 < 4);
    iVar13 = 0;
    iVar8 = 0;
    iStack_d0 = (int)&DAT_00746026 - (int)param_1;
    puStack_dc = &DAT_0069f528;
    iStack_d8 = 4;
    psVar14 = param_1;
    do {
      psVar14 = psVar14 + 1;
      *(short *)(iStack_d0 + (int)psVar14) = (short)iVar13;
      iStack_d4 = 0;
      if (0 < *psVar14) {
        do {
          DAT_00725428 = DAT_00725428 + 1;
          cVar7 = sound_channel_create(iVar13);
          if (cVar7 == '\0') {
            DAT_00725428 = DAT_00725428 + -1;
            *psVar14 = *psVar14 + -1;
            param_1[iVar8 + 5] = param_1[iVar8 + 5] + -1;
          }
          else {
            iVar13 = iVar13 + 1;
          }
          iStack_d4 = iStack_d4 + 1;
        } while ((short)iStack_d4 < *psVar14);
      }
      puStack_dc = puStack_dc + 1;
      iVar8 = iVar8 + 1;
      iStack_d8 = iStack_d8 + -1;
    } while (iStack_d8 != 0);
    bVar1 = 0 < DAT_00725428;
    puVar20 = auStack_94;
    for (iVar8 = 0xd; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar20 = 0;
      puVar20 = puVar20 + 1;
    }
    auStack_94[3] = *(undefined4 *)PTR_DAT_00696718;
    uStack_84 = *(undefined4 *)(PTR_DAT_00696718 + 4);
    uStack_80 = *(undefined4 *)(PTR_DAT_00696718 + 8);
    uStack_7c = *(undefined4 *)PTR_DAT_00696720;
    uStack_78 = *(undefined4 *)(PTR_DAT_00696720 + 4);
    uStack_74 = *(undefined4 *)(PTR_DAT_00696720 + 8);
    puStack_64 = &DAT_0065e508;
    local_e2 = bVar1;
    sound_listener_update(auStack_94);
    if (bVar1) {
      DAT_007252e0 = 1;
      return 1;
    }
  }
LAB_005466f7:
  game_sound_dispose();
  return local_e2;
}

Disassembly of the EAX channel-budget search Ghidra lost (0x546056..0x54654f; jump tables:
0x54670c grow {0x5460ed,0x54610c,0x54612c,0x54614c}, 0x54671c saturated {0x546181,0x54618e,0x54619f,0x5461b0},
0x54672c shrank {0x5461f4,0x5461fb,0x546202,0x546209}, 0x54673c rollback {0x546289,0x5462a8,0x5462c8,0x5462e8},
0x54674c advance {0x54638d,0x5463a7,0x5463bb,0x5463cf}). Frame: [esp+0x20/14/18/1c] counts 0..3,
[esp+0x44/40/38/3c] previous 0..3, [esp+0x30]/ebx/edi/ebp requested 0..3, [esp+0x11/13/12/27]
saturated 0..3, [esp+0x26] shrank, [esp+0x2f] done, [esp+0x28] pool, [esp+0x34] iterations:

#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
