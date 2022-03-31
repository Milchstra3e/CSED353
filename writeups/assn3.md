Assignment 3 Writeup
=============

My name: Changhun Oh

My POVIS ID: pcsech16

My student ID (numeric): 20160779

This assignment took me about 10 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): I do it all alone.

## 1. Program Structure and Design of Assignment 3

### 1.1. Structure and Design of RetxManager
RetxManager is helper class for TCPSender to track retransmission information. More specific information is decribed below.

#### 1.1.1. Structure of RetxManager
I add some attributes and functions in RetxManager, and I omit several trivial functions.

```c++
private:
    std::queue<std::pair<uint64_t, TCPSegment>> _wait_segments{};

    unsigned int _initial_retx_timeout;
    unsigned int _current_retx_timeout = 0;
    unsigned int _consecutive_retx = 0;
    size_t _elapsed_time = 0;

    bool _alarm_on() const { return !_wait_segments.empty(); };

public:
    void update(const uint64_t absolute_ackno);
    void alarm_reset();
    void push_segment(const uint64_t absolute_seqno, const TCPSegment segment);

    std::optional<TCPSegment> check_alarm(const size_t ms_since_last_tick, const uint16_t receiver_window_size);
```
+ Private fields and methods
    + **_wait_segments**: Queued segments that is waiting for ackno. It is consist of both expected ackno and segment information itself.
    + **_initial_retx_timeout**: Initial time length of retransmission timeout.
    + **_current_retx_timeout**: Currently applied time length of retransmission timeout. It would be changed due to exponential backoff.
    + **_consecutive_retx**: Tracking how many times segment has been sent repeatly.
    + **_elapsed_time**: Elapsed time since alarm is on.
    + **_alarm_on**: Whether to run alarm now.
+ Public methods
    + **update**: Whenever received new ackno and call this, it will update _wait_segments.
    + **alarm_reset**: Reset alarm or Turn off alarm.
    + **push_segment**: Add new segment into _wait_segments.
    + **check_alarm**: As time passed, it is tracking whether to resend segments.

#### 1.1.2. Algorithm of RetxManager
I omit algorithm of several functions, because it is trivial how to do and why to be programmed.

##### 1.1.2.1. Algorithm of update
```c++
while (!_wait_segments.empty()) {
    if (_wait_segments.front().first > absolute_ackno) break;
    _wait_segments.pop();
    alarm_reset();
}
```
Update routine is quite simple. It just compare received ackno with ackno of the earlist pushed segment in queue. If the former is larger than or equal to the later, it need not to wait anymore, so pop out that segment.

##### 1.1.2.2. Algorithm of check_alarm
```c++
_elapsed_time += ms_since_last_tick;
if (_elapsed_time < _current_retx_timeout) return nullopt;

if (receiver_window_size) {
    _consecutive_retx++;
    _current_retx_timeout *= 2;
}

_elapsed_time = 0;
return _wait_segments.front().second;
```
Check_alarm routine is also quite simple. If tracked elapsed time is over than timeout, it will return what to resend and affect actual retransmission timeout(_current_retx_timeout).

### 1.2. Structure and Design of TCPSender

#### 1.2.1. Structure of TCPSender
I add some attributes and functions in TCPSender, and I omit several trivial attributes and functions.

```c++
uint64_t _curr_ackno = 0;
uint16_t _receiver_window_size = 1;
uint16_t _remain_window_size = 1;

RetxManager _retx_manager;

bool _sent_fin = false;

TCPSegment _gen_segment(uint64_t absolute_seqno, bool syn, bool fin, std::string payload);
```
+ **_curr_ackno**: The highest received ackno that is less than or equal to _next_seqno.
+ **_receiver_window_size**: The most recently received window_size.
+ **_remain_window_size**: How many sequences TCPSender can send now.
+ **_retx_manager**: Tracker for retrasmission.
+ **_sent_fin**: Tracking whether to send packet containing flag **fin**.
+ **_gen_segment**: Packing all information into TCPSegment.

#### 1.2.2. Algorithm of TCPSender
I omit algorithm of several functions, because it is trivial how to do and why to be programmed.

##### 1.2.2.1. Algorithm of fill_window
Fill_window function is divided into four parts.

