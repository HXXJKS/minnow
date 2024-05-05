#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{ 

  // helper printer
  cout << "syn " << message.SYN << endl;
  cout << "fin " << message.FIN << endl;
  cout << "res " << message.RST << endl;
  cout << "payload:" << message.payload << endl;
  cout << "pay size: " << message.payload.size() << endl;
  cout << "seqno " << message.seqno.get_raw() << endl;

  // if SYN
  if (!bool_syn_ && message.SYN) {
    bool_syn_ = true;
    zero_point_ = message.seqno;

    // transform the index of the insert substring
    uint64_t idx = message.seqno.unwrap(zero_point_, reassembler().get_ackno());

    cout << "cur idx " << reassembler().get_ackno() << endl;

    reassembler_.insert(idx, message.payload, message.FIN);

  } 
  // if already settled
  else if (bool_syn_) {

    cout << "never been here" << endl;
    /*cout << "syn " << message.SYN << endl;
    cout << "fin " << message.FIN << endl;
    cout << "res " << message.RST << endl;*/
    cout << "payload:" << message.payload << endl;
    cout << "pay size: " << message.payload.size() << endl;
    cout << "seqno " << message.seqno.get_raw() << endl;

    // transform the index of the insert substring
    uint64_t idx = message.seqno.unwrap(zero_point_, reassembler().get_ackno());

    cout << "idx " << idx << endl;

    reassembler_.insert(idx, message.payload, message.FIN);
  }

  bool_fin_ = message.FIN;
  bool_reset_ = message.RST;
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage send_msg;
  
  // ackno
  if (bool_syn_) {

    cout << "cur: " << reassembler().get_ackno() << endl;
    cout << "zero " << zero_point_.get_raw() << endl;

    uint64_t wrap_n = reassembler().get_ackno() + 1;
    if (bool_fin_) {
      wrap_n += 1;
    }

    send_msg.ackno = Wrap32::wrap(wrap_n, zero_point_);

  }

  //cout << "ackno: " << send_msg.ackno.get_raw() << endl;

  // windows_size
  uint64_t tmp_size = writer().available_capacity();
  if (tmp_size > static_cast<uint64_t>(UINT16_MAX)) {
    send_msg.window_size = UINT16_MAX;
  } else {
    send_msg.window_size = static_cast<uint16_t>(writer().available_capacity());
  }

  // RST
  send_msg.RST = bool_reset_;
  
  return send_msg;
}
