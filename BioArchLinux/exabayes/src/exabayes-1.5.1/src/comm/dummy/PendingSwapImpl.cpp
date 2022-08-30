#include "PendingSwapImpl.hpp"

#include <string>
#include "SwapElem.hpp"
#include "ParallelSetup.hpp"

PendingSwap::Impl::Impl(SwapElem elem)
  : AbstractPendingSwap(elem)
{
}

std::vector<char> PendingSwap::Impl::getRemoteData() const 
{
  assert(0) ;
  return std::vector<char>{}; 
}
  
void PendingSwap::Impl::initialize(ParallelSetup& pl, std::vector<char> myChainSer, nat runid)
{
}

bool PendingSwap::Impl::allHaveReceived(ParallelSetup& pl)  
{
  return true; 
} 

bool PendingSwap::Impl::isFinished()  
{
  return true; 
}   
