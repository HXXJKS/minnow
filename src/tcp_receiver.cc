#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{ 

  // if SYN
  if (!bool_syn_ && message.SYN) {
    bool_syn_ = true;
    zero_point_ = message.seqno;

    // transform the index of the insert substring
    uint64_t idx = message.seqno.unwrap(zero_point_, reassembler().get_ackno());

    reassembler_.insert(idx, message.payload, message.FIN);
  } 
  // if already settled
  else if (bool_syn_) {
    // transform the index of the insert substring
    uint64_t idx = message.seqno.unwrap(zero_point_, reassembler().get_ackno());
    reassembler_.insert(idx, message.payload, message.FIN);
  }
  bool_reset_ = message.RST;
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage send_msg;
  
  // ackno
  if (bool_syn_) {
    send_msg.ackno = Wrap32::wrap(reassembler().get_ackno() + 1, zero_point_);
  }

  // windows_size
  send_msg.window_size = writer().available_capacity();

  // RST
  send_msg.RST = bool_reset_;
  
  return send_msg;
}
