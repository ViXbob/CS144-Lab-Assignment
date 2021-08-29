#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    // LISTEN and first segment with SYN
    if(seg.header().syn && !ackno().has_value()) _ISN.emplace(seg.header().seqno);

    // State: SYN_RECV
    if(ackno().has_value() && !stream_out().input_ended()) {
        size_t index = unwrap(seg.header().seqno, isn().value(), _ackno()) - !seg.header().syn;
        _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(_ISN.has_value()) return wrap(_ackno(), _ISN.value());
    else return optional<WrappingInt32>();
}

size_t TCPReceiver::_ackno() const {
    return _reassembler.assembled_bytes() + _ISN.has_value() + _reassembler.empty();
}

size_t TCPReceiver::window_size() const { 
    // return _capacity - (_reassembler.assembled_bytes() - _reassembler.stream_out().bytes_read()); 
    return stream_out().remaining_capacity();
}
