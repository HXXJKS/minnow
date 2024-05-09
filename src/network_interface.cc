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
  auto ip_addr = next_hop.ipv4_addr();
  // if in address table
  if (ip_to_ethernet_table.find(ip_addr) != ip_to_ethernet_table.end()) {
    // create an Ethernet frame and send it
  } else {  // otherwise add to queue
    // send an ARP request to IP addr

    // push to queue
    arp_waiting_queue.push(std::make_pair({ip_addr, dgram}));

    // if is waiting front, send arp request and start timer
  }
  
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{ 
  auto frame_header = frame.header();
  // if frame is IPv4
  if (frame_header.type == TYPE_IPv4) {
    // parse payload
      // successful push datagram to received
  }
  else if (frame_header.type == TYPE_ARP) {// else ARP

    // parse payload

      // successful, remember 30 secs

      // if arp, send ARP reply
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  
}
