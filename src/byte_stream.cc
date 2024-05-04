#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), bytestream_queue_() {}

bool Writer::is_closed() const
{
  return end_writer_;
}

void Writer::push( string data )
{ 
  uint64_t data_size = data.size();
  uint64_t avai_num = available_capacity();
  //uint64_t cur_idx = 0;
  if (data_size > avai_num) {
    for (uint64_t i=0; i<avai_num; i++) {
      bytestream_queue_.push(data[i]);
      bytes_buffed_++;
      total_bytes_pushed_++;
    }
  } else {
    for (uint64_t i=0; i<data_size; i++) {
      bytestream_queue_.push(data[i]);
      bytes_buffed_++;
      total_bytes_pushed_++;
    }
  }
  
}

void Writer::close()
{
  end_writer_ = true;
}

uint64_t Writer::available_capacity() const
{ 
  // available spots in stream
  return capacity_ - bytes_buffed_;
}

uint64_t Writer::bytes_pushed() const
{
  return total_bytes_pushed_;
}

bool Reader::is_finished() const
{
  return end_writer_ && (bytes_buffed_ == 0);
}

uint64_t Reader::bytes_popped() const
{
  return total_bytes_popped_;
}

string_view Reader::peek() const
{
  // only see the front
  return string_view(&bytestream_queue_.front(), 1);
}

void Reader::pop( uint64_t len )
{ 
  // exceeds buffed bytes
  if (len > bytes_buffed_) {
    //throw std::runtime_error("pop length larger than buffed bytes");
    for (uint64_t i=0; i<bytes_buffed_; i++){
      bytestream_queue_.pop();
      bytes_buffed_--;
      total_bytes_popped_++;
    }
  } else {
    for (uint64_t i=0; i<len; i++){
      bytestream_queue_.pop();
      bytes_buffed_--;
      total_bytes_popped_++;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  return bytes_buffed_;
}
