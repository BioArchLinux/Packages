#ifndef _REMOTE_COMM_IMPL_SEQ_HPP
#define _REMOTE_COMM_IMPL_SEQ_HPP

#include <vector>

#include "common.h"

class CommRequest; 

#include "RemoteComm.hpp"
#include "CommCoreNew.hpp"

class RemoteComm::Impl
{
  typedef Impl SELF; 

public: 
  Impl()
    : dummy{0}
  {}  
  Impl(const Impl& rhs)
    : dummy{rhs.dummy}
  {}

  Impl& operator=( Impl rhs); 
  friend void swap(Impl &lhs, Impl& rhs)
  {
    std::swap(lhs.dummy, rhs.dummy); 
  }; 

  ~Impl(){}; 

  void createSendRequest(std::vector<char> array, int dest, int tag, CommRequest& req);
  void createRecvRequest(int src, int tag, nat length, CommRequest& req);  

  void waitAtBarrier() const {} 

#include "CommCore.hpp"

  nat getNumberOfPhysicalNodes();
  static uint64_t _maxTagValue; 

private: 
  nat dummy; 
}; 

#include "comm/dummy/RemoteCommImpl.tpp"


#endif
