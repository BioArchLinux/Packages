#ifndef _UNFINISHED_SWAP
#define _UNFINISHED_SWAP

#include <cassert>
#include <vector>
#include <memory>

/** 
    @file PendingSwap.hpp

    @brief represents a swap for which in the parallel case have not received the other chain yet

    This class has no meaning in non-MPI-versions. 
*/ 


#include "common.h"

class ParallelSetup; 
class SwapElem; 

class AbstractPendingSwap; 

class PendingSwap
{
public: 
  class Impl; 			// since we have an abstract variant, this does not really fit anymore 

  PendingSwap( SwapElem swap, bool isLocal); 
  PendingSwap(PendingSwap&& rhs)     ;
  PendingSwap& operator=( const PendingSwap& elem)   = delete; 
  PendingSwap( const PendingSwap& rhs) = delete; 
  ~PendingSwap();
  
  friend void swap(PendingSwap& lhs, PendingSwap& rhs); 
  
  bool isFinished();

  void initialize(ParallelSetup& pl, std::vector<char> myChainSer, nat runid); 
  SwapElem getSwap() const ; 

  bool allHaveReceived(ParallelSetup& pl) ;

  std::vector<char> getRemoteData()  const; 
  
private: 			// METHODS 
  int createTag( nat numChains ) const ; 

private: 			// ATTRIBUTES
  std::unique_ptr<AbstractPendingSwap> _impl; 
}; 

#endif

