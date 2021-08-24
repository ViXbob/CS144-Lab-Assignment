#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity = 0) :
_capacity(capacity), _byte_written(0), _byte_read(0), _byte_stream(0), _end(false), _error(false) { 
    DUMMY_CODE(capacity);
}

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    size_t delta_len = std::min(data.size(), _capacity - _byte_stream.size());
    _byte_stream.insert(_byte_stream.end(), data.begin(), data.begin() + delta_len);
    _byte_written += delta_len;
    return delta_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    return std::string(_byte_stream.begin(), _byte_stream.begin() + std::min(len, _byte_stream.size()));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len); 
    size_t delta_len = std::min(len, _byte_stream.size());
    _byte_stream.erase(_byte_stream.begin(), _byte_stream.begin() + delta_len);
    _byte_read += delta_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    size_t delta_len = std::min(len, _byte_stream.size());
    std::string rtn(_byte_stream.begin(), _byte_stream.begin() + delta_len);
    pop_output(len);
    return rtn;
}

void ByteStream::end_input() { _end = true; }

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return _byte_stream.size(); }

bool ByteStream::buffer_empty() const { return _byte_stream.empty(); }

bool ByteStream::eof() const { return _end && _byte_stream.empty(); }

size_t ByteStream::bytes_written() const { return _byte_written; }

size_t ByteStream::bytes_read() const { return _byte_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _byte_stream.size(); }
