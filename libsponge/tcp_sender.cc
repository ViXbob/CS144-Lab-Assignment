#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _rto(retx_timeout) {}

void TCPSender::pop_outstanding_segment() {
    while(!_outstanding_segment.empty()) {
        if(_outstanding_segment.begin() -> first + 
           _outstanding_segment.begin() -> second.length_in_sequence_space() <= _ackno)  {
            _bytes_in_flight -= _outstanding_segment.begin() -> second.length_in_sequence_space();
            _outstanding_segment.erase(_outstanding_segment.begin());
        } else break;
    }
}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    size_t window_size = _window_size;
    if(window_size == 0) window_size = 1;

    window_size = _ackno + window_size - _next_seqno;

    size_t MAX_PAYLOAD_SIZE = TCPConfig::MAX_PAYLOAD_SIZE;

    while(window_size > 0 && 
    (_stream.bytes_written() > _stream.bytes_read() 
    || (_fin == false && _stream.eof() == true)
    || (_syn == false && _ackno == 0) )) {
        size_t used = 0;
        
        Type1 tmp_oustanding_segment;
        tmp_oustanding_segment.first = _syn + _stream.bytes_read();

        TCPSegment seg;
        if(_syn == false && _ackno == 0) {
            _syn = true;
            seg.header().syn = true;
            used += 1;
        }

        seg.header().seqno = wrap(tmp_oustanding_segment.first, _isn);
        seg.payload() = Buffer(_stream.read(min(window_size - used, MAX_PAYLOAD_SIZE)));
        used += seg.payload().size();

        if(used + 1 <= window_size) {
            if(_fin == false && _stream.eof() == true) {
                seg.header().fin = true;
                _fin = true;
                used += 1;
            }
        }

        window_size -= used;

        tmp_oustanding_segment.second = seg;
        _segments_out.push(seg);
        _outstanding_segment.insert(tmp_oustanding_segment);
        _bytes_in_flight += seg.length_in_sequence_space();
        _next_seqno = tmp_oustanding_segment.first + seg.length_in_sequence_space();

        if(_timer.is_stopped()) _timer.set_new_timer(_rto);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    DUMMY_CODE(ackno, window_size); 
    uint64_t new_ackno = unwrap(ackno, _isn, _ackno);
    if(new_ackno > _ackno) {
        _rto = _initial_retransmission_timeout;
        _consecutive_retransmissions = 0;
        _ackno = new_ackno;
        pop_outstanding_segment();
        if(!_outstanding_segment.empty()) _timer.set_new_timer(_rto);
        else _timer.stop();
    }
    _window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick);
    _timer.time_expired(ms_since_last_tick);
    pop_outstanding_segment();
    if(_timer.is_expired() && !_outstanding_segment.empty()) {
        _segments_out.push(_outstanding_segment.begin() -> second);
        if(_window_size > 0) {
            _consecutive_retransmissions++;
            _rto <<= 1;
        }
        _timer.set_new_timer(_rto);
    } else if(_outstanding_segment.empty()) 
        _timer.stop();
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment tmp;
    tmp.header().seqno = _isn;
    _segments_out.push(tmp);
}
