#include "tcp_connection.hh"

#include <iostream>

#include <random>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::collect_output() {
    while(!_sender.segments_out().empty()) {
        if(_receiver.ackno().has_value()) {
            _sender.segments_out().front().header().ack = true;
            _sender.segments_out().front().header().ackno = _receiver.ackno().value();
            if(_receiver.window_size() >= numeric_limits<uint16_t> :: max()) 
                _sender.segments_out().front().header().win = numeric_limits<uint16_t> :: max();
            else 
                _sender.segments_out().front().header().win = _receiver.window_size();
        }
        if(!_sender.stream_in().eof() && _receiver.stream_out().input_ended())
            _linger_after_streams_finish = false;
        _segments_out.push(std::move(_sender.segments_out().front()));
        _sender.segments_out().pop();
    }
}

void TCPConnection::unclear_shutdown() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _is_unclear_shutdown = true;
}

void TCPConnection::send_rst_to_peer() {
    _sender.send_empty_segment();
    _sender.segments_out().front().header().rst = true;
    _segments_out.push(std::move(_sender.segments_out().front()));
    _sender.segments_out().pop();
}

bool TCPConnection::is_clear_shutdown() const {
    bool prereq =  (_receiver.unassembled_bytes() == 0 && _receiver.stream_out().input_ended())
                && (_sender.bytes_in_flight() == 0 && _sender.stream_in().eof())
                && (_sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2);
    return prereq && 
            (!_linger_after_streams_finish ||  time_since_last_segment_received() >= 10 * _cfg.rt_timeout);
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { 
    return _now_time - _time_when_last_segment_received; 
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
    DUMMY_CODE(seg); 
    _time_when_last_segment_received = _now_time;
    if(seg.header().rst) {
        if(!_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0) return;
        unclear_shutdown();
    } else {
        _receiver.segment_received(seg);
        if(seg.header().ack)
            _sender.ack_received(seg.header().ackno, seg.header().win);
        if(seg.length_in_sequence_space() > 0) {
            if(_sender.next_seqno_absolute() == 0)
                connect();
            else _sender.send_empty_segment();
        }
        collect_output();
    }
}

bool TCPConnection::active() const { 
    return !(_is_unclear_shutdown || is_clear_shutdown());
}

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    size_t result = _sender.stream_in().write(data);
    _sender.fill_window();
    collect_output();
    return result;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    _now_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    collect_output();
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        send_rst_to_peer();
        unclear_shutdown();
    }
}

void TCPConnection::end_input_stream() { 
    _sender.stream_in().end_input(); 
    _sender.fill_window();
    collect_output();
}

void TCPConnection::connect() {
    _sender.fill_window();
    collect_output();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            send_rst_to_peer();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
