#ifndef _SYNC_OSTREAM_HPP
#define _SYNC_OSTREAM_HPP

#include <sstream>
#include <mutex>
#include "GlobalVariables.hpp"

/** 
    thread safe stream
*/ 


class SyncOut
{
public: 
  SyncOut()
    : _lock(mtx)
    , _ss{}
  {
  }
  
  ~SyncOut(){}

  template<typename T>
  friend SyncOut& operator<<(SyncOut& out, T elem)
  {
    out._ss << elem ; 
    return out; 
  }

  friend std::ostream& operator<<(std::ostream &out, const SyncOut& rhs)
  {
    out << rhs._ss.str() ; 
    return out; 
  }
  
private :
  std::lock_guard<std::mutex> _lock; 
  std::stringstream _ss; 
}; 

#endif
