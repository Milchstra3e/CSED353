Assignment 5 Writeup
=============

My name: Changhun Oh

My POVIS ID: pcsech16

My student ID (numeric): 20160779

This assignment took me about 4 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): I do it all alone.

## 1. Program Structure and Design of Assignment5

### 1.1. Structure and Design of NetworkInterface
I add some attributes and functions in NetworkInterface, and I omit several trivial functions.

```c++
static constexpr size_t ARP_TIMEOUT = 5000;
static constexpr size_t CACHE_TIMEOUT = 30000;

std::map<uint32_t, std::pair<EthernetAddress, size_t>> _cache{};
std::list<std::pair<uint32_t, InternetDatagram>> _wait_dgram{};
std::list<std::pair<uint32_t, size_t>> _wait_ARP{};

ARPMessage _gen_ARPMessage(const uint16_t opcode, const uint32_t target_ip);
EthernetFrame _gen_frame(const uint32_t target_ip, const uint16_t type);
EthernetFrame _gen_frame(const uint32_t target_ip, const InternetDatagram dgram);
EthernetFrame _gen_frame(const uint32_t target_ip, const ARPMessage msg);

void _update();
```
+ Private fields and methods
    + **ARP_TIMEOUT**: Constant value to represent when to fire on alarm of ARP response timeout.
    + **CACHE_TIMEOUT**: Constant value to represent how much time information in the cache is alive.
    + **_cache**: Cache which maps IP into corresponding MAC(EthernetAddress).
    + **_wait_dgram**: List of IP datagrams which wait for arriving ARP response. In this list, it takes two type of elements; pair(Target Hop IP of Numerical version, IP Datagram)
    + **_wait_ARP**: List of ARP messages which wait for arriving ARP response. In this list, it takes two type of elements; pair(Target Hop IP of Numerical version, time since it sent)
    + **_gen_ARPMessage**: Generating ARP Message function regardless of either request or response.
    + **_gen_frame**: Generating Frame (Link-Layer Level information package to send through network) function. There are three functions which have same name. I use function overloading because three functions exactly have same purpose.
    + **_update**: Updating function when there is a change in the cache information. In this function, it will update both _wait_dgram and _wait_ARP.

### 1.2. Algorithm of NetworkInterface
I omit algorithms of several functions, because it is trivial how to do and why to be programmed.

#### 1.2.1. Algorithm of send_datagram
```c++
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
```
Flow chart of this algorithm is like below.
1. It check whether to be able to find IP-MAC relation from _cache and if so, it will send frame containing IP datagram to send.
2. If not, it will send the frame with ARP Message and push IP datagram into _wait_dgram and ARP message into _wait_ARP.
Importent point is checking redundant sending within timeout. We should be care of it.

#### 1.2.2. Algorithm of recv_frame
```c++
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
```
Flow chart of this algorithm is like below.
1. Check whether target MAC is mine.
2. If so, check type of Network-layer format and if IPv4, it will return datagram extracted from received frame.
3. If not IPv4 but ARP, it will check whether target IP is mine.
4. After then, if it matches, it will parse information of ARP and process by the opcode seperately.

#### 1.2.3. Algorithm of tick
```c++
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
```
This function devided two parts.
+ **Checking ARP Message**: In this part, it check whether to be over the timeout to wait. If so, it'll resend the ARP Message. 
+ **Checking Cache**: In this part, it check whether to be over the timeout to be alive. If so, it'll remove the IP-MAC relation from the _cache.

#### 1.2.4. Algorithm of _update
```c++
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
```
This function devided two parts.
+ **Removing ARP Message**: Whenever _cache is updated and this function is called, it first check whether what the ARP message has wait for is resolved. If so, it will remove that element from list of waiting ARP message.
+ **Removing IP Datagram**: Whenever _cache is updated and this function is called, it check whether what the IP Datagram has wait for is resolved. If so, it will remove that element from list of wating IP Datagram and it'll send the frame containing that IP datagram to next target hop IP.

## 2. Implementation Challenges:
There is no challenge in this assignment, because what to do is just pragramming specified in the ARP protocol.

However, I think carefully about how to manage IP datagram and ARP Message waiting for ARP Response. When I consider solving this problem by using list data structure, what to contain in that list element is main point for me. In my opinion, simple structure is best and I tried to find the way to represent element as the pair of primitive type as long as possible. However, in the case of _wait_dgram, I choose type of element as pair of uint32_t and InternetDatagram. In this case, it is violated the rule what I set, but it is best way to express shortly and briefly so I choose it.

## 3. Remaining Bugs:
I can't find any bugs in this assignment.

- Optional: I had unexpected difficulty with: There is no difficulty.

- Optional: I think you could make this assignment better by: It is enough good to study.

- Optional: I was surprised by: This assignment is well designed.

- Optional: I'm not sure about: Nothing about it.
