// Phase 4 syntax gate for types/bitmaps.h. No pointers in the new structs, so the 64-bit host
// sizes equal the 32-bit ones; the switch below fails to compile if any size is wrong.
#include "tags.h"
#include "memory.h"
#include "bitmaps.h"
int main(void){
  real_hsv_color hsv;
  targa_header header;
  dxt_color_block color;
  dxt3_block dxt3;
  dxt5_block dxt5;
  BitmapData bitmap;
  Bitmap group;
  datum_index tag = (datum_index)k_datum_index_none;
  switch (0) {
  case 0: break;
  case (sizeof(real_hsv_color) == 0x0c) ? 1 : 0: break;
  case (sizeof(targa_header) == 0x12) ? 2 : 0: break;
  case (sizeof(dxt_color_block) == 0x08) ? 3 : 0: break;
  case (sizeof(dxt3_block) == 0x10) ? 4 : 0: break;
  case (sizeof(dxt5_block) == 0x10) ? 5 : 0: break;
  case (sizeof(BitmapData) == 0x30) ? 6 : 0: break;
  case (sizeof(Bitmap) == 0x6c) ? 7 : 0: break;
  }
  hsv.hue = 0.0f;
  header.image_type = k_targa_image_type_true_color;
  header.bits_per_pixel = k_targa_bits_per_pixel;
  header.image_descriptor = k_targa_image_descriptor_top_left_8_alpha;
  color.indices = 0;
  dxt3.color = color;
  dxt5.color = color;
  bitmap.bitmap_class = k_bitmap_data_signature;
  bitmap.flags = _bitmap_data_compressed_bit | _bitmap_data_texture_cache_bit;
  group.type = bitmaptype_sprites;
  return (int)(tag + sizeof(hsv) + sizeof(header) + sizeof(dxt3) + sizeof(dxt5) + sizeof(group)
             + k_bitmap_data_format_count + _color_interpolation_long_hue_path_bit
             + k_targa_header_size + k_bitmap_data_valid_flags_mask);
}
