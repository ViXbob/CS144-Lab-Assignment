#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity = 0) :
_capacity(capacity), _l(0), _r(0), _byte_stream(""), _end(false), _error(false) { 
    DUMMY_CODE(capacity);
}

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    size_t delta_len = std::min(_capacity - (_r - _l), data.size());
    _byte_stream += data.substr(0, delta_len);
    _r = _byte_stream.size();
    return delta_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    return _byte_stream.substr(_l, std::min(len, _r - _l));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len); 
    _l += std::min(len, _r - _l);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    size_t L = _l;
    _l += std::min(len, _r - _l);
    return _byte_stream.substr(L, _l - L);
}

void ByteStream::end_input() { _end = true; }

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return _r - _l; }

bool ByteStream::buffer_empty() const { return _r == _l; }

bool ByteStream::eof() const { return _end && (_r == _l); }

size_t ByteStream::bytes_written() const { return _r; }

size_t ByteStream::bytes_read() const { return _l; }

size_t ByteStream::remaining_capacity() const { return _capacity - (_r - _l); }
