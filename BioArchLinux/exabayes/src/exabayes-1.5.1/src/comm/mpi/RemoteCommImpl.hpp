#ifndef _REMTE_COMM_IMPL_PARA_HPP
#define _REMTE_COMM_IMPL_PARA_HPP

#include "MpiType.hpp"
#include <vector>

#include "common.h"

class CommRequest; 

#include "RemoteComm.hpp"

class RemoteComm::Impl
{
public: 
  typedef Impl SELF; 
 
  Impl() 
    : _comm{MPI_COMM_WORLD}
  {
    MPI_Comm_dup(MPI_COMM_WORLD, &_comm);
  }
 
  Impl(const Impl& rhs); 
  Impl& operator=( Impl rhs); 
  friend void swap(Impl &lhs, Impl& rhs); 
  ~Impl(); 

  void waitAtBarrier() const; 

  #include "CommCore.hpp"

  void createSendRequest(std::vector<char> array, int dest, int tag, CommRequest &req); 
  void createRecvRequest(int src, int tag, nat length, CommRequest& req ); 

  nat getNumberOfPhysicalNodes()  ; 

  static uint64_t _maxTagValue; 
private: 
  MPI_Comm _comm; 
  
}; 


#include "comm/mpi/RemoteCommImpl.tpp"


#endif
