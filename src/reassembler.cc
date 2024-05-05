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
    auto res = container.begin();
    // store all data
    if (first_index + data.size() <= cur_idx + avai_cap) {
      res = container.insert(std::make_pair(first_index, std::make_pair(data, is_last_substring))).first;
    } else // otherwise store until avai
    {
      // first > cur
      data = data.resize(cur_idx + avai_cap - first_index);
      res = container.insert(std::make_pair(first_index, std::make_pair(data, false))).first;
    }

    //update pending bytes
    total_stored_bytes_ += data.size();

    // two snippets after each store
    // if res != end, backward merge
    // backward overlap // modify while condition
    while (std::next(res) != container.end() && (first_index + data.size() > std::next(res)->first)) {
      auto next = std::next(res);
      // all include
      if (first_index + data.size() > next->first + next->second.first.size()) {
        total_stored_bytes_ -= next->second.first.size();

        // direct erase
        container.erase(next);
      }
      else { // partial overlap
        total_stored_bytes_ -= (first_index + data.size() - next->first);

        // merge backwards
        string new_str = data.resize(next->first-first_index);
        new_str += next->second.first;
        bool new_bool = next->second.second;
        res->second = std::make_pair(new_str, new_bool);
        container.erase(next);
      }
    } // otherwise no backmerge
    

    // res prev != null, forward merge
    if (res != container.begin()) {
      auto prev = std::prev(res);

      auto prev_second = prev->second;
      // if overlap
      if (prev->first + prev_second.first.size() > first_index) {
        // all include
        if (prev->first + prev_second.first.size() > first_index + data.size()) {
          total_stored_bytes_ -= data.size();
          container.erase(res);
        } else { // partial overlap
          total_stored_bytes_ -= (prev->first + prev_second.first.size() - first_index);

          // merge with prev
          string new_str = prev_second.first.resize(first_index-prev->first);
          new_str += data;
          prev->second = std::make_pair(new_str, is_last_substring);
          container.erase(res);
        }
      }
    }

    return;
  } 

  // push next bytes
  if (first_index < cur_idx) {
    if (first_index + data.size() <= cur_idx) {
      return;
    } else {
      data = data.substr(cur_idx - first_index);
    }
  }
  // exceeds cap
  if (data.size() > avai_cap) {
    data = data.resize(avai_cap);
  } // else push data
  tmp_writer.push(data);
  cur_idx += data.size();

  // delete next map entry
  while (container.begin()->first <= cur_idx){
    // manually push
    auto man_itr = container.begin();
    auto tmp_second = man_itr->second;
    tmp_writer.push(tmp_second.first);
    total_stored_bytes_ -= tmp_second.first.size();
    cur_idx += tmp_second.first.size();
    if (tmp_second.second) {
      eof_ = true;
    }
    container.erase(man_itr);
  }
  
  // last
  if (is_last_substring || eof_) {
    tmp_writer.close();
    return;
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_stored_bytes_;
}

// helper printer
/*cout << endl << "cont size " << container_.size() << endl;
  cout << "total " << total_stored_bytes_ << endl;
  pt_itr = container_.begin();
  while (pt_itr != container_.end()) {
    cout << "2 first " << pt_itr->first << " char " << pt_itr->second.st_char << endl;
    pt_itr++;
  }*/