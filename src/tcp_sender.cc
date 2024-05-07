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

  //cout << endl << "push started" << endl;

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
  // if start, 
  if (msg.SYN == true) {
    i++;
    popped_bytes++;
    outstandings_seq_num++;
  }


  /*cout << "window size: " << allowed_win_size << endl;
  cout << "reader state: " << reader().has_error() << endl;
  cout << "bytes buffed: " << reader().bytes_buffered() << endl;*/

  while (i<allowed_win_size) {
    if (!reader().is_finished()) {
      if (reader().bytes_buffered() > 0) {
        if (outstandings_seq_num < allowed_win_size){
          tmp_payload += reader().peek();
          reader().pop(1);
          popped_bytes++;
          outstandings_seq_num++;
        }
      }
    } else {

      cout << endl << "im here" << endl << endl;

      cout <<  "out num " << outstandings_seq_num << endl;
      cout << "popped bytes " << popped_bytes  << endl;
      cout << "payload size " << tmp_payload << endl;
      cout << "allowed win size " << allowed_win_size << endl;

      // i++;
      // make sure the FIN fill in the window
      if (outstandings_seq_num < allowed_win_size) {
        msg.FIN = true;
        popped_bytes++;
        outstandings_seq_num++;
      }

      break;
    }
    i++;
  }
  msg.seqno = cur_isn;
  msg.payload = tmp_payload;

  /*cout << "payload:" << tmp_payload << endl;
  cout << "payloadsize: " << tmp_payload.size() << endl;
  cout << "syn " << msg.SYN << endl;
  cout << "fin " << msg.FIN << endl;*/

  cout << "payload:" << tmp_payload << endl;
  cout << "payloadsize: " << tmp_payload.size() << endl;
  cout << "syn " << msg.SYN << endl;
  cout << "fin " << msg.FIN << endl;

  // push outstanding only if wefoij
  if (tmp_payload.size() > 0 || msg.SYN || msg.FIN) {
    /*if (tmp_payload.size() > 0 || msg.SYN) { 
      outstandings.push(msg);
      //outstandings_seq_num += tmp_payload.size();

      //allowed_win_size -= tmp_payload.size();
    }*/

    outstandings.push(msg);
    transmit(msg);
  }
  
  
  cur_isn = cur_isn + popped_bytes;

  //cout << "stuck here" << endl;
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
  // helper printer
  /*cout << endl << "stuck here 2" << endl;
  cout << "receiver ackno: ";
  if (msg.ackno.has_value()) {
    cout << msg.ackno.value().get_raw() << endl;
  } else {
    cout << "no achno" << endl;
  }
  cout << "winsize: " << msg.window_size << endl;
  cout << "rst: " << msg.RST << endl;
  cout << "curisn: " << cur_isn.get_raw() << endl;

  // helper printer outstandings
  cout << "first out size: " << outstandings.front().payload.size() << endl;
  cout << "first out raw: " << outstandings.front().seqno.get_raw() << endl;*/


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
        Wrap32 tmp_wrap = outstandings.front().seqno + static_cast<uint32_t>(outstandings.front().payload.size());

        /*cout << "not empty" << !outstandings.empty() << endl;
        cout << "first raw " << tmp_wrap.get_raw() << endl;
        cout << "msg raw " << msg_wrap.get_raw() << endl;
        cout << "fully acknow: " << (tmp_wrap.get_raw() < msg_wrap.get_raw()) << endl << endl;*/

        while (!outstandings.empty() && 
        tmp_wrap.get_raw() <= msg_wrap.get_raw() )
        { 
          //cout << "pop once" << endl;
          if (outstandings.front().SYN) {
            outstandings_seq_num--;
          }
          outstandings_seq_num -= outstandings.front().payload.size();
          //allowed_win_size += outstandings.front().payload.size();
          outstandings.pop();

          if (!outstandings.empty()) {
            tmp_wrap = outstandings.front().seqno + static_cast<uint32_t>(outstandings.front().payload.size());
          }
        }
      }
    }
  }

  if (cur_isn.get_raw() < msg.ackno.value().get_raw()) {
    cur_isn = msg.ackno.value();
  }
  //cout << "stuck here 2 finished" << endl;

}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{ 
  //cout << "stuck here 3" << endl;

  sender_timeline += ms_since_last_tick;

  cout << "before timepost " << cur_timepost << endl;
  cout << "ms since last: " << ms_since_last_tick << endl;
  cout << "cur rto: " << cur_RTO << endl;

  // a RTO interval elapsed, transmit the earliest msg
  // if (ms_since_last_tick > cur_RTO) {
  if (ms_since_last_tick + cur_timepost >= cur_RTO) {
    cout << "not empty" << !outstandings.empty() << endl;

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

  cout << endl << "after timepost " << cur_timepost << endl;
  cout << "ms since last: " << ms_since_last_tick << endl;
  cout << "cur rto: " << cur_RTO << endl << endl;
}
