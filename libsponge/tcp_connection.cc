#include "tcp_connection.hh"

#include <iostream>

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return active() ? _sender.stream_in().remaining_capacity() : 0;
}

size_t TCPConnection::bytes_in_flight() const { return active() ? _sender.bytes_in_flight() : 0; }

size_t TCPConnection::unassembled_bytes() const { return active() ? _receiver.unassembled_bytes() : 0; }

size_t TCPConnection::time_since_last_segment_received() const {
    return active() ? _time_since_last_segment_received : 0;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    const TCPHeader &seg_header = seg.header();

    if (seg_header.rst)
        _error_occured();

    if (!active())
        return;

    _time_since_last_segment_received = 0;

    const bool check_keep_alive_seg = _receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 &&
                                      seg.header().seqno == _receiver.ackno().value() - 1;
    if (check_keep_alive_seg)
        _sender.send_empty_segment();
    else {
        _receiver.segment_received(seg);

        if (seg_header.ack) {
            const WrappingInt32 ackno = seg_header.ackno;
            const uint16_t window_size = seg_header.win;
            _sender.ack_received(ackno, window_size);
        } else
            _sender.fill_window();

        if (seg.length_in_sequence_space()) {
            if (_sender.segments_out().empty())
                _sender.send_empty_segment();
        }

        if (_receiver.stream_out().eof() && !(_sender.stream_in().eof() && _sent_fin))
            _linger_after_streams_finish = false;
    }

    _send_segments();
}

bool TCPConnection::active() const {
    if (_error)
        return false;

    if (!_prerequisite_test())
        return true;

    if (_linger_after_streams_finish)
        return true;

    return false;
}

size_t TCPConnection::write(const string &data) {
    if (!active())
        return 0;

    const size_t written_bytes = _sender.stream_in().write(data);
    _sender.fill_window();
    _send_segments();

    return written_bytes;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!active())
        return;

    _sender.tick(ms_since_last_tick);

    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS)
        _error_occured();

    _send_segments();

    _time_since_last_segment_received += ms_since_last_tick;

    const size_t max_linger_timeout = 10 * _cfg.rt_timeout;
    if (_prerequisite_test() && _time_since_last_segment_received >= max_linger_timeout)
        _linger_after_streams_finish = false;
}

void TCPConnection::end_input_stream() {
    if (!active())
        return;

    _sender.stream_in().end_input();
    _sender.fill_window();
    _send_segments();
}

void TCPConnection::connect() {
    _sender.fill_window();
    _send_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::_send_segments() {
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
}

void TCPConnection::_error_occured() {
    _error = true;
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
}

bool TCPConnection::_prerequisite_test() const {
    if (!_receiver.stream_out().eof())
        return false;

    if (!(_sender.stream_in().eof() && _sent_fin))
        return false;

    if (_sender.bytes_in_flight())
        return false;

    return true;
}