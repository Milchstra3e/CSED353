#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

RetxManager::RetxManager(const uint16_t retx_timeout)
    : _initial_retx_timeout(retx_timeout), _current_retx_timeout(retx_timeout) {}

void RetxManager::update(const uint64_t absolute_ackno) {
    if (!_alarm_on() || absolute_ackno <= _curr_ackno)
        return;
    _curr_ackno = absolute_ackno;

    while (!_wait_segments.empty()) {
        const TCPSegment &earliest_segment = _wait_segments.front().second;
        const uint64_t earliest_absolute_upperbound =
            _wait_segments.front().first + earliest_segment.length_in_sequence_space();

        if (earliest_absolute_upperbound > absolute_ackno)
            break;

        _wait_segments.pop();
    }

    alarm_reset();
}

void RetxManager::alarm_reset() {
    _current_retx_timeout = _initial_retx_timeout;
    _elapsed_time = 0;
    _consecutive_retx = 0;
}

void RetxManager::push_segment(const uint64_t absolute_seqno, const TCPSegment segment) {
    if (!_alarm_on())
        alarm_reset();

    _wait_segments.push(make_pair(absolute_seqno, segment));
}

optional<TCPSegment> RetxManager::alarm_check(const size_t ms_since_last_tick, const uint16_t receiver_window_size) {
    if (!_alarm_on())
        return nullopt;

    _elapsed_time += ms_since_last_tick;
    if (_elapsed_time < _current_retx_timeout)
        return nullopt;

    if (receiver_window_size) {
        _consecutive_retx++;
        _current_retx_timeout *= 2;
    }

    _elapsed_time = 0;
    return _wait_segments.front().second;
}

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retx_manager(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return {}; }

void TCPSender::fill_window() {}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { DUMMY_CODE(ackno, window_size); }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {}

TCPSegment TCPSender::_gen_segment(WrappingInt32 seqno, bool syn, bool fin, string payload) {
    TCPSegment segment;

    TCPHeader &tcp_header = segment.header();
    Buffer &tcp_payload = segment.payload();

    tcp_header.seqno = seqno;
    tcp_header.syn = syn;
    tcp_header.fin = fin;

    tcp_payload = Buffer(move(payload));

    return segment;
}

