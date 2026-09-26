// sound_adpcm_decode_mono  (not a Ghidra function; k_sound_decode_procs[1])
// address 0x54e920, size 310 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: k_sound_decode_procs 0x0065e640 = {0x7fff, 0x54e920, 0x54ea60}; sound_decode_dispatch
//   0x54e830 indexes it by channel count and calls the entry with seven arguments. Only reachable
//   through that table. First-boot track: loading a10's sounds decodes Xbox ADPCM.
// objdump 0x54e920..0x54ea55: no blocks returns 1. Per block: the header dword holds the first sample
//   (low word, signed) and the step index (byte 2); an index >= 89 fails (returns 0). The header
//   sample is written, then samples_per_block - 1 more, two per source byte (low nibble first),
//   each sound_adpcm_decode_sample(nibble, previous, adpcm_step_table[index]) with the index moved
//   by adpcm_index_table[nibble] and clamped to 0..88. The source then advances by block_size (the
//   last half byte of a block is unused). out_0 / out_1 are not touched. Returns 1.
// blam-cc: stack -> source, destination, block_count, block_size, samples_per_block, out_0, out_1 (cdecl)

#include "tags.h"
#include "memory.h"
#include "sound.h"

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

int32_t sound_adpcm_decode_mono(void *source, void *destination, int32_t block_count, int32_t block_size,
    int32_t samples_per_block, int32_t *out_0, int32_t *out_1)
{
    uint8_t *block = (uint8_t *)source;
    int16_t *out = (int16_t *)destination;
    uint32_t samples_after_header = (uint32_t)(samples_per_block - 1);

    if (block_count == 0) {
        return 1;
    }
    do {
        uint32_t header = *(uint32_t *)block;
        int32_t sample = (int16_t)header;
        int32_t index = (header >> 16) & 0xff;
        uint8_t *data = block + 4;
        uint32_t remaining = samples_after_header;

        block_count = block_count - 1;
        if (index >= 89) {
            return 0;
        }
        *out++ = (int16_t)sample;
        while (remaining != 0) {
            uint32_t byte = *data++;

            sample = sound_adpcm_decode_sample((uint8_t)(byte & 0xf), sample, (uint32_t)adpcm_step_table[index]);
            index = adpcm_next_index(index, byte & 0xf);
            *out++ = (int16_t)sample;
            if (--remaining == 0) {
                break;
            }
            sample = sound_adpcm_decode_sample((uint8_t)(byte >> 4), sample, (uint32_t)adpcm_step_table[index]);
            index = adpcm_next_index(index, byte >> 4);
            *out++ = (int16_t)sample;
            remaining = remaining - 1;
        }
        block = block + block_size;
    } while (block_count != 0);
    return 1;
}
