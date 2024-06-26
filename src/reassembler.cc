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

    // if larger replacement
    if (container.find(first_index) != container.end()) {
      if (data.size() > container[first_index].first.size()) {
        total_stored_bytes_ -= container[first_index].first.size();
        container.erase(first_index);
      } else {
        return;
      }
    }

    auto res = container.begin();
    // store all data
    if (first_index + data.size() <= cur_idx + avai_cap) {
      res = container.insert(std::make_pair(first_index, std::make_pair(data, is_last_substring))).first;
    } else // otherwise store until avai
    {
      // first > cur
      data.resize(cur_idx + avai_cap - first_index);
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
        data.resize(next->first-first_index);
        string new_str = data;
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
          prev_second.first.resize(first_index-prev->first);
          string new_str = prev_second.first;
          new_str += res->second.first;
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
    data.resize(avai_cap);
  } // else push data
  tmp_writer.push(data);
  cur_idx += data.size();

  // delete next map entry
  auto last_itr = container.begin();
  while (last_itr != container.end() && 
          last_itr->first <= cur_idx){

    // manually push
    auto next = std::next(last_itr);
    auto tmp_second = last_itr->second;
    
    total_stored_bytes_ -= tmp_second.first.size();

    // all include
    if (tmp_second.first.size() + last_itr->first > cur_idx) {
      tmp_second.first = tmp_second.first.substr(cur_idx - last_itr->first);
      tmp_writer.push(tmp_second.first);
      cur_idx += tmp_second.first.size();
      if (tmp_second.second) {
        eof_ = true;
    }
    }
    container.erase(last_itr);
    last_itr = next;
  }

  // last
  if (is_last_substring || eof_) {
    eof_ = true;
    tmp_writer.close();
    return;
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return total_stored_bytes_;
}