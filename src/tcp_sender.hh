#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ),
      cur_RTO( initial_RTO_ms ), next_seqno ( isn ), outstandings()
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

  // added reader
  Reader& reader() { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  

  // added variables
  // timer vars
  uint64_t cur_timepost = 0;  // current timepost
  uint64_t cur_RTO = 0;       // current rto interval
  uint64_t consecutive_count = 0;  // consecutive count

  // Sender vars
  uint64_t bytes_in_flight = 0;
  uint64_t window_size = 1;  // initial win size
  uint16_t allowed_space = 0; // allowed spots

  // absolute global seq num
  uint64_t abs_next_seqno = 0;  // absolute next seqno
  Wrap32 next_seqno;            // next seqno, or checkpoint
  std::queue<TCPSenderMessage> outstandings; // internal storage of outstanding eles

  // bools for state
  // bool for SYN and FIN
  bool input_start = false;
  bool input_finished = false;

  // record for FIN+1 receiver to prevent further push
  bool last_received = false;
  bool FIN_sent = false;
};
