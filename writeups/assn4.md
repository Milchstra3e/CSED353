Assignment 4 Writeup
=============

My name: Changhun Oh

My POVIS ID: pcsech16

My student ID (numeric): 20160779

This assignment took me about 18 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): I do it all alone.

Your benchmark results (without reordering, with reordering): [0.37 Gbit/s, 0.38 Gbit/s]

## 1. Program Structure and Design of Assignment 4

### 1.1 Structure and Design of TCPConnection
I add some attributes and functions in TCPConnection, and I omit several trivial functions.

```c++
bool _error = false;
bool _sent_fin = false;
size_t _time_since_last_segment_received = 0;

void _send_segments();
void _error_occured();
bool _prerequisite_test() const;
```
+ Private fields and methos
    + **_error**: Whenever RST flag is set, this flag will be true. In otherwords, it tells current tcp connection is in error state.
    + **_sent_fin**: Track whether to send fin flag within packet.
    + **_time_since_last_segment_received**: Time sine last segment received.
    + **_send_segments**: Send tcpsegment from tcp sender queue.
    + **_error_occured**: Whenever tcp connection is in error state, this function should be called and it will initialize variable for error state.
    + **_prerequisite_test**: Track whether to ready to finish tcp connection.

### 1.2. Algorthm of TCPConnection
I omit algorithms of several functions, because it is trivial how to do and why to be programed.

I programed program defensively, so all of tcp_connection public function is asserted whenever to be error state except active(), connect().

#### 1.2.1. 

## 2. Implementation Challenges
There is no challenge in the respect of algorithm, but there are several challenges when debugging.

In this assignment, wrapping up all of parts (both receiver and sender) required me to take care of the case that either receiver or sender maybe implemented wrong.

## 3. Remaining Bugs
I can't find any bugs in this assignment.

- Optional: I had unexpected difficulty with: I should consider all layer from bottom to top, so whenever degugging, it is hard to find where to be problem.

- Optional: I think you could make this assignment better by: It is enough good to study.

- Optional: I was surprised by: This assignment is well designed.

- Optional: I'm not sure about: Nothing about it.
