// targa_export  (Ghidra: targa_export, already named)
// address 0x43fe60, size 277 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: cea-pdb hint "targa_export (main)" via the three error strings it returns
//   ("couldn't open file", "couldn't write header", "couldn't write row"); calls
//   file_reference_create/_open/_write/_close (src/saved_games) and
//   bitmap_data_get_row_address (0x43f8e0, out of this session's range); builds a targa_header
//   on the stack (types/bitmaps.h) and writes it, then writes bitmap->height rows of
//   width * 4 bytes each, one bitmap_data_get_row_address call per row at mip level 0.
//   src/main/screenshot_render.c and src/main/movie_capture_frame_export.c already declare and
//   call this with the (BitmapData *, file_reference_record *) signature used here.
// register convention: EAX bitmap, EBX file_reference_record (destination), per
//   bitmaps_types_notes.md ("0x43fe60: EAX BitmapData, EBX file_reference_record").
//   // blam-cc: EAX -> bitmap, EBX -> destination
// Mip level verified against objdump (phase 4 review): 0x43ff03 xor eax,eax sets AX = 0 before
//   each call 0x43f8e0, with push 0 (x) and push ebp (y = row) on the stack.
// bitmap_data_get_row_address itself (0x43f8e0) was written by another agent in this session;
// its extern below uses that file's actual signature.

#include "tags.h"
#include "bitmaps.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // 0x5557a0, src/saved_games/file_reference_open.c
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, src/saved_games/file_reference_write.c
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, src/saved_games/file_reference_close.c

extern void *bitmap_data_get_row_address(BitmapData *bitmap_data, int16_t mip_level, int16_t x, int16_t y); // 0x43f8e0, this module

// blam-cc: EAX -> bitmap, EBX -> destination
// Writes bitmap's mip level 0 as an uncompressed 32-bit-per-pixel Targa file at destination.
// Returns NULL on success, or a static error string ("couldn't open file", "couldn't write
// header", "couldn't write row") on failure.
char *targa_export(BitmapData *bitmap, file_reference_record *destination)
{
    targa_header header;
    char *error;
    int32_t row;
    int32_t row_byte_size;
    void *row_pixels;

    if (file_reference_create(destination) != 0 &&
        file_reference_open(destination, _file_open_write) != 0) {

        header.id_length = 0;
        header.color_map_type = 0;
        header.image_type = k_targa_image_type_true_color;
        header.color_map_first_entry = 0;
        header.color_map_length = 0;
        header.color_map_entry_size = 0;
        header.x_origin = 0;
        header.y_origin = 0;
        header.width = (int16_t)bitmap->width;
        header.height = (int16_t)bitmap->height;
        header.bits_per_pixel = k_targa_bits_per_pixel;
        header.image_descriptor = k_targa_image_descriptor_top_left_8_alpha;

        error = 0;
        if (file_reference_write(destination, &header, sizeof(header)) == 0) {
            error = "couldn't write header";
        } else if (0 < (int16_t)bitmap->height) {
            row_byte_size = (int32_t)(int16_t)bitmap->width * 4;
            row = 0;
            do {
                row_pixels = bitmap_data_get_row_address(bitmap, 0, 0, (int16_t)row);
                if (file_reference_write(destination, row_pixels, row_byte_size) == 0) {
                    file_reference_close(destination);
                    return "couldn't write row";
                }
                row = row + 1;
            } while (row < (int16_t)bitmap->height);
            file_reference_close(destination);
            return 0;
        }
        file_reference_close(destination);
        return error;
    }
    return "couldn't open file";
}

#if 0
Original Ghidra decompilation (0x43fe60):

char * targa_export(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  char *local_18;

  iVar2 = 0;
  local_18 = (char *)0x0;
  cVar1 = file_reference_create();
  if ((cVar1 != '\0') && (cVar1 = file_reference_open(2), cVar1 != '\0')) {
    cVar1 = file_reference_write();
    if (cVar1 == '\0') {
      local_18 = "couldn\'t write header";
    }
    else if (0 < *(short *)(in_EAX + 6)) {
      do {
        bitmap_data_get_row_address(0,iVar2);
        cVar1 = file_reference_write();
        if (cVar1 == '\0') {
          file_reference_close();
          return "couldn\'t write row";
        }
        iVar2 = iVar2 + 1;
      } while (iVar2 < *(short *)(in_EAX + 6));
      file_reference_close();
      return (char *)0x0;
    }
    file_reference_close();
    return local_18;
  }
  return "couldn\'t open file";
}
#endif
