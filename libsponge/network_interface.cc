#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    if (_cache.find(next_hop_ip) != _cache.end())
        _frames_out.push(_gen_frame(next_hop_ip, dgram));
    else {
        _wait_dgram.push_back(make_pair(next_hop_ip, dgram));

        bool dup_check = false;
        for (auto &e : _wait_ARP)
            dup_check |= (e.first == next_hop_ip);

        if (!dup_check) {
            _wait_ARP.push_back(make_pair(next_hop_ip, 0));
            _frames_out.push(_gen_frame(next_hop_ip, _gen_ARPMessage(ARPMessage::OPCODE_REQUEST, next_hop_ip)));
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    const EthernetHeader &frame_header = frame.header();
    if (!(frame_header.dst == _ethernet_address || frame_header.dst == ETHERNET_BROADCAST))
        return nullopt;

    if (frame_header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;

        if (dgram.parse(frame.payload()) == ParseResult::NoError)
            return dgram;
    } else {
        ARPMessage msg;

        if (msg.parse(frame.payload()) == ParseResult::NoError && msg.target_ip_address == _ip_address.ipv4_numeric()) {
            const EthernetAddress sender_mac = msg.sender_ethernet_address;
            const uint32_t sender_ip = msg.sender_ip_address;

            _cache[sender_ip] = make_pair(sender_mac, 0);
            _update();

            if (msg.opcode == ARPMessage::OPCODE_REQUEST)
                _frames_out.push(_gen_frame(sender_ip, _gen_ARPMessage(ARPMessage::OPCODE_REPLY, sender_ip)));
        }
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // Checking ARP message whether to be over the resend timeout.
    for (auto &e : _wait_ARP) {
        e.second += ms_since_last_tick;
        if (e.second >= ARP_TIMEOUT)
            _frames_out.push(_gen_frame(e.first, _gen_ARPMessage(ARPMessage::OPCODE_REQUEST, e.first)));
    }

    // Checking Cache wheter to be over the lifetime timeout.
    auto cache_iter = _cache.begin();
    while (cache_iter != _cache.end()) {
        cache_iter->second.second += ms_since_last_tick;
        if (cache_iter->second.second >= CACHE_TIMEOUT)
            cache_iter = _cache.erase(cache_iter);
        else
            cache_iter++;
    }
}

ARPMessage NetworkInterface::_gen_ARPMessage(const uint16_t opcode, const uint32_t target_ip) {
    ARPMessage msg;
    msg.opcode = opcode;
    msg.sender_ip_address = _ip_address.ipv4_numeric();
    msg.target_ip_address = target_ip;
    msg.sender_ethernet_address = _ethernet_address;
    msg.target_ethernet_address =
        (_cache.find(target_ip) != _cache.end()) ? _cache[target_ip].first : EthernetAddress();

    return msg;
}

EthernetFrame NetworkInterface::_gen_frame(const uint32_t target_ip, const uint16_t type) {
    EthernetFrame frame;
    EthernetHeader &frame_header = frame.header();
    frame_header.type = type;
    frame_header.src = _ethernet_address;
    frame_header.dst = (_cache.find(target_ip) != _cache.end()) ? _cache[target_ip].first : ETHERNET_BROADCAST;

    return frame;
}

EthernetFrame NetworkInterface::_gen_frame(const uint32_t target_ip, const InternetDatagram dgram) {
    EthernetFrame frame = _gen_frame(target_ip, EthernetHeader::TYPE_IPv4);
    frame.payload() = dgram.serialize();
    return frame;
}

EthernetFrame NetworkInterface::_gen_frame(const uint32_t target_ip, const ARPMessage msg) {
    EthernetFrame frame = _gen_frame(target_ip, EthernetHeader::TYPE_ARP);
    frame.payload() = msg.serialize();
    return frame;
}

void NetworkInterface::_update() {
    // removing ARP Message which to arrive.
    auto ARP_iter = _wait_ARP.begin();
    while (ARP_iter != _wait_ARP.end()) {
        if (_cache.find(ARP_iter->first) != _cache.end())
            ARP_iter = _wait_ARP.erase(ARP_iter);
        else
            ARP_iter++;
    }

    // removing IP Datagram which target MAC corresponding target IP is found.
    auto dgram_iter = _wait_dgram.begin();
    while (dgram_iter != _wait_dgram.end()) {
        if (_cache.find(dgram_iter->first) != _cache.end()) {
            _frames_out.push(_gen_frame(dgram_iter->first, dgram_iter->second));
            dgram_iter = _wait_dgram.erase(dgram_iter);
        } else
            dgram_iter++;
    }
}
