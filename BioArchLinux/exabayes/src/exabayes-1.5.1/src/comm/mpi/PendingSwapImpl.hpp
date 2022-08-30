#ifndef _PENDING_SWAP_IMPL_HPP
#define _PENDING_SWAP_IMPL_HPP

#include <vector>

#include "AbstractPendingSwap.hpp"

class PendingSwap::Impl : public AbstractPendingSwap
{
public: 
  Impl(SwapElem elem); 

  std::vector<char> getRemoteData() const ; 
  bool isFinished()  ;   
  bool allHaveReceived(ParallelSetup& pl)  ; 
  void initialize(ParallelSetup& pl, std::vector<char> myChainSer, nat runid) ; 

private: 
  uint64_t createTag( int rank ) const ; 

private: 
  std::vector<CommRequest> _sentReqs; 
  CommRequest _recvReq; 
}; 


#endif
