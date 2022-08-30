#ifndef REVERVE_ARRAYS_HPP
#define REVERVE_ARRAYS_HPP

#include <list>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include "common.h"


class ArrayReservoir
{
public: 
  ArrayReservoir(bool freeOlds) ; 
  ~ArrayReservoir(); 
  ArrayReservoir(const ArrayReservoir& rhs) = delete; 
  ArrayReservoir& operator=(const ArrayReservoir& rhs) = delete; 

  double* allocate(size_t requiredLength ); 
  void deallocate(double* array); 

#ifdef _DEVEL
  std::tuple<uint64_t,uint64_t> getUsedAndUnusedBytes() const ; 
#endif

private: 
  std::unordered_map<double*,size_t> _usedArrays; 
  std::map<size_t,std::list<double*> > _unusedArrays; 
  
  static const double numGammaCats; 
  static const double thresholdForNewSEVArray; 
  bool _freeOlds; 
}; 
#endif
