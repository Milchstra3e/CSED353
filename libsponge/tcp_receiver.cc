#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &tcp_header = seg.header();

    if (tcp_header.syn && !_is_received_syn) {
        _is_received_syn = true;
        _isn = tcp_header.seqno;
    }

    if (!_is_received_syn)
        return;

    _is_received_fin |= tcp_header.fin;

    const WrappingInt32 relative_str_seqno(tcp_header.seqno + tcp_header.syn);
    const string &str = seg.payload().copy();
    const size_t absolute_str_seqno = unwrap(relative_str_seqno, _isn, _reassembler.get_unassm_base());

    _reassembler.push_substring(str, absolute_str_seqno, tcp_header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_is_received_syn)
        return std::nullopt;

    const size_t auxiliary_term = _is_received_syn + (_is_received_fin && _reassembler.empty());
    const size_t absolute_seqno = auxiliary_term + _reassembler.get_unassm_base();
    return wrap(absolute_seqno, _isn);
}

size_t TCPReceiver::window_size() const { return _reassembler.get_window_size(); }
