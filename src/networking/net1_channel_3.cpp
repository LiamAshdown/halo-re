#include "halo/networking/net1_channel.hpp"
#include "halo/memory/api.hpp"

extern "C" {
extern void *gt2GetConnectionData(void *gamespy_connection);
extern uint32_t gamespy_array_length(int32_t object);
extern uint16_t gt2GetRemotePort(int32_t object);
extern void gt2AddressToString(uint32_t address, uint16_t port, void *out_address);
}

namespace halo::networking {

/**
 * out/phase4/networking_functions.md summary ("per-channel data-received handler:
 * records the sender's address for connectionless channels, or appends the payload to the
 * channel's receive circular buffer for connection-oriented ones"); the tested flag (+0x0c bit
 * 0) is network_receive_queue.flags bit0 "connection oriented" per types/networking.h, and the
 * registered lookup gt2GetConnectionData(gamespy_handle) -> network_receive_queue* is reused from
 * network_connection_stats_record_packet.c, whose evidence note documents the same callee.
 *
 * @address 0x441ed0
 */
void ChannelCallbacks::on_receive(void *handle, uint8_t *data, int32_t length)
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
            halo::memory::circular_buffer_write((uint32_t)length, queue->incoming, data);
            return;
        }
    }
    return;
}

}
