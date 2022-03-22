#include "stream_reassembler.hh"

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _unassm_size(0)
    , _eof(false)
    , _buffer(capacity, 0)
    , _buffer_check(capacity, false) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    const size_t unassm_base = _output.bytes_written();
    const size_t buffer_upperbound = _capacity + unassm_base - _output.buffer_size();

    const size_t data_begin = max(index, unassm_base);
    const size_t data_expect = max(index + data.length(), unassm_base);
    const size_t data_end = min(data_expect, buffer_upperbound);

    // push data into buffer
    _eof |= (data_expect == data_end) && eof;
    for (size_t i = data_begin; i < data_end; i++) {
        const size_t data_idx = i - index;
        const size_t buffer_idx = i - unassm_base;

        if (!_buffer_check[buffer_idx]) {
            _buffer[buffer_idx] = data[data_idx];
            _buffer_check[buffer_idx] = true;
            _unassm_size++;
        }
    }

    // move data from buffer to bytestream
    string concat_data;
    for (size_t i = 0; i < _capacity && _buffer_check.front(); i++) {
        concat_data += _buffer.front();
        _unassm_size--;

        _buffer.pop_front();
        _buffer.push_back(0);

        _buffer_check.pop_front();
        _buffer_check.push_back(false);
    }
    _output.write(concat_data);

    // check both eof and end of assembling
    if (_unassm_size == 0 && _eof)
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return _unassm_size; }

bool StreamReassembler::empty() const { return _unassm_size == 0; }