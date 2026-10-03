#include "halo/networking/net1_channel.hpp"

extern "C" {
extern void *gt2GetConnectionData(void *gamespy_connection);
extern uint32_t gamespy_array_length(int32_t object);
extern uint16_t gt2GetRemotePort(int32_t object);
extern void gt2AddressToString(uint32_t address, uint16_t port, void *out_address);
extern uint32_t circular_buffer_write(uint32_t byte_count, circular_buffer *stream, uint8_t *source);
}

namespace halo::networking {

/**
 * Original `network_channel_receive_callback`, moved unchanged; recovered notes are in docs/original/networking/net1_channel.md.
 *
 * @address 0x441ed0
 */
void ChannelCallbacks::receive_callback(void *handle, uint8_t *data, int32_t length)
{
    network_receive_queue *queue;
    uint32_t address;
    uint16_t port;
    uint8_t address_buf[24];

    queue = (network_receive_queue *)gt2GetConnectionData(handle);
    if (queue != 0) {
        if ((queue->flags & 1) == 0) {
            address = gamespy_array_length((int32_t)handle);
            port = gt2GetRemotePort((int32_t)handle);
            gt2AddressToString(address, port, address_buf);
        } else if (0 < length) {
            circular_buffer_write((uint32_t)length, queue->incoming, data);
            return;
        }
    }
    return;
}

}
