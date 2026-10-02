// sound_adpcm_decode_stereo  (not a Ghidra function; k_sound_decode_procs[2])
// address 0x54ea60, size 470 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: k_sound_decode_procs 0x0065e640 = {0x7fff, 0x54e920, 0x54ea60}; sound_decode_dispatch
//   0x54e830 indexes it by channel count and calls the entry with seven arguments. Only reachable
//   through that table. First-boot track: loading a10's sounds decodes Xbox ADPCM.
// objdump 0x54ea60..0x54ec35: no blocks returns 1. Per block: a left and a right header dword
//   (first sample in the low word, step index in byte 2; an index >= 89 fails, returns 0); the
//   first frame (right << 16 | left) is written, then samples_per_block - 1 more frames. Nibbles
//   come in pairs of dwords (left, right), 8 per dword low nibble first; each frame decodes one left
//   and one right nibble with sound_adpcm_decode_sample(nibble, previous, adpcm_step_table[index]),
//   the index moved by adpcm_index_table[nibble] and clamped to 0..88. The source then advances by
//   block_size. out_0 / out_1 are not touched. Returns 1.
// blam-cc: stack -> source, destination, block_count, block_size, samples_per_block, out_0, out_1 (cdecl)

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t adpcm_index_table[16]; // 0x0065e56c
extern int16_t adpcm_step_table[89];  // 0x0065e590

extern int32_t sound_adpcm_decode_sample(uint8_t selector, int32_t prediction, uint32_t step); // 0x54e8c0

static int32_t adpcm_next_index(int32_t index, uint32_t nibble)
{
    index = index + adpcm_index_table[nibble];
    if (index < 0) {
        return 0;
    }
    if (index >= 89) {
        return 88;
    }
    return index;
}

int32_t sound_adpcm_decode_stereo(void *source, void *destination, int32_t block_count, int32_t block_size,
    int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    uint8_t *block = (uint8_t *)source;
    uint32_t *out = (uint32_t *)destination;
    uint32_t samples_after_header = (uint32_t)(samples_per_block - 1);

    if (block_count == 0) {
        return 1;
    }
    do {
        uint32_t left_header = ((uint32_t *)block)[0];
        uint32_t right_header;
        int32_t left_sample = (int16_t)left_header;
        int32_t left_index = (left_header >> 16) & 0xff;
        int32_t right_sample;
        int32_t right_index;
        uint32_t *data;
        uint32_t remaining = samples_after_header;

        block_count = block_count - 1;
        if (left_index >= 89) {
            return 0;
        }
        right_header = ((uint32_t *)block)[1];
        right_sample = (int16_t)right_header;
        right_index = (right_header >> 16) & 0xff;
        data = (uint32_t *)(block + 8);
        if (right_index >= 89) {
            return 0;
        }
        *out++ = ((uint32_t)(uint16_t)right_sample << 16) | (uint16_t)left_sample;
        while (remaining != 0) {
            uint32_t left_bits = data[0];
            uint32_t right_bits = data[1];
            uint32_t count = remaining < 8 ? remaining : 8;
            uint32_t i;

            data = data + 2;
            for (i = count; i != 0; i--) {
                left_sample = sound_adpcm_decode_sample((uint8_t)(left_bits & 0xf), left_sample,
                    (uint32_t)adpcm_step_table[left_index]);
                left_index = adpcm_next_index(left_index, left_bits & 0xf);
                right_sample = sound_adpcm_decode_sample((uint8_t)(right_bits & 0xf), right_sample,
                    (uint32_t)adpcm_step_table[right_index]);
                right_index = adpcm_next_index(right_index, right_bits & 0xf);
                *out++ = ((uint32_t)(uint16_t)right_sample << 16) | (uint16_t)left_sample;
                left_bits = left_bits >> 4;
                right_bits = right_bits >> 4;
            }
            remaining = remaining - count;
        }
        block = block + block_size;
    } while (block_count != 0);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
