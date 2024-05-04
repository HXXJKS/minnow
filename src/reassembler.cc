#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{ 
  cout << "start here first " << first_index << " data " << data << endl;


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
      for (uint64_t i=0; i<data.size(); i++) {
        if (container_.find(data[i]) == container_.end()) {
          container_[first_index + i].st_char = data[i];
          total_stored_bytes_++;
        }
      }
    } else // otherwise modify bool
    {
      for (uint64_t i=0; i<cur_idx + avai_cap - first_index; i++) {
        if (container_.find(data[i]) == container_.end()) {
          container_[first_index + i].st_char = data[i];
          total_stored_bytes_++;
        }
      }
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
      cout << "data " << data << endl;
    }
  }

  auto pt_itr = container_.begin();
  while (pt_itr != container_.end()) {
    cout << "first " << pt_itr->first << endl;
    cout << "char " << pt_itr->second.st_char << endl;
    pt_itr++;
  }

  // push part
  string tmp_str = "";
  uint64_t i = 0;
  while (i < avai_cap && i < data.size()) {
    tmp_str += data[i];
    i++;
  }
  tmp_writer.push(tmp_str);
  cur_idx += tmp_str.size();

  cout << "cur idx after push " << cur_idx << endl;
  cout << "cont size " << container_.size() << endl;
  cout << "total " << total_stored_bytes_ << endl;
  pt_itr = container_.begin();
  while (pt_itr != container_.end()) {
    cout << "1 first " << pt_itr->first << " char " << pt_itr->second.st_char << endl;
    pt_itr++;
  }

  // delete map until entry has unpushed part
  auto itr = container_.begin();
  while (itr != container_.end() && itr->first < cur_idx) {
    auto next = std::next(itr);
    container_.erase(itr);
    total_stored_bytes_--;
    itr = next;
  }

  cout << endl << "cont size " << container_.size() << endl;
  cout << "total " << total_stored_bytes_ << endl;
  pt_itr = container_.begin();
  while (pt_itr != container_.end()) {
    cout << "2 first " << pt_itr->first << " char " << pt_itr->second.st_char << endl;
    pt_itr++;
  }

  // push map until discontinuous
  tmp_str = "";
  itr = container_.begin();
  while (itr != container_.end() && itr->first == cur_idx) {
    auto next = std::next(itr);
    tmp_str += itr->second.st_char;
    container_.erase(itr);
    total_stored_bytes_ --;
    cur_idx ++;
    itr = next;
  }

  cout << endl << "cont size " << container_.size() << endl;
  cout << "total " << total_stored_bytes_ << endl;
  pt_itr = container_.begin();
  while (pt_itr != container_.end()) {
    cout << "3 first " << pt_itr->first << " char " << pt_itr->second.st_char << endl;
    pt_itr++;
  }

  tmp_writer.push(tmp_str);

  if (is_last_substring) {
    tmp_writer.close();
    return;
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_stored_bytes_;
}
