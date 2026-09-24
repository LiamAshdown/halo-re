#include "tags.h"
#include "memory.h"
#include "math.h"
int main(void){
  return (int)(sizeof(real_matrix4x3) + sizeof(real_matrix3x3) + sizeof(sphere_mesh)
             + sizeof(real_plane3d) + sizeof(periodic_function_table) + sizeof(data_array)
             + sizeof(Biped));
}