+ Computation all of related flags
    ```c++
    const bool contain_syn = (_next_seqno == 0);
    const bool contain_fin = (_stream.input_ended() && !_sent_fin);
    const uint16_t required_window_size = contain_syn + _stream.buffer_size() + contain_fin;
    const bool has_enough_window = _remain_window_size >= required_window_size;
    const bool has_flag = contain_syn || (has_enough_window && contain_fin);
    ```
    + **contain_syn**: In the case of ideal situation, after calling fill_window, if one segment has flag **syn**, then this flag set true.
    + **contain_fin**: In the case of ideal situation, after calling fill_window, if one segment has flag **fin**, then this flag set true.
    + **required_window_size**: It means how many window size are needed to send succesfully all information in the case of ideal situation.
    + **has_enough_window**: Whether to support required_window_size in current real situation.
    + **has_flag**: If there is one segment to have flag either **syn** or **fin** in current real situation, this flag set true. 

+ Calculate actual used window / payload size
    ```c++
    const uint16_t window_size = min(_remain_window_size, required_window_size);
    const uint16_t payload_size = window_size - contain_syn - (has_enough_window && contain_fin);
    ```
    + **window_size**: How many window_size are used now.
    + **payload_size**: How many bytes are sent now.

+ Calculate actual the number of segment
    ```c++
    const unsigned int payload_segment_cnt = (payload_size + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
    const unsigned int segment_cnt = max(payload_segment_cnt, static_cast<unsigned int>(has_flag));
    ```
    + **payload_segment_cnt**: The number of segment to send all bytes.
    + **segment_cnt**: The number of segment to send all bytes with flag **syn** or **fin** (if needed).

+ Fill each segment with flag and bytes
    ```c++
    for (unsigned int i = 0; i < segment_cnt; i++) {
        const bool is_first_iter = (i == 0);
        const bool is_last_iter = (i + 1 == segment_cnt);

        const bool syn = is_first_iter && contain_syn;
        const bool fin = is_last_iter && has_enough_window && contain_fin;

        const uint16_t remain_payload_size = payload_size - i * MAX_PAYLOAD_SIZE;
        const uint16_t read_bytes = min(remain_payload_size, MAX_PAYLOAD_SIZE);
        const string payload = _stream.read(read_bytes);

        const TCPSegment segment = _gen_segment(_next_seqno, syn, fin, payload);

        _next_seqno += segment.length_in_sequence_space();
        _remain_window_size -= segment.length_in_sequence_space();
        _sent_fin |= fin;

        _segments_out.push(segment);
        _retx_manager.push_segment(_next_seqno, segment);
    }
    ```
    Fill each segment with flag and bytes by using variable derived from above three steps.

##### 1.2.2.2. Algorithm of ack_received
```c++
const uint64_t absolute_ackno = unwrap(ackno, _isn, _next_seqno);

if (absolute_ackno <= _next_seqno) {
    _curr_ackno = max(_curr_ackno, absolute_ackno);
    _receiver_window_size = window_size;

    const uint16_t actual_window_size = max(window_size, uint16_t(1));
    const uint64_t bytes_in_flight_size = bytes_in_flight();

    if (actual_window_size > bytes_in_flight_size)
        _remain_window_size = actual_window_size - bytes_in_flight_size;
    else
        _remain_window_size = 0;

    _retx_manager.update(_curr_ackno);
}
```
Check whether received ackno is valid by comparing _next_seqno, after then update _curr_ackno, _remain_window_size and _retx_manager with window_size and received ackno.

## 2. Implementation Challenges
In this assignment 3, there are two implementation challenges.

## 2.1. Design of RetxManager Class
It is big challenge in this assignment. I want to avoid using unnecessary duplicated parameter, because it will lead to the problem of synchronization between RetxManager and TCPSender. Therefore, I had to decide how many kinds of information should be given as initialized factors and parameters.

## 2.2. Design of fill_dinwo function
It is easy to program without generality. First of time, I solve this assignment by dividing some cases. It bring about breaking generality and being not so beautiful. Therefore, after then, I tried to find the way how to generalied generation of segment. The answer is this code.

## 3. Remaining Bugs
I can't find any bugs in this assignment.

- Optional: I had unexpected difficulty with: I'm very sensitive to variable name, so whenever I assign name to variable, it takes at most 30 minutes.

- Optional: I think you could make this assignment better by: It is enough good to study.

- Optional: I was surprised by: This assignment is well designed.

- Optional: I'm not sure about: Nothing about it.
