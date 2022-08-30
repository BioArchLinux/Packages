#include <memory>

#include "PendingSwap.hpp"

#include "ParallelSetup.hpp"
#include "SwapElem.hpp"

#include "PendingSwapImpl.hpp"

#include "extensions.hpp"

#include "LocalSwap.hpp"

using namespace std;

PendingSwap::PendingSwap( SwapElem swap, bool isLocal )
  : _impl{nullptr}
{
  if(isLocal)
    _impl = make_unique<LocalSwap>(swap) ;
  else 
    _impl = make_unique<PendingSwap::Impl>(swap); 
}

PendingSwap::~PendingSwap()
{
}


PendingSwap::PendingSwap(PendingSwap&& rhs)
  : _impl(std::move(rhs._impl))
{
  rhs._impl = nullptr; 
}


std::vector<char> PendingSwap::getRemoteData() const  
{
  return _impl->getRemoteData(); 
} 


void PendingSwap::initialize(ParallelSetup& pl, std::vector<char> myChainSer, nat runid) 
{
  _impl->initialize(pl, myChainSer, runid);
}


bool PendingSwap::allHaveReceived(ParallelSetup& pl)  
{
  return _impl->allHaveReceived(pl); 
}


bool PendingSwap::isFinished()  
{
  return _impl->isFinished();
}


SwapElem PendingSwap::getSwap() const 
{
  return _impl->getSwap();
}


void swap(PendingSwap& lhs, PendingSwap& rhs)
{
  std::swap(lhs._impl, rhs._impl);
}
