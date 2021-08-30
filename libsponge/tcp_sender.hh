#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>
#include <set>
#include <utility>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  using Type1 = std::pair<uint64_t, TCPSegment>;
  private:
    class TCPSegmentComparator {
        public:
            TCPSegmentComparator() {}
            ~TCPSegmentComparator() {}
            bool operator ()(const Type1 &lhs, const Type1 &rhs) const {
                return lhs.first < rhs.first;
            }
    };

    class RetransmissionTimer {
        public:
            RetransmissionTimer(uint32_t initial_time, bool set_expired, bool set_stopped) 
                : _remaining_time(initial_time)
                , _is_expired(set_expired) 
                , _is_stopped(set_stopped) {}

            void time_expired(uint32_t time_to_expire) {
                if(_is_stopped) return;
                if(time_to_expire >= _remaining_time) _is_expired = true;
                else _remaining_time -= time_to_expire;
            }

            bool is_expired() { return _is_expired && !_is_stopped; }

            bool is_stopped() { return _is_stopped; }

            void set_new_timer(uint32_t initial_time) {
                _remaining_time = initial_time;
                _is_expired = false;
                _is_stopped = false;
            }

            void stop() {
                _is_stopped = true;
                _is_expired = false;
                _remaining_time = 0;
            }
        private:
            uint32_t _remaining_time;
            bool _is_expired{0};
            bool _is_stopped{0};
    };

    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    uint64_t _bytes_in_flight{0};

    uint32_t _consecutive_retransmissions{0};

    uint64_t _ackno{0};

    uint32_t _rto;

    std::set<Type1, TCPSegmentComparator> _outstanding_segment{};

    RetransmissionTimer _timer{0, false, true};

    uint16_t _window_size{1};

    bool _syn{false};

    bool _fin{false};
  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    bool ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}

    void pop_outstanding_segment();

    WrappingInt32 isn() const { return _isn; }

    uint64_t ackno_absolute() const { return _ackno; }

    std::set<Type1, TCPSegmentComparator> &outstanding_segment() { return _outstanding_segment; }

    const std::set<Type1, TCPSegmentComparator> &outstanding_segment() const { return _outstanding_segment; }

    uint16_t window_size() const { return _window_size; }

    bool SYN() const { return _syn; }

    bool FIN() const { return _fin; }

    // State of sender
    bool CLOSED() const { return (next_seqno_absolute() == 0); }
    bool SYN_SENT() const { return (next_seqno_absolute() > 0 && next_seqno_absolute() == bytes_in_flight()); }
    bool SYN_ACKED() const { return (next_seqno_absolute() > bytes_in_flight() && !stream_in().eof()); }
    bool SYN_ACKED_FIN_TO_SEND() const { 
        return (next_seqno_absolute() < stream_in().bytes_written() + 2 && stream_in().eof()); 
    }
    bool FIN_SENT() const { 
        return (stream_in().eof() 
            && next_seqno_absolute() == stream_in().bytes_written() + 2
            && bytes_in_flight() > 0); 
    }
    bool FIN_ACKED() const {
        return (stream_in().eof() 
            && next_seqno_absolute() == stream_in().bytes_written() + 2
            && bytes_in_flight() == 0); 
    }
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
