Assignment 2 Writeup
=============

My name: Changhun Oh

My POVIS ID: pcsech16

My student ID (numeric): 20160779

This assignment took me about 4 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): I do it all alone.

## 1. Program Structure and Design of the TCPReceiver

### 1.1. Structure of TCPReceiver

I add some attribute in TCPReceiver.

```c++
WrappingInt32 _isn = WrappingInt32(0);
bool _is_received_syn = false;
bool _is_received_fin = false;
```
+ **_isn**: tracking start sequence number of TCP segment.
+ **_is_received_syn**: tracking receiving TCP segment with syn flag.
+ **_is_received_fin**: tracking receiving TCP segment with fin flag.

### 1.2. Algorithm of TCPReceiver

#### 1.2.1. Algorithm of segment_received

segment_received routine divides two parts

+ tracking syn/fin flag
    ```c++
    if (tcp_header.syn && !_is_received_syn) {
        _is_received_syn = true;
        _isn = tcp_header.seqno;
    }

    if (!_is_received_syn)
        return;

    _is_received_fin |= tcp_header.fin;
    ```
    Before receiving TCP segment with syn flag, we should prohibit from doing further progress. To archive this purpose, we're tracking whether to receive it.

+ pushing payload into reassembler
    ```c++
    const WrappingInt32 relative_str_seqno(tcp_header.seqno + tcp_header.syn);
    const string &str = seg.payload().copy();
    const size_t absolute_str_seqno = unwrap(relative_str_seqno,
                                            _isn,
                                            _reassembler.get_unassm_base());

    _reassembler.push_substring(str, absolute_str_seqno - 1, tcp_header.fin);
    ```
    It's enough easy to understand this code without explanation. Because I set naively what eof in _reassembler means, I send eof information when I receive TCP segment with fin flag. 

#### 1.2.2. Algorithm of ackno

ackno routine is like below.
```c++
if (!_is_received_syn)
    return std::nullopt;

const size_t auxiliary_term = _is_received_syn + (_is_received_fin && _reassembler.empty());
const size_t absolute_seqno = auxiliary_term + _reassembler.get_unassm_base();
return wrap(absolute_seqno, _isn);
```
Ackno should be consider as how many bytes we assembled with auxiliary term (syn/fin flag).
In case of syn flag, we don't have to care about it, but we should be care of fin flag.
Only when all characters are received and assembled, fin flag should be added in ackno.

### 1.3. Algorithm of wrap/unwrap routines

Wrap routine describes like below.

```c++
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return WrappingInt32{isn + n}; }
```

Unwrap routine describes like below.

```c++
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    const uint64_t relative_seqno = static_cast<uint32_t>(n - isn);

    if (checkpoint <= relative_seqno)
        return relative_seqno;

    const uint64_t ring_cnt = (checkpoint - relative_seqno) >> 32;
    const uint64_t lower_bound = relative_seqno + (ring_cnt << 32);
    const uint64_t upper_bound = relative_seqno + ((ring_cnt + 1) << 32);
    return (checkpoint - lower_bound) > (upper_bound - checkpoint) ? upper_bound : lower_bound;
}
```
It's based on following formula.

What to do is translation from relative sequence number ($n - isn$) to absolute sequence number that is closest from checkpoint. By using this assumption(the closest point), we can derive like below.

$$
ring\_cnt \cdot 2^{32} + relative\_seqno \le checkpoint \lt (ring\_cnt+1) \cdot 2^{32} + relative\_seqno
$$

As a result, value of ring_cnt can be evaluated.

$$
\therefore ring\_cnt = (checkpoint - relative\_seqno) / 2^{32}
$$


Therefore, there are two candidates to meet the condition; having relative sequence number ($n - isn$).

$$
    lower\_bound = ring\_cnt \cdot 2^{32} + relative\_seqno
    \\
    upper\_bound = (ring\_cnt+1) \cdot 2^{32} + relative\_seqno
$$

To find the closest point, we should compare distance from checkpoint to lower_bound and upper_bound respectively. By the way, we don't have to consider the case of being same distance to both of them, because distance from lower_bound to upper_bound has even number.

As a result, unwrap algorithm is deriven using above formula.

## 2. Implementation Challenges

In this assignment, there is no challenge for me to think hard.

Just one thing I spend a little bit is about deriving wrap and unwrap algorithm.

## 3. Remaining Bugs

I can't find any bugs in this assignment.

- Optional: I had unexpected difficulty with: At first time, concept of absolute and relative sequence number made me confused.

- Optional: I think you could make this assignment better by: It is enough good to study.

- Optional: I was surprised by: This assignment is well designed.

- Optional: I'm not sure about: Nothing about it.
