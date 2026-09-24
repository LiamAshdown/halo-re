// Phase 4 syntax gate for types/hs.h. The host gcc is 64-bit, so struct sizes here are the
// 32-bit sizes plus 4 per pointer; every size in hs.h was checked against that rule by hand.
#include "tags.h"
#include "memory.h"
#include "hs.h"
int main(void){
  hs_syntax_node node;
  hs_thread thread;
  hs_global global;
  object_list_header header;
  hs_sleep_state sleep_state;
  node.type = _hs_type_object_list;
  node.flags = _hs_syntax_node_primitive_bit | _hs_syntax_node_global_bit;
  node.data.real_value = 0.0f;
  thread.type = _hs_thread_command;
  thread.flags = _hs_thread_pushed_frame_bit;
  global.value.datum_value = k_datum_index_none;
  header.reference_count = 0;
  sleep_state.ticks = 30;
  return (int)(sizeof(node) + sizeof(thread) + sizeof(global) + sizeof(header)
             + sizeof(sleep_state) + sizeof(hs_stack_frame) + sizeof(hs_syntax_node_data)
             + sizeof(hs_function_definition) + sizeof(hs_global_definition)
             + sizeof(hs_global_value) + sizeof(object_list_reference)
             + sizeof(object_list_iterator) + sizeof(hs_enum_definition)
             + sizeof(hs_variadic_arguments_state) + sizeof(hs_argument_list_state)
             + sizeof(hs_random_state) + sizeof(hs_boolean_state)
             + sizeof(hs_arithmetic_state) + sizeof(hs_typed_arguments_state)
             + sizeof(data_array) + sizeof(ScenarioScriptNode) + sizeof(Scenario)
             + (int)_hs_script_stub + (int)_hs_function_sleep + (int)_hs_context_host_bit
             + (int)k_hs_function_count + (int)k_hs_maximum_source_files
             + (int)k_hs_global_builtin_bit + (int)k_hs_autocomplete_required_mask);
}
