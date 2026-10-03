#include "halo/interface/ifr1_bitmap_sequence_lookup.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/cache/api.hpp"


namespace halo::interface {

/**
 * Resolves animation frame frame_index of bitmap_tag's sequence_index'th BitmapGroupSequence to a byte offset
 * 8 bytes into the matching BitmapGroupSprite element, or 0 if any handle is invalid or the sequence has no
 * sprites.
 * blam-cc: ECX -> bitmap_tag, AX -> sequence_index, DI -> frame_index
 *
 * @address 0x4ab630
 */
int32_t BitmapSequenceLookup::get_bitmap_offset(datum_index bitmap_tag, int16_t sequence_index, int16_t frame_index)
{
    if (bitmap_tag == (datum_index)-1 || sequence_index == -1 || frame_index == -1) {
        return 0;
    }

    {
        Bitmap *tag_data = halo::interface::tag_data<Bitmap>(bitmap_tag);
        if (sequence_index < (int32_t)tag_data->bitmap_group_sequence.count) {
            BitmapGroupSequence *sequence =
                (BitmapGroupSequence *)tag_data->bitmap_group_sequence.pointer + sequence_index;
            int32_t sprite_count = (int32_t)sequence->sprites.count;
            if (sprite_count != 0) {
                return (frame_index % sprite_count) * 0x20 + 8 + (int32_t)sequence->sprites.pointer;
            }
        }
    }
    return 0;
}

}
