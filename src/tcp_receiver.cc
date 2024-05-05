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
    uint64_t idx = message.seqno.unwrap(zero_point_, reassembler().get_ackno()) - 1;

    reassembler_.insert(idx, message.payload, message.FIN);
  }

  if (!bool_fin_) {
    bool_fin_ = message.FIN;
  }


  // there might be other seterror bypass this portion
  if (message.RST == true) {
    bool_reset_ = true;
    reader().set_error();
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage send_msg;
  
  // ackno
  if (bool_syn_) {

    uint64_t wrap_n = reassembler().get_ackno() + 1;


    // fin and the whole string been inserted
    if (bool_fin_ && reassembler().reassem_eof()) {
      wrap_n += 1;
    }

    send_msg.ackno = Wrap32::wrap(wrap_n, zero_point_);


  }

  // windows_size
  uint64_t tmp_size = writer().available_capacity();
  if (tmp_size > static_cast<uint64_t>(UINT16_MAX)) {
    send_msg.window_size = UINT16_MAX;
  } else {
    send_msg.window_size = static_cast<uint16_t>(writer().available_capacity());
  }

  // RST
  if(writer().has_error()) {
    send_msg.RST = true;
  }
  
  return send_msg;
}
