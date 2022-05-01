#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

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

    if(_cache.find(next_hop_ip) != _cache.end())
        _frames_out.push(_gen_frame(next_hop_ip, dgram));
    else {
        _wait_dgram.push_back(make_pair(next_hop_ip, dgram));
        
        bool dup_check = false;
        for (auto &e : _wait_ARP)
            dup_check |= (e.first == next_hop_ip);

        if(!dup_check) {
            _wait_ARP.push_back(make_pair(next_hop_ip, 0));
            _frames_out.push(_gen_frame(next_hop_ip, _gen_ARPMessage(ARPMessage::OPCODE_REQUEST, next_hop_ip)));
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }


ARPMessage NetworkInterface::_gen_ARPMessage(const uint16_t opcode, const uint32_t target_ip) {
    ARPMessage msg;
    msg.opcode = opcode;
    msg.sender_ip_address = _ip_address.ipv4_numeric();
    msg.target_ip_address = target_ip;
    msg.sender_ethernet_address =  _ethernet_address;
    msg.target_ethernet_address = (_cache.find(target_ip) != _cache.end()) ? _cache[target_ip].first : EthernetAddress();

    return msg;
}

EthernetFrame NetworkInterface::_gen_frame(const uint32_t target_ip) {
    EthernetFrame frame;
    EthernetHeader &frame_header = frame.header();
    frame_header.type = EthernetHeader::TYPE_ARP;
    frame_header.src = _ethernet_address;
    frame_header.dst = (_cache.find(target_ip) != _cache.end()) ? _cache[target_ip].first : ETHERNET_BROADCAST;

    return frame;
}

EthernetFrame NetworkInterface::_gen_frame(const uint32_t target_ip, const InternetDatagram dgram) {
    EthernetFrame frame = _gen_frame(target_ip);
    frame.payload() = dgram.serialize();
    return frame;
}

EthernetFrame NetworkInterface::_gen_frame(const uint32_t target_ip, const ARPMessage msg) {
    EthernetFrame frame = _gen_frame(target_ip);
    frame.payload() = msg.serialize();
    return frame;
}