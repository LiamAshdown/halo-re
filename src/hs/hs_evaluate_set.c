// hs_evaluate_set  (not a Ghidra function; the evaluate handler of hs function 4 "set")
// address 0x488fd0, size 331 bytes
// name confidence: 0.9  rewrite confidence: 0.85
// evidence: hs_function_definitions 0x688b58[4] -> record 0x6576d0, name "set", evaluate (+0xc)
//   0x488fd0; only reachable through that pointer. First-boot track: the UI map's scripts run it.
// objdump 0x488fd0..0x48911a: reserves 4 unused scratch bytes. The variable node is the call's
//   second child; its word data is the global reference, whose type comes from
//   hs_global_definitions (bit 15 set) or the scenario's globals (stride 0x5c, +0x20).
//   First call: an object_list global (type 0x17) drops one reference from its current list
//   (object_list_header_data element +4, stride 0xc); then the value expression (the variable's
//   next_node) is pushed straight into the global's hs_globals_data slot (+4, stride 8; builtin
//   index, or scenario index + 0x1eb).
//   Second call: hs_global_write_value(reference); an object_list global gains one reference
//   on its new list; returns hs_global_get_value(reference).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern data_array *hs_globals_data; // 0x0087a46c
extern data_array *object_list_header_data; // 0x0087a464
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern Scenario *global_scenario; // 0x00746f8c

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & 0xffff) * 0x14);
}

static void object_list_adjust_references(hs_global_reference reference, int16_t delta)
{
    int32_t list = hs_global_get_value(reference);

    if (list != k_datum_index_none) {
        *(int16_t *)((uint8_t *)object_list_header_data->data + (list & 0xffff) * 0xc + 4) += delta;
    }
}

void hs_evaluate_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    hs_syntax_node *variable;
    hs_global_reference reference;
    uint16_t index;
    hs_type_t type;
    uint32_t slot;

    variable = syntax_get(syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node);
    thread->stack->size = thread->stack->size + 4;
    reference = (hs_global_reference)variable->data.global_reference;
    index = reference & k_hs_global_index_mask;
    if ((reference & k_hs_global_builtin_bit) != 0) {
        type = hs_global_definitions[index]->type;
    } else {
        type = ((ScenarioGlobal *)global_scenario->globals.pointer)[index].type;
    }

    if (first != 0) {
        if (type == 0x17) {
            object_list_adjust_references(reference, -1);
        }
        slot = (reference & k_hs_global_builtin_bit) != 0 ? index : index + k_hs_builtin_global_count;
        hs_thread_push(variable->next_node, thread_index,
            (uint8_t *)hs_globals_data->data + (slot & 0xffff) * 8 + 4);
        return;
    }
    hs_global_write_value(reference);
    if (type == 0x17) {
        object_list_adjust_references(reference, 1);
    }
    hs_thread_return(hs_global_get_value(reference), thread_index);
}
