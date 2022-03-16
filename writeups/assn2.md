Assignment 2 Writeup
=============

My name: Changhun Oh

My POVIS ID: pcsech16

My student ID (numeric): 20160779

This assignment took me about 4 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): I do it all alone.

## 1. Program Structure and Design of the TCPReceiver

### 1.1. Structure of TCPReceiver



### 1.2. Algorithm of TCPReceiver

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

## 3. Remaining Bugs

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
