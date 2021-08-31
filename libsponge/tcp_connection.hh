#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"
#include <iostream>

//! \brief A complete endpoint of a TCP connection
class TCPConnection {
  private:
    // friend class TCPConnectionDebugger;
    TCPConfig _cfg;
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};
    bool _is_unclear_shutdown{false};
    bool _is_clear_shutdown{false};
    size_t _now_time{0};
    size_t _time_when_last_segment_received{0};

    //! outbound queue of segments that the TCPConnection wants sent
    std::queue<TCPSegment> _segments_out{};

    //! Should the TCPConnection stay active (and keep ACKing)
    //! for 10 * _cfg.rt_timeout milliseconds after both streams have ended,
    //! in case the remote TCPConnection doesn't know we've received its whole stream?
    bool _linger_after_streams_finish{true};

    void collect_output();

    void unclear_shutdown();

    // bool is_clear_shutdown() const;

    void send_empty_segment();

    void test_end();

    void set_segment(TCPSegment &seg);

    void segment_receive_route(const TCPSegment &seg);

    class TCPConnectionDebugger {
        private:
            bool open_debugger{true};
            static const std::string left_1;
            static const std::string left_2;
            static const std::string right_;

        public:
            TCPConnectionDebugger(bool open = true) : open_debugger(open) {}
            ~TCPConnectionDebugger() {}

            std::string color_1(const std::string &data) {
                return left_1 + data + right_;
            }

            std::string color_2(const std::string &data) {
                return left_2 + data + right_;
            }

            void print_segment(const TCPConnection &that, const TCPSegment &seg, const std::string &desription, bool check = true) {
                if(!check || !open_debugger) return;
                std::cerr << "\n" << color_1(desription) << "\n";
                std::cerr << (color_2("Flag") + " : ") 
                        << (seg.header().syn ? "S" : "")
                        << (seg.header().fin ? "F" : "")
                        << (seg.header().ack ? "A" : "")
                        << (seg.header().rst ? "R" : "") << "\n"
                        << (color_2("Sequnce Number") + " : ")
                        << (seg.header().seqno.raw_value()) << "\n"
                        << (color_2("Acknowledgement Number") + " : ")
                        << (seg.header().ackno) << "\n"
                        << (color_2("Window Size") + " : ")
                        << (seg.header().win) << "\n"
                        << (color_2("Payload") + " : ")
                        << (seg.payload().size() ? seg.payload().str() : "empty string") << "\n"
                        << (color_2("Payload Size") + " : ")
                        << (seg.payload().size()) << "\n"
                        << (color_2("Sequnce Space") + " : ")
                        << (seg.length_in_sequence_space()) << "\n"
                        << (color_2("ackno of sender") + " : ")
                        << (that._sender.ackno_absolute()) << "\n"
                        << (color_2("next seqno absolute of sender") + " : ")
                        << (that._sender.next_seqno_absolute()) << "\n";
                if(that._sender.FIN_ACKED())
                    std::cerr << "FIN_ACKED" << std::endl;
                if(that._receiver.FIN_RECV())
                    std::cerr << "FIN_RECV" << std::endl;
            }
    };
  public:
    TCPConnectionDebugger _debugger{false};
    //! \name "Input" interface for the writer
    //!@{

    //! \brief Initiate a connection by sending a SYN segment
    void connect();

    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    //! \returns the number of bytes from `data` that were actually written.
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! \brief The inbound byte stream received from the peer
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //!@{

    //! Called when a new segment has been received from the network
    void segment_received(const TCPSegment &seg);

    //! Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    //! moving is allowed; copying is disallowed; default construction not possible

    //!@{
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}

    // friend void TCPConnectionDebugger::print_segment(const TCPConnection &that, const TCPSegment &seg, const std::string &desription, bool check = true);

    // State of TCP FSM
    bool LISTEN() const { return _receiver.LISTEN() && _sender.CLOSED(); }
    bool SYN_RCVD() const { return _receiver.SYN_RECV() && _sender.SYN_SENT(); }
    bool SYN_SENT() const { return _receiver.LISTEN() && _sender.SYN_SENT(); }
    bool ESTABLISHED() const { return _receiver.SYN_RECV() && _sender.SYN_ACKED(); }
    bool CLOSE_WAIT() const { return _receiver.FIN_RECV() && _sender.SYN_ACKED() && !_linger_after_streams_finish; }
    bool LAST_ACK() const { return _receiver.FIN_RECV() && _sender.FIN_SENT() && !_linger_after_streams_finish; }
    bool CLOSING() const { return _receiver.FIN_RECV() && _sender.FIN_SENT(); }
    bool FIN_WAIT_1() const { return _receiver.SYN_RECV() && _sender.FIN_SENT(); }
    bool FIN_WAIT_2() const { return _receiver.SYN_RECV() && _sender.FIN_ACKED(); }
    bool TIME_WAIT() const { return _receiver.FIN_RECV() && _sender.FIN_ACKED(); }
    // bool RESET()
    // bool CLOSED()
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
