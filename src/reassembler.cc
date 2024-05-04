#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  Writer& tmp_writer = output_.writer();
  uint64_t avai_cap = tmp_writer.available_capacity();
  // discard situation
  if (first_index >= cur_idx + avai_cap) {
    return;
  }

  // storage
  if (first_index > cur_idx) {
    // store all data
    if (first_index + data.size() <= cur_idx + avai_cap) {
      container_[first_index] = {first_index, data, is_last_substring};
      total_stored_bytes_ += data.size();
    } else // otherwise modify bool
    {
      data = data.substr(0, cur_idx + avai_cap - first_index);
      container_[first_index] = {first_index, data, false};
      total_stored_bytes_ += data.size();
    }

    return;
  } 

  // next bytes
  // remove pushed parts if so
  if (first_index < cur_idx) {
    if (first_index + data.size() <= cur_idx) {
      return;
    } else {
      data = data.substr(cur_idx - first_index);
      //first_index = cur_idx; // check for later map erase
    }
  }

  // push part
  string tmp_str;
  uint64_t i = 0;
  while (i < avai_cap && i < data.size()) {
    tmp_str += data[i];
    i++;
  }
  tmp_writer.push(tmp_str);
  cur_idx += tmp_str.size();

  // delete map until entry has unpushed part
  auto itr = container_.begin();
  while (itr != container_.end()) {
    if (itr->second.data.size() + itr->second.first_index <= cur_idx) {

      cout << endl << "cur idx " << cur_idx << endl; 
      cout << "first index " << first_index << endl;
      cout << "delete one" << endl;
      auto prt_itr = container_;
      while (prt_itr != container_.end()) {
        cout << "fst idx " << prt_itr->second.first_index << endl;
        cout << "data " << prt_itr->second.data << endl;
        prt_itr++;
      } 

      auto next = std::next(itr);
      total_stored_bytes_ -= itr->second.data.size();
      container_.erase(itr);
      itr = next;
    } else {
      auto next = std::next(itr);
      itr = next;
    }
  }

  // insert next map entry
  itr = container_.begin();
  if (itr != container_.end()) {
    insert(itr->second.first_index, itr->second.data, itr->second.is_last_substring);
  }

  if (is_last_substring) {
    tmp_writer.close();
    return;
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_stored_bytes_;
}
