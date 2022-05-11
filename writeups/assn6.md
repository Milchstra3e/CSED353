Assignment 6 Writeup
=============

My name: Changhun Oh

My POVIS ID: pcsech16

My student ID (numeric): 20160779

This assignment took me about 2 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): I do it all alone.

## 1. Program Structure and Design of Assignment6

### 1.1. Structure and Design of Router
I add some attributes and functions in Router, and I omit several trivial functions.

```c++
struct elem {
    uint32_t route_prefix = 0;
    std::optional<Address> next_hop{};
    size_t interface_num = 0;
};

std::multimap<uint8_t, elem, std::greater<uint8_t>> _route_table{};
```
+ Private Structure

    I defined new structure for multimap element. Each variable means like below.
    + route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against.
    + next_hop: next_hop The IP address of the next hop.
    + interface_num: The index of the interface to send the datagram out on. (It is related to attributes: std::vector\<AsyncNetworkInterface\> _interfaces.)

+ Private Attribute
    + _route_table: multimap mapping prefix_length with elem described above. (It is automatically sorted by descending order.)

### 1.2. Algorithm of Router
I omit algorithms of several functions, because it is trivial how to do and why to be programmed.

#### 1.2.1. Algorithm of route_one_datagram
```c++
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
```
1. checking whether to expire ttl. (It is less or equal than one.)
2. comparing each element in map.
3. If finding, sending next hop or datagram destination.

## 2. Implementation Challenges
There is no challenge in this assignment, because what to do is just programming router to switch datagram flow according to given ip information.

One thing thinking hard is about which data structure I should use to solve router switching problem. After consideration, I used multimap.

## 3. Remaining Bugs
I can't find any bugs in this assignment.

- Optional: I had unexpected difficulty with: There is no difficulty.

- Optional: I think you could make this lab better by: It is enough good to study.

- Optional: I was surprised by: This assignment is weel designed.

- Optional: I'm not sure about: Nothing about it.
