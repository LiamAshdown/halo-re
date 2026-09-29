#pragma once
/* vorbisfile.h -- the Ogg Vorbis file functions the game calls, from vorbisfile.dll in the Halo folder (Xiph.org;
   not built here). cdecl, plain export names; standalone/libs/vorbisfile.def gives the import library and the
   standalone link delay-loads the DLL. The OggVorbis_File state is carried as an opaque buffer by the sound code, and
   ov_open_callbacks takes the ov_callbacks structure by value (its four function pointers). */
#ifndef HALO_VORBISFILE_H
#define HALO_VORBISFILE_H

#include <stdint.h>

int32_t ov_open_callbacks(void *datasource, void *vorbis_file, char *initial, int32_t initial_bytes, void *read_func,
    void *seek_func, void *close_func, void *tell_func);
int32_t ov_read(void *vorbis_file, char *buffer, int32_t length, int32_t bigendian_p, int32_t word_size, int32_t is_signed,
    int32_t *bitstream_index);
int32_t ov_clear(void *vorbis_file);
int32_t ov_crosslap(void *old_vorbis_file, void *new_vorbis_file);

#endif
