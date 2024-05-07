#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstandings_seq_num;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_count;
}

void TCPSender::push( const TransmitFunction& transmit )
{ 

  cout << endl << "push started" << endl;

  TCPSenderMessage msg;
  // SYN processing
  if (!input_start) {
    input_start = true;
    msg.SYN = true;
  }

  // reset processing
  if (reader().has_error()) {
    msg.RST = true;
  } 

  // format string to segment
  string tmp_payload;

  uint64_t i = 0;
  uint32_t popped_bytes = 0;

  uint64_t payload_counter = 0;

  // if start, 
  if (msg.SYN == true) {
    i++;
    popped_bytes++;
    outstandings_seq_num++;

    payload_counter++;
  }

  cout << "window size " << allowed_win_size << endl;

  uint64_t max_payload = static_cast<uint64_t>(std::min(static_cast<int>(allowed_win_size), 1452));

  cout << "payload size: " << max_payload << endl;

  while (i<max_payload) {
    if (!reader().is_finished()) {
      // input has buffed bytes
      if (reader().bytes_buffered() > 0) {
        // make sure the outstanding bytes fill in the window
        if (outstandings_seq_num < allowed_win_size){
          tmp_payload += reader().peek();
          reader().pop(1);
          popped_bytes++;
          outstandings_seq_num++;

          // handle segments
          payload_counter++;
          if (payload_counter == max_payload) {

            msg.seqno = cur_isn;
            msg.payload = tmp_payload;

            // push to outstandings
            if ((msg.sequence_length() > 0) && (last_received == false) && !FIN_sent) {
              cout << "input finished " << input_finished << endl;
              outstandings.push(msg);
              transmit(msg);
            }
            
            // reset
            payload_counter = 0;
            tmp_payload = "";
            cur_isn = cur_isn + popped_bytes;
            popped_bytes = 0;

          }
        }
      }
    } 
    else // reader finished
    {
      cout <<  "out num " << outstandings_seq_num << endl;
      cout << "popped bytes " << popped_bytes  << endl;
      cout << "payload size " << tmp_payload << endl;
      cout << "allowed win size " << allowed_win_size << endl;

      // i++;
      // make sure the FIN fill in the window
      if (outstandings_seq_num < allowed_win_size) {
        msg.FIN = true;

        // first push FIN
        if (!input_finished) {
          input_finished = true;
          popped_bytes++;
          outstandings_seq_num++;
        }

        //popped_bytes++;
        // outstandings_seq_num++;
      }

      break;
    }
    i++;
  }
  msg.seqno = cur_isn;
  msg.payload = tmp_payload;

  cout << "payload:" << tmp_payload << endl;
  cout << "payloadsize: " << tmp_payload.size() << endl;
  cout << "syn " << msg.SYN << endl;
  cout << "fin " << msg.FIN << endl;

  // push outstanding only if wefoij
  // if ((tmp_payload.size() > 0 || msg.SYN || msg.FIN) && (!last_received)) {
  if ((msg.sequence_length() > 0) && (last_received == false) && !FIN_sent) {

    cout << "input finished " << input_finished << endl;
    outstandings.push(msg);
    transmit(msg);
  }

  if (input_finished) {
    FIN_sent = true;
  }
  
  cout << "cur isn " << cur_isn.get_raw() << endl;

  
  cur_isn = cur_isn + popped_bytes;

  cout << "cur isn " << cur_isn.get_raw() << endl;

  cout << "push finished" << endl;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = cur_isn;
  msg.SYN = false;
  msg.FIN = false;
  msg.RST = false;
  msg.payload = "";

  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{ 
  cout << endl << "receive start" << endl;

  cout << "received win size: " << msg.window_size << endl;

  allowed_win_size = msg.window_size;

  // new acknowledged segments
  if (msg.ackno.has_value() && !outstandings.empty()) {

    if (!(msg.ackno.value().get_raw() <= outstandings.front().seqno.get_raw())) {

      // reset timing
      cur_RTO = initial_RTO_ms_;
      cur_timepost = 0;
      consecutive_count = 0;

      // pop off fully acknowledged outstandings
      if (msg.ackno.has_value()) {
        Wrap32 msg_wrap = msg.ackno.value();
        Wrap32 tmp_wrap = outstandings.front().seqno + static_cast<uint32_t>(outstandings.front().sequence_length());

        cout << "not empty" << !outstandings.empty() << endl;
        cout << "first raw " << tmp_wrap.get_raw() << endl;
        cout << "msg raw " << msg_wrap.get_raw() << endl;
        cout << "fully acknow: " << (tmp_wrap.get_raw() < msg_wrap.get_raw()) << endl;

        while (!outstandings.empty() && 
        tmp_wrap.get_raw() <= msg_wrap.get_raw() )
        { 

          if (tmp_wrap.get_raw() == msg_wrap.get_raw() && !last_received && input_finished) {
            last_received = true;
          }

          // ackno beyond
          if (outstandings.size() == 1 && tmp_wrap.get_raw() < msg_wrap.get_raw()) {
            cout << "thru here" << endl;
            break;
          }

          cout << "pop once" << endl;
          if (outstandings.front().SYN || outstandings.front().FIN) {
            outstandings_seq_num--;
          }
          outstandings_seq_num -= outstandings.front().payload.size();
          outstandings.pop();

          if (!outstandings.empty()) {
            tmp_wrap = outstandings.front().seqno + static_cast<uint32_t>(outstandings.front().sequence_length());
          }
        }
      }
    }
  }

  cout << "cur isn " << cur_isn.get_raw() << endl;

  if (msg.ackno.has_value() && (cur_isn.get_raw() < msg.ackno.value().get_raw())) {
    /*if (cur_isn.get_raw() + 1 != ) {

    }*/

    cout << endl << "pluis 1 been here\n" << endl;

    cur_isn = cur_isn + 1;
  }

  cout << "cur isn " << cur_isn.get_raw() << endl;
  
  cout << "fin received" << last_received << endl;

  cout << "receive finished" << endl;

}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{ 
  //cout << "stuck here 3" << endl;
  sender_timeline += ms_since_last_tick;

  // a RTO interval elapsed, transmit the earliest msg
  // if (ms_since_last_tick > cur_RTO) {
  if (ms_since_last_tick + cur_timepost >= cur_RTO) {

    transmit(outstandings.front());

    // if non-zero window size
    if (allowed_win_size != 0) {
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
