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

#### 1.2.1. Algorithm of segment_received
```c++
const bool check_keep_alive_seg = _receiver.ackno().has_value() &&
                                    seg.length_in_sequence_space() == 0 &&
                                    seg.header().seqno == _receiver.ackno().value() - 1;
if (check_keep_alive_seg)
    _sender.send_empty_segment();
else {
    _receiver.segment_received(seg);

    if (seg_header.ack)
        _sender.ack_received(seg_header.ackno, seg_header.win);
    else
        _sender.fill_window();

    if (seg.length_in_sequence_space() && _sender.segments_out().empty())
        _sender.send_empty_segment();

    if (_receiver.stream_out().eof() && !(_sender.stream_in().eof() && _sent_fin))
        _linger_after_streams_finish = false;
}

_send_segments();
```
1. checking keep_alive_seg.
2. If not, tcp connection notify receiver to receive new segment.
3. tcp connection also notify sender to receive new ackno.
4. If tcp connection receive meaningless packet, it also send meaningless packet to continue connection efficiently.
5. As described assignment4, checking passive close.
6. After all done, call _send_segments


#### 1.2.2. Algorithm of _send_segments
```c++
queue<TCPSegment> &wait_segments = _sender.segments_out();

while (!wait_segments.empty()) {
    TCPSegment seg = wait_segments.front();
    TCPHeader &seg_header = seg.header();

    _sent_fin |= seg_header.fin;

    const optional<WrappingInt32> recv_ackno = _receiver.ackno();
    if (recv_ackno.has_value()) {
        seg_header.ack = true;
        seg_header.ackno = recv_ackno.value();
    }

    const size_t recv_window_size = _receiver.window_size();
    const size_t max_window_size = static_cast<size_t>(numeric_limits<uint16_t>().max());

    seg_header.win = min(recv_window_size, max_window_size);
    seg_header.rst = _error;

    _segments_out.push(seg);
    wait_segments.pop();
}
```
1. getting queue of wait segments and until queue is empty, below algorithm is repeated.
2. checking and updating current segment has fin flag.
3. reflecting receiver ackno into send segment.
4. reflecting receiver window into send segment.
5. reflecting error state into send segment.
6. push segment into wait_segments.

## 2. Implementation Challenges
There is no challenge in the respect of algorithm, but there are several challenges when debugging.

In this assignment, wrapping up all of parts (both receiver and sender) required me to take care of the case that either receiver or sender maybe implemented wrong.

## 3. Remaining Bugs
I can't find any bugs in this assignment.

- Optional: I had unexpected difficulty with: I should consider all layer from bottom to top, so whenever degugging, it is hard to find where to be problem.

- Optional: I think you could make this assignment better by: It is enough good to study.

- Optional: I was surprised by: This assignment is well designed.

- Optional: I'm not sure about: Nothing about it.
