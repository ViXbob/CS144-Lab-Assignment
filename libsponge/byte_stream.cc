#include "byte_stream.hh"


// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity = 0) : _capacity(capacity) { 
    DUMMY_CODE(capacity);
}

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    return write(string_view(data));
}

size_t ByteStream::write(const string_view &str) {
    DUMMY_CODE(str);
    size_t delta_len = min(str.size(), _capacity - buffer_size());
    _buffer.push_back(move(string().assign(str.begin(), str.begin() + delta_len)));
    _byte_written += delta_len;
    return delta_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    size_t true_len = min(len, buffer_size());
    string ret;
    ret.reserve(true_len);
    for(const auto &buffer : _buffer) {
        if(true_len >= buffer.size()) {
            ret.append(buffer);
            true_len -= buffer.size();
            if(true_len <= 0) break;
        } else {
            string s = buffer.copy();
            ret.append(string().assign(s.begin(), s.begin() + true_len));
            break;
        }
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len); 
    size_t delta_len = min(len, buffer_size());
    if(delta_len <= 0) return;
    _byte_read += delta_len;
    while(delta_len > 0)  {
        if(delta_len >= _buffer.front().size()) {
            delta_len -= _buffer.front().size();
            _buffer.pop_front();
        } else {
            _buffer.front().remove_prefix(delta_len);
            break;
        }
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    size_t delta_len = min(len, buffer_size());
    string ret = peek_output(delta_len);
    pop_output(delta_len);
    return ret;
}

void ByteStream::end_input() { _end = true; }

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return _byte_written - _byte_read; }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return _end && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _byte_written; }

size_t ByteStream::bytes_read() const { return _byte_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
