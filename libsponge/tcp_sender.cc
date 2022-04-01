#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

using namespace std;

RetxManager::RetxManager(const uint16_t retx_timeout) : _initial_retx_timeout(retx_timeout) {}

void RetxManager::update(const uint64_t absolute_ackno) {
    if (!_alarm_on())
        return;

    while (!_wait_segments.empty()) {
        if (_wait_segments.front().first > absolute_ackno)
            break;

        _wait_segments.pop();
        alarm_reset();
    }
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

optional<TCPSegment> RetxManager::check_alarm(const size_t ms_since_last_tick, const uint16_t receiver_window_size) {
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
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retx_manager(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _curr_ackno; }

void TCPSender::fill_window() {
    const uint16_t MAX_PAYLOAD_SIZE = uint16_t(TCPConfig::MAX_PAYLOAD_SIZE);

    // Computation all of related flags
    const bool contain_syn = (_next_seqno == 0);
    const bool contain_fin = (_stream.input_ended() && !_sent_fin);
    const size_t required_window_size = contain_syn + _stream.buffer_size() + contain_fin;
    const bool has_enough_window = _remain_window_size >= required_window_size;
    const bool has_flag = contain_syn || (has_enough_window && contain_fin);

    // Calculate actual used window / payload size
    const uint16_t window_size = min(static_cast<size_t>(_remain_window_size), required_window_size);
    const uint16_t payload_size = window_size - contain_syn - (has_enough_window && contain_fin);

    // Calculate actual the number of segment
    const unsigned int payload_segment_cnt = (payload_size + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
    const unsigned int segment_cnt = max(payload_segment_cnt, static_cast<unsigned int>(has_flag));

    // Fill each segment with flag and bytes
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
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
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

        fill_window();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    const optional<TCPSegment> retx_segment = _retx_manager.check_alarm(ms_since_last_tick, _receiver_window_size);

    if (retx_segment.has_value())
        _segments_out.push(retx_segment.value());
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retx_manager.get_consecutive_retx(); }

void TCPSender::send_empty_segment() {
    const TCPSegment segment = _gen_segment(_next_seqno, false, false, string());
    _segments_out.push(segment);
}

TCPSegment TCPSender::_gen_segment(uint64_t absolute_seqno, bool syn, bool fin, string payload) {
    TCPSegment segment;

    TCPHeader &tcp_header = segment.header();
    Buffer &tcp_payload = segment.payload();

    tcp_header.seqno = wrap(absolute_seqno, _isn);
    tcp_header.syn = syn;
    tcp_header.fin = fin;

    tcp_payload = Buffer(move(payload));

    return segment;
}
