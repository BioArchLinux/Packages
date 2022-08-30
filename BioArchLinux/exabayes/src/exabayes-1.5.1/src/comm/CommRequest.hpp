#ifndef _COMM_REQUEST_HPP
#define _COMM_REQUEST_HPP

#include "common.h"

#include <vector>
#include <memory>

#include <cassert>

class RemoteComm; 

class CommRequest
{
public: 
  class Impl; 
  friend class RemoteComm; 

  CommRequest(std::vector<char> array = std::vector<char>{}); 
  CommRequest(CommRequest &&rhs)   ; 
  CommRequest(const CommRequest& rhs) = delete; 
  CommRequest& operator=(CommRequest rhs); 
  ~CommRequest(); 

  friend void  swap(CommRequest& lhs, CommRequest& rhs); 

  std::vector<char> getArray() const; 
  bool isServed() const; 

private: 
  std::unique_ptr<CommRequest::Impl> _impl; 
}; 



#endif
