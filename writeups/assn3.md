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

## 2. Implementation Challenges

## 3. Remaining Bugs

I can't find any bugs in this assignment.

- Optional: I had unexpected difficulty with: I'm very sensitive to variable name, so whenever I assign name to variable, it takes at most 30 minutes.

- Optional: I think you could make this assignment better by: It is enough good to study.

- Optional: I was surprised by: This assignment is well designed.

- Optional: I'm not sure about: Nothing about it.
