#ifndef _REMOTE_COMM_HPP
#define _REMOTE_COMM_HPP

#include <vector>
#include <memory>

#include <iostream>

#include "common.h"

class CommRequest; 

class RemoteComm
{
public: 
  class Impl; 			// CLASS declaration!
  typedef RemoteComm SELF ; 

  RemoteComm() ; 
  RemoteComm(const RemoteComm &rhs)   ; 
  RemoteComm( RemoteComm &&rhs)  ; 
  ~RemoteComm();
  RemoteComm& operator=( RemoteComm rhs); 
  friend void swap(RemoteComm &lhs, RemoteComm& rhs); 

  #include "CommCore.hpp"

  void waitAtBarrier() const; 

  void createSendRequest(std::vector<char> array, int dest, int tag, CommRequest& req); 
  void createRecvRequest(int src, int tag, nat length, CommRequest& req); 

  friend std::ostream& operator<<(std::ostream& out, const RemoteComm& rhs)
  {
    return out << "R-" << rhs.getRank()  << "/" << rhs.size() ; 
  }
  
  nat getNumberOfPhysicalNodes() ; 
  int getOffsetForThreadPin() ; 

  static uint64_t getMaxTag() ; 
  
private:
  std::unique_ptr<RemoteComm::Impl> _impl; 
}; 


#endif
