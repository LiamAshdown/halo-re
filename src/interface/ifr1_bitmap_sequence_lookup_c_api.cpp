#include "halo/interface/ifr1_bitmap_sequence_lookup.hpp"

/**
 * C ABI entry point; forwards to halo::interface::BitmapSequenceLookup::get_bitmap_offset.
 * blam-cc: ECX -> bitmap_tag, AX -> sequence_index, DI -> frame_index
 *
 * @address 0x4ab630
 */
extern "C" int32_t bitmap_group_sequence_get_bitmap_offset(datum_index bitmap_tag, int16_t sequence_index, int16_t frame_index)
{
    return halo::interface::BitmapSequenceLookup::get_bitmap_offset(bitmap_tag, sequence_index, frame_index);
}
