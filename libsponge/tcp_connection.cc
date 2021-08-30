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
        TCPSegment seg(std::move(_sender.segments_out().front()));
        set_segment(seg);
        _debugger.print_segment(*this, seg, "Segment sent!", _now_time <= 500);
        _segments_out.push(std::move(seg));
        _sender.segments_out().pop();
    }
}

void TCPConnection::unclear_shutdown() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _is_unclear_shutdown = true;
}

void TCPConnection::send_empty_segment() {
    _sender.send_empty_segment();
    collect_output();
}

/*
bool TCPConnection::is_clear_shutdown() const {
    bool prereq =  (_receiver.unassembled_bytes() == 0 && _receiver.stream_out().input_ended())
                && (_sender.bytes_in_flight() == 0 && _sender.stream_in().eof())
                && (_sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2);
    bool prereq = (_receiver.FIN_RECV() && _sender.FIN_ACKED());
    return prereq && 
            (!_linger_after_streams_finish ||  time_since_last_segment_received() >= 10 * _cfg.rt_timeout);
}
*/

void TCPConnection::test_end() {
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        unclear_shutdown();
        send_empty_segment();
    }
    if(!_sender.stream_in().eof() && _receiver.FIN_RECV())
        _linger_after_streams_finish = false;

    // TIME_WAIT() = _receiver.FIN_RECV() and _sender.FIN_ACKED()
    if(TIME_WAIT()) {
        _is_clear_shutdown |= 
            (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout);
    }
}

void TCPConnection::set_segment(TCPSegment &seg) {
    if(_is_unclear_shutdown) {
        seg.header().rst = true;
    } else if(!_receiver.LISTEN()) {
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        if(_receiver.window_size() >= numeric_limits<uint16_t> :: max()) 
            seg.header().win = numeric_limits<uint16_t> :: max();
        else 
            seg.header().win = _receiver.window_size();
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { 
    return _now_time - _time_when_last_segment_received; 
}

void TCPConnection::segment_receive_route(const TCPSegment &seg) {
    if(LISTEN()) {
        if(!seg.header().syn) return;
        _receiver.segment_received(seg);
        _sender.fill_window();
    } else if(seg.header().rst) {
        unclear_shutdown();
    } else {
        bool should_send_empty_segment = false;

        should_send_empty_segment |= (!_receiver.segment_received(seg));

        if(SYN_SENT() && should_send_empty_segment) return;

        if(seg.header().ack) 
            should_send_empty_segment |= (!_sender.ack_received(seg.header().ackno, seg.header().win));

        should_send_empty_segment |= (seg.length_in_sequence_space() > 0);
        
        if(should_send_empty_segment && _sender.segments_out().empty()) 
            _sender.send_empty_segment();
    }
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
    DUMMY_CODE(seg); 
    _debugger.print_segment(*this, seg, "Segment received!");
    _time_when_last_segment_received = _now_time;
    segment_receive_route(seg);
    collect_output();
    test_end();
}

bool TCPConnection::active() const { 
    return !(_is_unclear_shutdown || _is_clear_shutdown);
}

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    size_t result = _sender.stream_in().write(data);
    _sender.fill_window();
    collect_output();
    test_end();
    return result;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    _now_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    collect_output();
    test_end();
}

void TCPConnection::end_input_stream() { 
    _sender.stream_in().end_input(); 
    _sender.fill_window();
    collect_output();
    test_end();
}

void TCPConnection::connect() {
    if(LISTEN()) {
        _sender.fill_window();
        collect_output();
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            unclear_shutdown();
            send_empty_segment();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
