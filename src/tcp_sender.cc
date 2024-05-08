#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return bytes_in_flight;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_count;
}

void TCPSender::push( const TransmitFunction& transmit )
{ 
  // has error and set RST
  if (reader().has_error()) {
    TCPSenderMessage msg = make_empty_message();
    transmit(msg);
    return;
  }

  // cout << endl << "push start " << endl;

  uint64_t cur_window_size = window_size ? window_size : 1;

  // cout << "cur win size " << cur_window_size << endl;
  // cout << "bytes in flight " << bytes_in_flight << endl;

  // make sure window size is not exceeded
  while (cur_window_size > bytes_in_flight) {
    TCPSenderMessage msg;

    // start set
    if (!input_start) {
      msg.SYN = true;
      input_start = true;
    }

    msg.seqno = isn_ + abs_next_seqno;

    uint16_t payload_size = std::min(TCPConfig::MAX_PAYLOAD_SIZE, cur_window_size - bytes_in_flight - msg.SYN);
    payload_size = std::min(payload_size, static_cast<uint16_t>(reader().bytes_buffered()));
    
    // cout << "payload size " << payload_size << endl;
    // cout << "buffer size " << reader().bytes_buffered() << endl;

    // take payload size from input stream
    string payload;
    for (uint16_t i =0; i < payload_size; i++) {
      payload += reader().peek();
      reader().pop(1);
    }

    // cout << "output size" << payload.size() << endl;

    // include FIN, otherwise leave FIN to next push
    if (!input_finished && reader().is_finished() 
        && payload.size() + bytes_in_flight + msg.SYN < cur_window_size) {
        msg.FIN = true;
        input_finished = true;
    }

    msg.payload = payload;

    // zero length msg
    if (msg.sequence_length() == 0) {
      // cout << "no data" << endl;
      break;
    }

    // no outstanding segments, simply restart timer
    if (outstandings.empty()) {
      cur_RTO = initial_RTO_ms_;
      cur_timepost = 0;
    }

    // push and set sender vars
    transmit(msg);
    bytes_in_flight += msg.sequence_length();
    outstandings.push(msg);
    abs_next_seqno += msg.sequence_length();

    // if FIN, quit 
    if (msg.FIN) {
      // cout << "fin" << endl;
      break;
    }
  }
  
  // cout << "push finish " << endl;
}

TCPSenderMessage TCPSender::make_empty_message() const
{ 
  // cout << "been called here!" << endl;
  TCPSenderMessage msg;
  msg.seqno = isn_ + abs_next_seqno;
  // has error and set RST
  if (reader().has_error()) {
    // cout << "rst set" << endl;
    msg.RST = true;
  }
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{   
  // cout << endl << "receive start " << endl;

  // ackno exists
  if (msg.ackno.has_value()) {
    auto abs_ack_received = msg.ackno.value().unwrap(isn_, abs_next_seqno);

    // larger ackno received
    if (abs_ack_received > abs_next_seqno) {
      return;
    }

    // pop off fully acknowledged segments
    while (!outstandings.empty()) {
      auto tmp_front = outstandings.front();
      if (tmp_front.seqno.unwrap(isn_, abs_next_seqno) + tmp_front.sequence_length() <= abs_ack_received) {
        bytes_in_flight -= tmp_front.sequence_length();
        outstandings.pop();

        // reset timer
        cur_RTO = initial_RTO_ms_;
        if (!outstandings.empty()) {
          cur_timepost = 0;
        }
        
      } else {
        break;  // not fully acknowledged
      }
    }

    consecutive_count = 0;
  }
  window_size = msg.window_size;

  // set error if needed
  if (msg.RST) {
    // cout << "set an error" << endl;
    writer().set_error();
  } 
  // cout << "receive finish " << endl;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{ 
  // cout << "tick start" << endl;
  cur_timepost += ms_since_last_tick;
  if (cur_timepost >= cur_RTO && !outstandings.empty()) {
    // cout << "retransmit" << endl;
    if (window_size > 0) {
      cur_RTO *= 2;
    }

    cur_timepost = 0;
    consecutive_count++;
    transmit(outstandings.front());
  }
  // cout << "tick finish" << endl;
}
