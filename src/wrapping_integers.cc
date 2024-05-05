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
  // Your code here.
  (void)zero_point;
  (void)checkpoint;
  return {};
}
