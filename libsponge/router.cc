#include "router.hh"

#include <iostream>

using namespace std;

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    _route_table.insert(pair<uint8_t, elem>(prefix_length, elem{route_prefix, next_hop, interface_num}));
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    auto &dgram_header = dgram.header();

    if (dgram_header.ttl <= 1)
        return;

    dgram_header.ttl--;

    for (const auto &e : _route_table) {
        const uint8_t prefix_length = e.first;
        const uint32_t route_prefix = e.second.route_prefix;
        const optional<Address> next_hop = e.second.next_hop;
        const size_t interface_num = e.second.interface_num;

        uint32_t route_prefix_applied = prefix_length ? route_prefix >> (32 - prefix_length) : 0;
        uint32_t dgram_dst_applied = prefix_length ? dgram_header.dst >> (32 - prefix_length) : 0;

        if (route_prefix_applied == dgram_dst_applied) {
            const Address target =
                next_hop.has_value() ? next_hop.value() : Address::from_ipv4_numeric(dgram_header.dst);
            interface(interface_num).send_datagram(dgram, target);
            break;
        }
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
