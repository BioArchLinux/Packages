#include "ArrayReservoir.hpp" 
#include "GlobalVariables.hpp" 
#include <algorithm>
#include <cassert>
#include "extensions.hpp"

const double ArrayReservoir::thresholdForNewSEVArray = 1.05 ; 

ArrayReservoir::ArrayReservoir(bool freeOlds)  
  : _usedArrays{}
  , _unusedArrays{}
  , _freeOlds(freeOlds)
{
}


ArrayReservoir::~ArrayReservoir()
{
  for(auto elem : _unusedArrays)
    {
      for(auto elem2 : std::get<1>(elem))
	free(elem2);
    }
  for(auto elem : _usedArrays)
    free(std::get<0>(elem)); 
} 


double* ArrayReservoir::allocate(size_t requiredLength)
{
  double *result = nullptr; 

  auto found = _unusedArrays.lower_bound(requiredLength); 
  
  auto lengthOfFound = std::get<0>(*found);
  
  // we free anyway such that we do not acccumulate a lot of large
  // arrays when the SEV technique is used
  bool freeOldArrayDespiteAvailable =  _freeOlds &&   ( size_t(double(requiredLength) * thresholdForNewSEVArray)  < lengthOfFound )  ; 

  if( found != end(_unusedArrays)
      && not freeOldArrayDespiteAvailable )
    {
      auto id = std::get<0>(*found); 
      auto theList = std::get<1>(*found); 
      assert(theList.size() > 0); 
      result = theList.front(); 
      std::get<1>(*found).pop_front();
      if(std::get<1>(*found).size() == 0)
	_unusedArrays.erase(found);
      _usedArrays[result] = id; 
    }
  else 
    {				
      if(_freeOlds &&   _unusedArrays.size() > 0  )
	{
	  auto maxElemIter = std::max_element(begin(_unusedArrays), end(_unusedArrays));
	  auto &maxElem = *maxElemIter; 
	  free(std::get<1>(maxElem).front()); 
	  std::get<1>(maxElem).pop_front(); 
	  if(std::get<1>(maxElem).size() == 0)
	    _unusedArrays.erase(maxElemIter); 
	}

      // tout << "allocating elem of length " <<  requiredLength<<  "\t" << SHOW(lengthOfFound)  << std::endl; 
      
      result = aligned_malloc<double,size_t(EXA_ALIGN)>(requiredLength); 
      _usedArrays.insert(std::make_pair(result, requiredLength));
    }

  assert(result != nullptr) ; 
  // tout << "POP " << result << std::endl; 
  return result; 
}


void ArrayReservoir::deallocate(double* array)
{
  // tout << "push " << array <<  std::endl; 

  assert(array != nullptr); 

  auto iter = _usedArrays.find(array); 

  assert(iter != _usedArrays.end()); 
  auto elem = *iter; 

  _unusedArrays[std::get<1>(elem)].push_front(std::get<0>(elem)); 

  _usedArrays.erase(iter);

  assert(_usedArrays.find(array) == _usedArrays.end() ); 
  
  array = nullptr; 
} 


#ifdef _DEVEL
std::tuple<uint64_t,uint64_t> ArrayReservoir::getUsedAndUnusedBytes() const 
{
  uint64_t usedBytes =  std::accumulate(begin(_usedArrays), end(_usedArrays), 0, 
				     [](nat elem, const std::pair<double*,nat> &elemB) { return elem + std::get<1>(elemB); } );
  uint64_t unusedBytes = std::accumulate(begin(_unusedArrays), end(_unusedArrays), 0, 
				       [](nat elem , const std::pair<nat,double*> &elemB) { return elem  + std::get<0>(elemB); });
 return std::make_tuple(usedBytes, unusedBytes);
} 
#endif
