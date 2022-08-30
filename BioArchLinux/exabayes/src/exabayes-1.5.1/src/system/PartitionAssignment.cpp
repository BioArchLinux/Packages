#include <algorithm>
#include <cassert>
#include <list>

#include "GlobalVariables.hpp"
#include "PartitionAssignment.hpp"


static inline int popAndYield(std::vector<nat> &list)
{
  if(list.empty())
    return -1; 
  else 
    {
      auto val = list.back();
      list.pop_back();
      return val; 
    }
}  


static inline nat openNewPartition(std::vector<PartInfo>::iterator &iter, std::vector<PartInfo>::iterator end)
{
  auto result = 0; 

  if(iter != end)
    ++iter; 
  if(iter != end)
    result = iter->num; 

  return result; 
}


void PartitionAssignment::improvedAssign( std::vector<PartInfo> partitions )
{
  std::sort(begin(partitions), end(partitions), []( const PartInfo& a,  const PartInfo&b ) {return a.num < b.num; }); 
  
  auto partIter = begin(partitions);
  
  auto __totalSites = 0u; 
  for(auto &p : partitions)
    __totalSites += p.num; 

  auto cap = static_cast<nat>(ceil( double(__totalSites) / double(_numProc) )); 
  auto remainder =  int(cap * _numProc -  __totalSites); 
  assert(remainder >= 0);   
  auto numFull = 0u; 
  auto numAssigned = std::vector<nat>(_numProc,0); 
  auto sizeAssigned = std::vector<nat>(_numProc, 0); 

  auto numLow = 0u; 
  auto iterate = true;

  // phase 2 initial assignment 
  while(iterate)
    {
      for(nat proc = 0; proc < _numProc; ++proc)
	{
	  if( partIter != end(partitions) && sizeAssigned[proc] + partIter->num <= cap )
	    {
	      _assignToProcFull(*partIter,proc, numAssigned, sizeAssigned); 

	      if(sizeAssigned[proc] == cap)
		{
		  ++numFull; 
		  if(numFull == _numProc - remainder )
		    --cap; 
		}
	      ++partIter;
	    }
	  else 
	    {
	      // low = proc+1; 	// +1 necessary? 
	      numLow = numAssigned[proc];
	      iterate = false;
	      break; 
	    }
	}
    }


  // phase 4: stick breaking 
  auto procsHigh = std::vector<nat>{}; 
  auto procsLow = std::vector<nat>{}; 
  numFull = 0; 
  for(nat proc = 0; proc < _numProc; ++proc)
    {
      if( sizeAssigned[proc] < cap)
	{
	  if( numAssigned[proc] == numLow )
	    procsLow.push_back(proc);
	  else if(numAssigned[proc] == numLow + 1)
	    procsHigh.push_back(proc); 
	}
      else 
	++numFull; 
    }
  assert(procsHigh.size() + procsLow.size() + numFull == _numProc); 

  int toAdd = ( partIter == end(partitions )) ? 0 : partIter->num ; 

  int highProc =  procsHigh.empty() ? popAndYield(procsLow) : popAndYield(procsHigh) ;
  int lowProc =  procsLow.empty() ? popAndYield(procsHigh) : popAndYield(procsLow) ; 

  while( not (procsLow.empty() &&  procsHigh.empty() && highProc == -1 && lowProc == -1 ))
    {
      if(highProc != - 1  && sizeAssigned[highProc] + toAdd >= cap)
	{
	  auto toTransfer = cap - sizeAssigned[highProc]; 
	  auto offset = partIter->num - toAdd; 
	  
	  _assignToProcPartially(*partIter, highProc, numAssigned, sizeAssigned, offset, toTransfer); 
	  toAdd -= toTransfer; 

	  if(toAdd == 0 )
	    {
	      toAdd = openNewPartition(partIter, end(partitions)); 
	      if(toAdd == 0)
		break; 
	    }

	  ++numFull; 
	  
	  if(numFull == _numProc - remainder)
	    {
	      --cap; 
	    }
	  
	  highProc = procsHigh.empty() ? popAndYield(procsLow) : popAndYield(procsHigh); 
	}
      else if(lowProc != - 1)
	{
	  if ( sizeAssigned[lowProc] + toAdd < cap)
	    {
	      auto offset = partIter->num - toAdd; 
	      _assignToProcPartially(*partIter, lowProc, numAssigned, sizeAssigned, offset, toAdd); 
	      if(highProc != -1 )
		procsHigh.push_back(highProc); 
	      highProc = lowProc;
	      toAdd = openNewPartition(partIter, end(partitions)); 
	      if(toAdd == 0)
		break; 
	      lowProc = procsLow.empty() ? popAndYield(procsHigh) : popAndYield(procsLow); 
	    }
	  else 
	    {
	      auto toTransfer = cap - sizeAssigned[lowProc] ; 
	      auto offset = partIter->num - toAdd; 
	      _assignToProcPartially(*partIter, lowProc, numAssigned, sizeAssigned, offset, toTransfer); 
	      toAdd -= toTransfer;
	      
	      if(toAdd == 0 )
		{
		  toAdd = openNewPartition(partIter, end(partitions)); 
		  if(toAdd == 0)
		    break; 
		}

	      ++numFull; 
	      if(numFull == _numProc - remainder)
		--cap; 

	      lowProc = procsHigh.empty() ? popAndYield(procsLow) : popAndYield(procsHigh) ; 
	    }
	}
    }

  assert(toAdd == 0); 
  assert(partIter == end(partitions)); 
}


