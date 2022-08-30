#ifndef _NEW_COMMUNICATOR_HPP
#define _NEW_COMMUNICATOR_HPP

#include "RemoteComm.hpp"      
#include "LocalComm.hpp"
#include "threadDefs.hpp" 

#include <vector>
#include <cassert>

#include <unordered_map>

class ThreadResource; 

class Communicator
{
  typedef Communicator SELF ; 
public: 
  /** 
      @param tid2rank absolute ranks in the communicator ; contains only ranks of local threads   
  */ 
  Communicator(std::unordered_map<tid_t,int> tid2rank); 
  Communicator(const Communicator& rhs) = delete; 
  Communicator(Communicator &&rhs) = default; 
  ~Communicator(){}
  Communicator& operator=(Communicator rhs) ; 
  friend void swap(Communicator& lhs, Communicator &rhs); 

  void createSendRequest(std::vector<char> array, int dest, int tag, CommRequest& req);
  void createRecvRequest(int src, int tag, nat length, CommRequest& req); 

#include "CommCore.hpp"

  friend std::ostream& operator<<(std::ostream & out, const Communicator& rhs); 
  
  int mapToLocalRank( int rank) const  ; 
  int mapToRemoteRank(int rank) const ; 

  int getProcsPerNode() ; 
  
  LocalComm&  getLocalComm() ; 
  RemoteComm& getRemoteComm(); 

  void initWithMaxChains(size_t numChains, size_t numThreadsChecking); 
  
private:
  RemoteComm _remoteComm; 
  LocalComm _localComm; 
};  


#include "Communicator.tpp"

#endif


