// Phase 4 syntax gate for types/cache.h. The host gcc is 64-bit, so struct sizes here are the
// 32-bit sizes plus 4 per pointer; every size in cache.h was checked against that rule by hand.
#include "tags.h"
#include "memory.h"
#include "cache.h"
int main(void){
  cache_file_slot slot;
  cache_io_request request;
  tag_iterator iterator;
  map_download_state download;
  slot.header.version = k_cache_file_version;
  request.data_file_index = _cache_io_data_file_sounds;
  iterator.group_tag = _tag_group_gbxmodel;
  download.progress = 0.0f;
  return (int)(sizeof(cache_file_header) + sizeof(cache_file_tag_header) + sizeof(tag_instance)
             + sizeof(file_time) + sizeof(cache_io_completion) + sizeof(data_file)
             + sizeof(data_file_reference) + sizeof(sound_cache_entry)
             + sizeof(texture_cache_entry) + sizeof(cache_file_slot_category)
             + sizeof(cache_file_download_status) + sizeof(slot) + sizeof(request)
             + sizeof(iterator) + sizeof(download)
             + sizeof(data_array) + sizeof(cache) + sizeof(BitmapData)
             + sizeof(SoundPermutation) + sizeof(ScenarioBSP));
}
