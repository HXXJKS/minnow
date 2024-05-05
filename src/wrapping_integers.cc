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

  uint64_t diff = checkpoint - raw_value_;

  // Calculate the number of wraps needed to reach the checkpoint
  uint64_t wraps = diff / (uint64_t(1) << 32);

  // Calculate the absolute sequence number closest to the checkpoint
  uint64_t absSeqNum = raw_value_ + (wraps * (uint64_t(1) << 32));

  // If the absolute sequence number is greater than the checkpoint, decrement it by 2^32
  if (absSeqNum > checkpoint) {
      absSeqNum -= (uint64_t(1) << 32);
  }

  return absSeqNum;
}
