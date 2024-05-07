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
  cout << endl << "push start " << endl;

  TCPSenderMessage msg;

  cout << "allowed space " << allowed_space << endl;
  cout << "window size " << window_size << endl;

  // if receive SYN, start
  if (!input_start) {
    input_start = true;
    msg.SYN = true;
    msg.FIN = false;
    msg.RST = false;
    msg.seqno = next_seqno;

    cout << "add length SYN " << msg.sequence_length() << endl;

    // push immediately
    // wrap up and send
    msg.seqno = next_seqno;
    next_seqno = next_seqno + msg.sequence_length();
    abs_next_seqno += msg.sequence_length();
    bytes_in_flight += msg.sequence_length();
    if (input_start) {
      allowed_space -= msg.sequence_length();
    }
    outstandings.push(msg);
    transmit(msg);

    return;
  }

  // SYN nsent, not acknowledged
  if (!outstandings.empty() && outstandings.front().SYN) {
    return;
  }

  // input not finished and empty buffer
  if (!reader().is_finished() && !reader().bytes_buffered()) {
    return;
  }

  // if FIN sent
  if (FIN_sent) {
    return;
  }

  // non-zero window size
  if (window_size) {
    cout << "allowed space " << allowed_space << endl;
    // while to push allowed space
    while (allowed_space) 
    {
      size_t tmp_payload = std::min(TCPConfig::MAX_PAYLOAD_SIZE, reader().bytes_buffered());
      tmp_payload = std::min(tmp_payload, static_cast<size_t>(allowed_space));

      cout << "next sqno " << next_seqno.get_raw() << endl;
      

      // read tmp payload off the stream
      string tmp_str;
      for (size_t i=0; i<tmp_payload; i++) {
        tmp_str += reader().peek();
        reader().pop(1);
      }

      // FIN included in the payload
      if (reader().is_finished() && tmp_payload < allowed_space) {
        msg.FIN = true;
        FIN_sent = true;
      }

      // push once as paylods has been reached
      // wrap up and send
      cout << "add length while"  << msg.sequence_length() << endl;
      msg.payload = tmp_str;
      msg.seqno = next_seqno;
      next_seqno = next_seqno + msg.sequence_length();
      abs_next_seqno += msg.sequence_length();
      bytes_in_flight += msg.sequence_length();
      if (input_start) {
        allowed_space -= msg.sequence_length();
      }
      outstandings.push(msg);
      transmit(msg);

      // if empty input
      if (!reader().bytes_buffered()) {
        break;
      }
      
    }

    return;
  }
  else if (allowed_space == 0) // non-zero
  {
    // if finished, FIN
    if (reader().is_finished()) {
      msg.FIN = true;
      input_finished = true;
    } 
    else if (reader().bytes_buffered() > 0) // non-empty buffer
    {
      string non_zero_str;
      non_zero_str = reader().peek();
      reader().pop(1);
      msg.payload = non_zero_str;
    }
  }


  cout << "add length end"  << msg.sequence_length() << endl;
  // wrap up and send
  msg.seqno = next_seqno;
  next_seqno = next_seqno + msg.sequence_length();
  abs_next_seqno += msg.sequence_length();
  bytes_in_flight += msg.sequence_length();
  if (input_start) {
    allowed_space -= msg.sequence_length();
  }
  outstandings.push(msg);
  transmit(msg);


  cout << "push finish " << endl;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  // abs and seqno not updated after return
  msg.seqno = next_seqno;
  msg.SYN = false;
  msg.FIN = false;
  msg.RST = false;
  msg.payload = "";
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{   
  cout << endl << "receive start " << endl;

  // make sure it has ackno
  if (msg.ackno.has_value()) {

    cout << "has ackno" << endl;

    auto ackno_received = msg.ackno.value();  // Wrap32
    uint64_t abs_ack_received = ackno_received.unwrap(isn_, abs_next_seqno);

    // in-flight bytes
    if (!outstandings.empty()) {

      cout << "in flight bytes" << endl;

      // ackno smaller than first or larger than next
      if (abs_ack_received < outstandings.front().seqno.unwrap(isn_, abs_next_seqno) 
          || abs_ack_received > abs_next_seqno) 
      {
        cout << "wrong ackno" << endl;
        cout << "abs next seqno " << abs_next_seqno << endl;
        cout << "abs ack no " << abs_ack_received << endl;
        cout << "front no " << outstandings.front().seqno.unwrap(isn_, abs_next_seqno) << endl;

        return;
      }

      // pop off in-flight bytes until not acknowledged
      while (!outstandings.empty()) {
        cout << "try delete" << endl;
        auto tmp_front = outstandings.front();

        cout << endl;
        cout << "front abs" << tmp_front.seqno.unwrap(isn_, abs_next_seqno) << endl;
        cout << "tmp front len " << tmp_front.sequence_length() << endl;
        cout << "abs ack no " << abs_ack_received << endl;
        cout << endl;

        // front fully acknowledged
        if (tmp_front.seqno.unwrap(isn_, abs_next_seqno) + tmp_front.sequence_length() <= abs_ack_received) {

          cout << "pop one" << endl;

          // pop front
          bytes_in_flight -= tmp_front.sequence_length();
          outstandings.pop();

          // reset timer
          cur_timepost = 0;
          cur_RTO = initial_RTO_ms_;
          consecutive_count = 0;
        } 
        else // front not fully acknowledged
        {
          cout << "not acked" << endl;


          // update window size
          window_size = msg.window_size;
          allowed_space = msg.window_size;
          if (!outstandings.empty()) {
            allowed_space = static_cast<uint16_t>(
              abs_ack_received + window_size - outstandings.front().seqno.unwrap(isn_, abs_next_seqno) - bytes_in_flight
            );
          }

          return;
        }

      }
    } 
    else  // empty in-flight bytes
    {
      // ackno larger than next
      if (abs_ack_received > abs_next_seqno) 
      {
        return;
      }

    }


    // update window size
    window_size = msg.window_size;
    allowed_space = msg.window_size;
    if (!outstandings.empty()) {
      allowed_space = static_cast<uint16_t>(
        abs_ack_received + window_size - outstandings.front().seqno.unwrap(isn_, abs_next_seqno) - bytes_in_flight
      );
    }
  } // otherwise no ackno

  cout << "receive finish " << endl;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{ 

  // a RTO interval elapsed, transmit the earliest msg
  // if (ms_since_last_tick > cur_RTO) {
  if (ms_since_last_tick + cur_timepost >= cur_RTO) {

    transmit(outstandings.front());

    // if non-zero window size
    if (window_size != 0) {
      consecutive_count++;
      cur_RTO *= 2;
    }

    // reset the timer
    cur_timepost = 0;
  } 
  else // otherwise add timepost
  {
    cur_timepost += ms_since_last_tick;
  }

}