void PartitionAssignment::_assignToProcFull(PartInfo p, nat proc, std::vector<nat> &numAssigned, std::vector<nat> &sizeAssigned) 
{
  auto assnmnt = Assignment{p.id, proc, 0, p.num, 0}; 
  _proc2assignment.insert(std::make_pair(proc, assnmnt));
  sizeAssigned[proc] += p.num; 
  ++numAssigned[proc];   
}


void PartitionAssignment::_assignToProcPartially(PartInfo p, nat proc, std::vector<nat> &numAssigned, std::vector<nat> &sizeAssigned, nat offset, nat numElem) 
{
  assert(numElem > 0 ); 
  auto assignment = Assignment{p.id, proc, offset, numElem, 0}; 
  _proc2assignment.insert(std::make_pair(proc, assignment));
  sizeAssigned[proc] += numElem; 
  ++numAssigned[proc];   
}


void PartitionAssignment::assignForType(  std::vector<PartInfo> curParts )
{
  // extract relevant elements 
  nat totalSites = 0; 
  for(auto &p : curParts)
    totalSites += p.num; 

  auto sitesPerProc = int(totalSites / _numProc); 
  auto remainder = totalSites - (sitesPerProc * _numProc); 
      
  std::sort(begin(curParts), end(curParts), [](const PartInfo &a, const PartInfo& b){ return a.num < b.num ; }); 

  auto sitesLeft = std::vector<int>(_numProc, sitesPerProc);
  for(auto i = 0u; i< remainder; ++i)
    ++sitesLeft[i]; 

  // that's a bit much, but it only assures that elements are
  // always ordered by 
  // * number of partitions assigned so far (first, ascending)
  // * number of sites assigned so far (second, descending)  
  auto comparator = [](const std::pair<int,int> &a, const std::pair<int,int>& b)
    {
      return ( a.first == b.first  && a.second  > b.second ) 
      || a.first < b.first; 
    }; 
  
  auto numPartAndSites2Proc = std::multimap<std::pair<int,int>,int, decltype(comparator)>(comparator); 
  for(nat i = 0; i < _numProc; ++i)
    numPartAndSites2Proc.insert(std::make_pair(std::make_pair(0, 0), i)); 
      
  for(auto part : curParts)
    {
      int toAssign = part.num; 
      nat offset = 0; 
	  
      while(toAssign > 0 )
	{
	  auto ptr2elem = begin(numPartAndSites2Proc); 
	  auto proc = ptr2elem->second; 
	  auto numParts = ptr2elem->first.first; 
	  auto numSites = ptr2elem->first.second; 
	  auto elemsToTransfer = std::min(sitesLeft[proc], toAssign); 
	  assert(elemsToTransfer > 0 ); 

	  // create assignment 
	  _proc2assignment.insert(std::make_pair(proc, Assignment{part.id, nat(proc), nat(offset), nat(elemsToTransfer), 0}));

	  // update bookkeeping 
	  sitesLeft[proc] -= elemsToTransfer; 
	  toAssign -= elemsToTransfer; 
	  offset += elemsToTransfer; 
	  numPartAndSites2Proc.erase(ptr2elem);
	  ++numParts ; 
	  numSites += elemsToTransfer; 

	  if(sitesLeft[proc] > 0)
	    numPartAndSites2Proc.insert(std::make_pair(std::make_pair(numParts, numSites), proc)); 
	}
    }
}







