#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  router_table.push_back(std::make_pair(route_prefix, router_entry{prefix_length, next_hop, interface_num}));
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // go through all interfaces
  for (const auto& tmp_interface : _interfaces) {
    std::queue<InternetDatagram>& tmp_dgrams = tmp_interface->datagrams_received();
    // go through all datagrams
    while (!tmp_dgrams.empty()) {
      InternetDatagram& tmp_dgram = tmp_dgrams.front();
      // search in route table
      uint8_t tmp_longest_pre_len = 0;
      int tmp_idx = -1;
      for (size_t i=0; i<router_table.size(); i++) {
        // if find prefix
        if (router_table[i].first == tmp_dgram.header.dst) {
          if (router_table[i].second.pre_len > tmp_longest_pre_len ) {
            tmp_idx = i;
          }
        }
      }

      // match a longest
      if (tmp_idx != -1) {
        // TTL allowed 
        if (tmp_dgram.header.ttl > 1) {
          tmp_dgram.header.ttl--;
          // send the datagram
          if (router_table[tmp_idx].second.next_hop.has_value()) {
            interface(router_table[tmp_idx].second.inter_num)->send_datagram(tmp_dgram, router_table[tmp_idx].second.next_hop.value());
          } else {
            interface(router_table[tmp_idx].second.inter_num)->send_datagram(tmp_dgram, Address::from_ipv4_numeric(tmp_dgram.header.dst));
          }
          
        }
      }

      // pop off
      tmp_dgrams.pop();

    }
  }
}
