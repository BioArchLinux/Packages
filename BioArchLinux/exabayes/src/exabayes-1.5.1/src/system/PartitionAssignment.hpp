#ifndef _PARTITION_ASSIGNMENT_HPP
#define _PARTITION_ASSIGNMENT_HPP

#include <map>

#include "common.h"
#include "Partition.hpp"	

struct Assignment
{
  nat partId; 
  nat procNum;
  nat offset; 
  nat width; 
  nat compStates; 
}; 




std::ostream& operator<<(std::ostream& out, const Assignment& rhs); 

struct PartInfo
{
  nat id; 
  nat num; 
  nat compStates; 		// not used in modern thingie 
}; 


class PartitionAssignment
{
public: 
  explicit PartitionAssignment(size_t size )
    : _numProc(size)
    , _proc2assignment{}
  {
  }

  size_t getTotalWidthPerProc(nat proc) const; 
  void assignOld(const std::vector<Partition>& pass); 

  void assign(const std::vector<Partition>& pass); 
  const std::multimap<nat,Assignment>& getAssignment() const {return _proc2assignment; }
  std::pair< std::vector<int>,std::vector<int> > getCountsAndDispls(size_t bla) const ; 

  size_t getNumProc() const {return _numProc; }

  auto getNumPartPerProcess() const -> std::vector<nat>; 
  auto getSitesPerProcess() const  -> std::vector<nat>; 

private: 			// METHODS 
  void assignForType( std::vector<PartInfo> ); 
  void improvedAssign( std::vector<PartInfo> partitions); 
  void _assignToProcFull(PartInfo p, nat proc, std::vector<nat> &numAssigned, std::vector<nat> &sizeAssigned) ; 
  void _assignToProcPartially(PartInfo p, nat proc, std::vector<nat> &numAssigned, std::vector<nat> &sizeAssigned, nat offset, nat numElem) ; 
  
private: 			// ATTRIBUTES
  size_t _numProc; 
  std::multimap<nat,Assignment> _proc2assignment;
}; 


#endif
