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
  Wrap32 wraped_check = wrap(checkpoint);
  uint32_t dis_check_this = this.raw_value_ - wraped_check.raw_value_;
  uint64_t converted_dis = static_cast<uint64_t>(dis_check_this);
  return (checkpoint + converted_dis);
}
