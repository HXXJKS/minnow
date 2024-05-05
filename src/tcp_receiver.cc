#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{ 
  bool_fin_ = message.FIN;

  // set ISN
  if (message.SYN == true && !bool_syn_) {
    receiver_isn_ = message.seqno;
    receiver_isn_ = receiver_isn_ + 1;
    absolute_sqno++;
  } 
  // else already started
  else if (message.SYN == false && bool_syn_) {
    receiver_isn_ = receiver_isn_ + 1;

    reassembler_.insert(absolute_sqno, message.payload, bool_fin_);
  }

  bool_reset_ = message.RST;
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage send_msg;
  
  // ackno
  if (bool_syn_) {
    send_msg.ackno = receiver_isn_;
  }

  // windows_size
  send_msg.windows_size = reassembler_.output_.available_capacity();

  // RST
  if (bool_reset_) {
    send_msg.RST = bool_reset_;
  } 
  
}
