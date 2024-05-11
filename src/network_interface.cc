#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  auto ip_addr = next_hop.ipv4_numeric();
  // if in address table
  if (ip_to_ethernet_table.find(ip_addr) != ip_to_ethernet_table.end())
  {
    // create an Ethernet frame and send it
    // header
    EthernetFrame send_frame;
    send_frame.header.dst = ip_to_ethernet_table[ip_addr].first;
    send_frame.header.src = ethernet_address_;
    send_frame.header.type = EthernetHeader::TYPE_IPv4;

    // payload
    send_frame.payload = serialize(dgram);
    transmit(send_frame);
  }
  else
  {
    // send ARP request if not waiting
    if (arp_waiting_queue.find(ip_addr) == arp_waiting_queue.end())
    {
      // arp message
      ARPMessage arp_request_msg;
      arp_request_msg.opcode = ARPMessage::OPCODE_REQUEST;
      arp_request_msg.sender_ethernet_address = ethernet_address_;
      arp_request_msg.target_ethernet_address = {};
      arp_request_msg.sender_ip_address = ip_address_.ipv4_numeric();
      arp_request_msg.target_ip_address = ip_addr;

      // assemble reply frame
      EthernetFrame request_frame;
      request_frame.header.dst = ETHERNET_BROADCAST;
      request_frame.header.type = EthernetHeader::TYPE_ARP;
      request_frame.header.src = ethernet_address_;
      request_frame.payload = serialize(arp_request_msg);

      // add to waiting
      arp_waiting_queue[ip_addr] = std::make_pair(dgram, 0);

      // send the reply
      transmit(request_frame);
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  auto frame_header = frame.header;

  // if missend to us, ignore
  if (frame_header.dst != ethernet_address_ && frame_header.dst != ETHERNET_BROADCAST)
  {
    return;
  }

  // if frame is IPv4
  if (frame_header.type == EthernetHeader::TYPE_IPv4)
  {
    // if successful parse payload
    InternetDatagram ipv4_dgram;
    if (parse(ipv4_dgram, frame.payload))
    {
      // successful push datagram to received
      datagrams_received_.push(ipv4_dgram);
    }
  }
  else if (frame_header.type == EthernetHeader::TYPE_ARP)
  { // else ARP

    ARPMessage arp_msg;
    // parse payload
    if (parse(arp_msg, frame.payload))
    {
      bool is_request = (arp_msg.opcode == ARPMessage::OPCODE_REQUEST) && (arp_msg.target_ip_address == ip_address_.ipv4_numeric());

      // incoming request, send a reply
      if (is_request)
      {
        // assemple payload arp msg
        ARPMessage arp_reply_msg;
        arp_reply_msg.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply_msg.sender_ethernet_address = ethernet_address_;
        arp_reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply_msg.target_ethernet_address = arp_msg.sender_ethernet_address;
        arp_reply_msg.target_ip_address = arp_msg.sender_ip_address;

        // assemble reply frame
        EthernetFrame reply_frame;
        reply_frame.header.dst = arp_msg.sender_ethernet_address;
        reply_frame.header.src = ethernet_address_;
        reply_frame.header.type = EthernetHeader::TYPE_ARP;
        reply_frame.payload = serialize(arp_reply_msg);

        // send the reply
        transmit(reply_frame);
      }

      //cout << "msg code: " << (arp_msg.opcode == ARPMessage::OPCODE_REPLY) << endl;

      bool is_reply = (arp_msg.opcode == ARPMessage::OPCODE_REPLY) && (arp_msg.target_ethernet_address == ethernet_address_);

      // update mapping table
      if (is_request || is_reply)
      {
        ip_to_ethernet_table[arp_msg.sender_ip_address] = std::make_pair(arp_msg.sender_ethernet_address, 0);
      }

      //cout << "reply bool " << is_reply << endl;

      // reset mapping entry
      if (is_reply)
      {
        // find in the waiting queue and transmit
        if (arp_waiting_queue.find(arp_msg.sender_ip_address) != arp_waiting_queue.end())
        {
          // call send_datagram instead
          Address tmp_ipv4 = tmp_ipv4.from_ipv4_numeric(arp_msg.sender_ip_address);
          InternetDatagram send_dgram = arp_waiting_queue[arp_msg.sender_ip_address].first;

          // delete from waiting
          arp_waiting_queue.erase(arp_waiting_queue.find(arp_msg.sender_ip_address));

          // send at the end
          send_datagram(send_dgram, tmp_ipv4);
        }
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // refresh mapper table first
  auto itr = ip_to_ethernet_table.begin();
  while (itr != ip_to_ethernet_table.end())
  {
    // after 30 secs expiration
    if (itr->second.second + ms_since_last_tick >= 30000)
    {
      itr = ip_to_ethernet_table.erase(itr);
    }
    else
    { // otherwise increase time and check next entry
      itr->second.second += ms_since_last_tick;
      itr++;
    }
  }

  // refresh ARP waiting queue
  auto arp_itr = arp_waiting_queue.begin();
  while (arp_itr != arp_waiting_queue.end())
  {
    // if 5 secs expire, resend the request
    if (arp_itr->second.second + ms_since_last_tick >= 5000)
    {
      arp_itr->second.second = 0;

      // arp message
      ARPMessage arp_request_msg;
      arp_request_msg.opcode = ARPMessage::OPCODE_REQUEST;
      arp_request_msg.sender_ethernet_address = ethernet_address_;
      arp_request_msg.sender_ip_address = ip_address_.ipv4_numeric();
      arp_request_msg.target_ip_address = arp_itr->first;

      // assemble reply frame
      EthernetFrame request_frame;
      request_frame.header.dst = ETHERNET_BROADCAST;
      request_frame.header.type = EthernetHeader::TYPE_ARP;
      request_frame.header.src = ethernet_address_;
      request_frame.payload = serialize(arp_request_msg);

      // send the reply
      transmit(request_frame);
    }
    else
    {
      // otherwise check next entry
      arp_itr->second.second += ms_since_last_tick;
      arp_itr++;
    }
  }
}
