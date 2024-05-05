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
  uint64_t u32_max = static_cast<uint64_t>(1 << 32);
  uint32_t dist = raw_value_ - wrap( checkpoint, zero_point ).raw_value_;

  // closer to right or before first wrap
  if (dist < static_cast<uint64_t>(1 << 31) ||
      dist + checkpoint < u32_max) {
    return checkpoint + dist;
  }

  // otherwise return the left one
  return checkpoint + dist - u32_max;

}
