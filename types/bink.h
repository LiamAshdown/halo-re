/* bink.h -- the Bink video functions the game calls, from binkw32.dll in the Halo folder (RAD Game Tools; not built
   here). binkw32.dll exports them under their decorated names (_BinkOpen@8, ...), so they are declared under those
   names -- _BinkOpen decorates to __BinkOpen@8, the symbol standalone/libs/binkw32.def gives the import that asks the
   DLL for _BinkOpen@8 -- and the game code calls them as BinkOpen etc. through the macros. The standalone link
   delay-loads the DLL (found through the loader's SetDllDirectory to the Halo folder). A movie handle is the game's
   bink_movie_prefix (types/main.h). */
#ifndef HALO_BINK_H
#define HALO_BINK_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint32_t __stdcall _BinkOpenDirectSound(uint32_t param);
int32_t __stdcall _BinkSetSoundSystem(void *open, uint32_t param);
void *__stdcall _BinkOpen(const char *name, uint32_t flags);
void __stdcall _BinkClose(void *bink);
int32_t __stdcall _BinkPause(void *bink, int32_t pause);
int32_t __stdcall _BinkWait(void *bink);
int32_t __stdcall _BinkDoFrame(void *bink);
void __stdcall _BinkNextFrame(void *bink);
int32_t __stdcall _BinkCopyToBuffer(void *bink, void *dest, int32_t dest_pitch, uint32_t dest_height, uint32_t dest_x,
    uint32_t dest_y, uint32_t flags);

#define BinkOpenDirectSound _BinkOpenDirectSound
#define BinkSetSoundSystem _BinkSetSoundSystem
#define BinkOpen _BinkOpen
#define BinkClose _BinkClose
#define BinkPause _BinkPause
#define BinkWait _BinkWait
#define BinkDoFrame _BinkDoFrame
#define BinkNextFrame _BinkNextFrame
#define BinkCopyToBuffer _BinkCopyToBuffer

#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
#endif
