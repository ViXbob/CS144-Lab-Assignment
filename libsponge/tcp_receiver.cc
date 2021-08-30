#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    // LISTEN and first segment with SYN
    if(seg.header().syn) {
        // refuse other syn if syn is already received
        if(!LISTEN()) return false;
        _ISN.emplace(seg.header().seqno);
        if(seg.length_in_sequence_space() == 1) return true;
    }
    // still LISTEN
    if(LISTEN()) return false;
    // FIN
    if(seg.header().fin && FIN_RECV()) return false; 
    // State: SYN_RECV
    if(SYN_RECV()) {
        size_t abs_seqno = unwrap(seg.header().seqno, isn().value(), _ackno());
        size_t index = abs_seqno - !seg.header().syn;
        if(abs_seqno + seg.payload().size() + seg.header().syn < _ackno()) return false;
        if(abs_seqno >= _ackno() + window_size()) {
            /*
                bare acknowledgement when (seqno == ackno) && (window_size == 0)
                do not need to send a bare ack back
            */
            if(!(window_size() == 0 && seg.length_in_sequence_space() == 0)) 
                return false;
        }
        _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);
    }
    // if(FIN_RECV()) return false;
    return true;
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
