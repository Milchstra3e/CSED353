#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _capacity(capacity), _total_write(0), _total_read(0), _buffer(), _error(false), _end_of_write(false) {}

size_t ByteStream::write(const string &data) {
    const size_t remain_cap = remaining_capacity();
    const size_t write_len = min(data.length(), remain_cap);

    for (size_t i = 0; i < write_len; i++)
        _buffer.push_back(data[i]);
    _total_write += write_len;

    return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    const size_t buf_size = buffer_size();
    const size_t peek_len = min(len, buf_size);

    string peek_str;

    for (size_t i = 0; i < peek_len; i++)
        peek_str += _buffer[i];

    return peek_str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    const size_t buf_size = buffer_size();
    const size_t pop_len = min(len, buf_size);

    for (size_t i = 0; i < pop_len; i++)
        _buffer.pop_front();
    _total_read += pop_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string read_str = peek_output(len);
    pop_output(len);

    return read_str;
}

void ByteStream::end_input() { _end_of_write = true; }

bool ByteStream::input_ended() const { return _end_of_write; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _total_write; }

size_t ByteStream::bytes_read() const { return _total_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