void PartitionAssignment::assign(const std::vector<Partition>& partitions)
{
  //  order partitions by type 
  auto typeToPart =  std::multimap<nat, PartInfo>{}; 
  nat ctr = 0; 
  for( auto &p : partitions )
    {
      auto type = p.getDataType(); 
      typeToPart.insert(std::make_pair(type, PartInfo{ctr, nat(p.getUpper() - p.getLower()), 0}) );
      ++ctr; 
    }
  
  auto uniqKeys = std::vector<std::pair<nat, PartInfo>> {};
  std::unique_copy(begin(typeToPart),
		   end(typeToPart),
		   std::back_inserter(uniqKeys),
		   [](const std::pair<nat, PartInfo> &entry1, const std::pair<nat, PartInfo> &entry2) 
		   {
		     return (entry1.first == entry2.first);
		   }
             );
  
  for(auto uniq : uniqKeys)
    {
      auto iterPair = typeToPart.equal_range(uniq.first) ; 
      auto relevant = std::vector<PartInfo>{}; 
      for( auto iter = iterPair.first; iter != iterPair.second; ++iter )
	relevant.push_back(iter->second); 
      
#if 0 
      assignForType(relevant); 
#else 
      improvedAssign(relevant); 
#endif
    }
  
  assert(_proc2assignment.size() >= partitions.size()); 
}


void PartitionAssignment::assignOld(const std::vector<Partition>& partitions)
{
  // sort partitions by computational load 
  auto ps = std::vector<PartInfo>{}; 
  nat ctr = 0; 
  for(auto &p : partitions)
    {
      nat len = p.getHandle().upper - p.getHandle().lower; 
      nat load = p.getHandle().states  * p.getHandle().states ; 
      ps.push_back(PartInfo{ctr, len, load});
      ++ctr; 
    }
  std::sort(begin(ps), end(ps), [](const PartInfo& a, const PartInfo &b){return a.num * a.compStates > b.num * b.compStates; }); 

  double totalLoad = 0; 
  for(auto &p : ps)
    totalLoad += p.num * p.compStates; 

  nat loadPerProc = nat(ceil(totalLoad / double(_numProc))); 
  nat curProc = 0; 
  nat curLoad = 0; 
  for(auto &p : ps)
    {
      nat partitionElemsLeft =  p.num; 
      nat offset = 0; 
      while(curProc < _numProc && partitionElemsLeft != 0) 
	{
	  nat elemsToTransfer = nat(ceil(double(std::min(partitionElemsLeft * p.compStates, loadPerProc - curLoad) ) / double(p.compStates))); 
	  assert(elemsToTransfer > 0 ); 

	  _proc2assignment.insert(std::make_pair(curProc, Assignment{p.id, curProc, offset, elemsToTransfer, p.compStates}));

	  offset += elemsToTransfer; 

	  partitionElemsLeft -= elemsToTransfer; 
	  curLoad += elemsToTransfer * p.compStates; 
	  
	  if(loadPerProc <= curLoad)
	    {
	      curLoad = 0; 
	      ++curProc; 
	    }
	}
      assert(offset == p.num); 
    }
}


std::ostream& operator<<(std::ostream& out, const Assignment& rhs) 
{
  out << "id=" << rhs.partId << "\t->" << rhs.procNum << "\t(" << rhs.offset << "," << rhs.width << ")" ; 
  return out; 
}


size_t PartitionAssignment::getTotalWidthPerProc(nat proc) const 
{
  size_t result = 0; 
  auto iterPair = _proc2assignment.equal_range(proc); 
  for(auto iter = std::get<0>(iterPair); iter != std::get<1>(iterPair); ++iter)
    result += std::get<1>(*iter).width; 
  return result; 
}


std::vector<nat> 
PartitionAssignment::getSitesPerProcess() const 
{
  auto result = std::vector<nat>(_numProc,0);
  for(auto &p : _proc2assignment)
    {
      auto ass = std::get<1>(p); 
      result[ass.procNum] += ass.width; 
    }
  return result; 
}


auto PartitionAssignment::getNumPartPerProcess() const 
  -> std::vector<nat>
{
  auto result = std::vector<nat>(_numProc,0); 
  for(auto &p : _proc2assignment)
    {
      auto ass = std::get<1>(p); 
      ++result[ass.procNum]; 
    }
  return result; 
}



std::pair< std::vector<int>,std::vector<int> > 
PartitionAssignment::getCountsAndDispls( size_t numInComm) const 
{
  auto countsPerProc = std::vector<int>{}; 
  for(nat i = 0; i <  numInComm; ++i)
    countsPerProc.push_back(int(getTotalWidthPerProc(i))); // TODO inefficient
  auto displPerProc = std::vector<int>{}; 
  displPerProc.push_back(0); 
  for(nat i = 1; i  < numInComm ; ++i)
    displPerProc.push_back(int(displPerProc[i-1] + countsPerProc[i-1]));

  return std::make_pair(countsPerProc,displPerProc);
} 

