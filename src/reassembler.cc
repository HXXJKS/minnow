#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{ 
  const Writer& reassembler_writer_ = writer();
  uint64_t cur_avai_cap_ = reassembler_writer_.available_capacity();
  // next bytes
  if (first_index == cur_idx_) {
    // if data fit into the available capacity
    if (data_size <= cur_avai_cap_) {
      // push to buffed zone
      reassembler_writer_.push(data);
      // update cur_idx
      cur_idx_ += data.size();

      if (is_last_substring) {
        reassembler_writer_.close();
        return;
      }

      auto itr = container_.begin();
      // delete map first entry if equal
      while (itr != container_.end() && itr->first < first_index) {

        total_stored_bytes_ -= itr->second.data.size();

        auto next = std::next(itr);

        // Erase the current element
        container_.erase(itr);

        // Move to the next element
        itr = next;
      }

      // insert next entry
      itr = container_.begin();
      if (itr != container_.end()){
        insert(itr->second.first_index, itr->second.data, itr->second.is_last_substring);
      }
    } 
    else // otherwise discard
    {
      return;
    }
  } else // other two cases
  {
    // duplicated substring
    if (first_index < cur_idx_) {
      return;
    } else // check whether it fit wihin avai_cap
    {
      // store it
      if (first_index + data.size() < cur_idx_ + cur_avai_cap_) {
        container_[first_index] = {first_index, data, is_last_substring};
        total_stored_bytes_ += data.size();
      } else // otherwise discard
      {
        return;
      }
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_stored_bytes_;
}
