// camera_debug_save_to_file  (Ghidra: camera_debug_save_to_file, already named)
// address 0x445880, size 192 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: opens "camera.txt" (.data 0x00660024) with mode "w" (0x00660030) and writes four
//   lines with "%f %f %f\n" (0x00660018) / "%f\n" (0x0065fc28): observer_camera position
//   (0x006ac6d0), forward (0x006ac6f0), up (0x006ac6fc) and field_of_view (0x006ac708) of
//   observers[0]. camera_debug_load_from_file (0x445940) reads the same four lines back.
//   chimera sig__camera_coord_sig lands inside this function (0x44589d, the first fld).
// register convention: none; cdecl, no arguments.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#include "fn_camera.h"

extern observer observers[1]; // 0x006ac65c


// Writes the local player's final camera (position, forward, up, field of view) to camera.txt.
void camera_debug_save_to_file(void)
{
    void *file = fopen("camera.txt", "w");
    observer_camera *camera = &observers[0].camera;

    if (file != 0) {
        fprintf(file, "%f %f %f\n", (double)camera->position.x, (double)camera->position.y,
            (double)camera->position.z);
        fprintf(file, "%f %f %f\n", (double)camera->forward.i, (double)camera->forward.j,
            (double)camera->forward.k);
        fprintf(file, "%f %f %f\n", (double)camera->up.i, (double)camera->up.j, (double)camera->up.k);
        fprintf(file, "%f\n", (double)camera->field_of_view);
        fclose(file);
    }
}

#if 0
Original Ghidra decompilation (0x445880):

void camera_debug_save_to_file(void)

{
  FILE *_File;

  _File = (FILE *)FUN_00624186(&DAT_00660024,0x660030);
  if (_File != (FILE *)0x0) {
    _fprintf(_File,"%f %f %f\n",(double)DAT_006ac6d0,(double)DAT_006ac6d4,(double)DAT_006ac6d8);
    _fprintf(_File,"%f %f %f\n",(double)_DAT_006ac6f0,(double)_DAT_006ac6f4,(double)_DAT_006ac6f8);
    _fprintf(_File,"%f %f %f\n",(double)_DAT_006ac6fc,(double)_DAT_006ac700,(double)_DAT_006ac704);
    _fprintf(_File,"%f\n",(double)_DAT_006ac708);
    _fclose(_File);
  }
  return;
}
#endif
