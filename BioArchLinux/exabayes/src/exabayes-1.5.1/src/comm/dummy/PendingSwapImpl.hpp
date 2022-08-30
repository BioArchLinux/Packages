#ifndef _PENDING_SWAP_IMPL_HPP
#define _PENDING_SWAP_IMPL_HPP

class ParallelSetup; 

#include <string>
#include <vector>

#include "AbstractPendingSwap.hpp"

class PendingSwap::Impl : public AbstractPendingSwap
{
public: 
  Impl(SwapElem elem); 
  virtual ~Impl(){}
  std::vector<char> getRemoteData() const ; 
  void initialize(ParallelSetup& pl, std::vector<char> myChainSer, nat runid); 
  bool allHaveReceived(ParallelSetup& pl)  ; 
  bool isFinished()  ; 
}; 


#endif
