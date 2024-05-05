#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // static cast conversion, automaticlaly wrap around
  uint32_t res_raw = static_cast<uint32_t>(n);
  return (zero_point + res_raw);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  
  /*Wrap32 wraped_check = wrap(checkpoint, zero_point);
  // right
  uint32_t right_dis = raw_value_ - wraped_check.raw_value_;

  // left
  uint32_t left_dis = wraped_check.raw_value_ - raw_value_;

  if (left_dis <= right_dis) {
    return (checkpoint - static_cast<uint64_t>(left_dis));
  }

  return (static_cast<uint64_t>(right_dis) + checkpoint);*/

  const uint64_t mod = static_cast<uint64_t>( 1 ) << 32;
  uint32_t dist = raw_value_ - wrap( checkpoint, zero_point ).raw_value_;
  if ( dist <= ( static_cast<uint32_t>( 1 ) << 31 ) || checkpoint + dist < mod ) {
    return dist + checkpoint;
  }
  return dist + checkpoint - mod;
}
