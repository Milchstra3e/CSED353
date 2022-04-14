#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return active() ? _sender.stream_in().remaining_capacity() : 0;
}

size_t TCPConnection::bytes_in_flight() const { return active() ? _sender.bytes_in_flight() : 0; }

size_t TCPConnection::unassembled_bytes() const { return active() ? _receiver.unassembled_bytes() : 0; }

size_t TCPConnection::time_since_last_segment_received() const {
    return active() ? _time_since_last_segment_received : 0;
}

void TCPConnection::segment_received(const TCPSegment &seg) { DUMMY_CODE(seg); }

bool TCPConnection::active() const { return {}; }

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    return {};
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

void TCPConnection::end_input_stream() {}

void TCPConnection::connect() {}

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