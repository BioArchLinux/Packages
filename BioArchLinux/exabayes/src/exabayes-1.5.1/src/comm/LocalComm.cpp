#include "LocalComm.hpp"

#include <cassert>
#include <thread>


LocalComm::LocalComm(std::unordered_map<tid_t,int> tid2rank)
  : _tid2LocCommIdx(tid2rank)
  , _colors(_tid2LocCommIdx.size(),0u)
  , _ranks(_tid2LocCommIdx.size(), 0u)
  , _size(_tid2LocCommIdx.size())
  , _mgsPerTag{}
  , _newMessages(tid2rank.size(), std::vector<MessageQueueSingle>(size_t(_size)))
{
  nat ctr = 0; 
  for(auto &r : _ranks)
    r = ctr++; 
}

LocalComm::LocalComm(const LocalComm& rhs) 
  : _tid2LocCommIdx(rhs._tid2LocCommIdx)
  , _colors(rhs._colors)
  , _ranks(rhs._ranks)
  , _size(rhs._size)
  , _mgsPerTag(rhs._mgsPerTag)
  , _newMessages(rhs._newMessages)
{
}
  


LocalComm::LocalComm(LocalComm &&rhs) 
  : _tid2LocCommIdx(std::move(rhs._tid2LocCommIdx))
  , _colors(std::move(rhs._colors))
  , _ranks(std::move(rhs._ranks))
  , _size(std::move(rhs._size))
  , _mgsPerTag(std::move(rhs._mgsPerTag))
  , _newMessages(std::move(rhs._newMessages))
{
}

LocalComm& LocalComm::operator=(LocalComm rhs )
{
  swap(*this, rhs);
  return *this;
}


std::ostream& operator<<(std::ostream& out, const LocalComm& rhs)
{
  out << "{" << rhs.getColor() << "," << rhs.getRank() << "}"; 
  return out; 
}


size_t LocalComm::getNumThreads() const 
{
  return _ranks.size();
}

size_t LocalComm::size() const 
{
  return _size; 
}


int LocalComm::getRank() const
{ 
  return _ranks.at(_tid2LocCommIdx.at(MY_TID));  
} 


bool LocalComm::isValid() const  
{
  // TODO evil, but reasonable 
  bool result = true; 
  for(nat i = 0; i < _colors.size() ; ++i)
    for(nat j = i + 1 ; j < _colors.size() ; ++j)
      result &= (_ranks[i] != _ranks[j] ||  _colors[i] != _colors[j] ) ; 
  
  auto color2num =  std::unordered_map<int,int>{};
  for(nat i =0; i < _colors.size() ; ++i)
    ++color2num[_colors[i]] ; 
  int aNum = begin(color2num)->second; 
  for(auto &elem : color2num)
    assert(aNum == elem.second); 
  assert(int(_size) == aNum); 

  return result; 
}


LocalComm LocalComm::split(const std::vector<int> &color, const std::vector<int> &rank)  const
{
  auto result = *this; 
  
  auto uniqCols = std::unordered_set<int>{}; 
  uniqCols.insert(begin(color), end(color)); 
  // auto minCol = *(std::min_element(begin(uniqCols), end(uniqCols))); 
  auto tColors = color; 

  if(rank[0] > 0 )
    {
      auto tRanks = rank; 
      for(auto &r : tRanks)
	r -= rank[0]; 
      result.setRanks(tRanks); 
      result.setColors(tColors); 
    }
  else 
    {
      result.setRanks(rank);
      result.setColors(tColors); 
    }
  
  auto col2occ = std::unordered_map<int,int>{}; 
  for(auto c : result._colors)
    ++col2occ[c]; 
  result._size = begin(col2occ)->second; 
 
  result.isValid(); 
 
  return result; 
}


void swap(LocalComm& lhs, LocalComm& rhs )
{
  using std::swap; 
  swap(lhs._tid2LocCommIdx, rhs._tid2LocCommIdx); 
  swap(lhs._colors, rhs._colors); 
  swap(lhs._ranks, rhs._ranks); 
  swap(lhs._size, rhs._size); 
  swap(lhs._mgsPerTag, rhs._mgsPerTag); 
  swap(lhs._newMessages, rhs._newMessages); 
} 


int LocalComm::getIdx(int col, int rank) const 
{
  int result = -1; 
  for(auto i = 0u ; i < getNumThreads(); ++i)
    {
      if(_colors[i] == col && _ranks[i] == rank)
	return int(i); 
    }
  assert(0); 
  return result; 
}



void LocalComm::waitAtBarrier()
{
  // TODO efficiency. But it is not used currently anyway...
  auto arr = std::vector<uint8_t>{1};
  arr = allReduce<uint8_t>(arr);
  assert(arr.size() == 1 && arr[0] == size()); 
}



static void dummyFun()
{
  
}

bool LocalComm::haveThreadSupport() const 
{
  bool isAvailable = true; 
  try
    { 
      auto&& t = std::thread(dummyFun); 
      t.join();
    }
  catch(std::system_error &anExcept)
    {
      isAvailable = false; 
    }
  
  return isAvailable; 
}



void LocalComm::initializeAsyncQueue(size_t siz, size_t numSlots)
{
  auto maxCol = *(std::max_element(begin(_colors), end(_colors))); 
  _mgsPerTag.resize(maxCol+1);
  for(int j = 0; j < maxCol+1; ++j)
    {
      for(nat i = 0; i < numSlots; ++i)
	_mgsPerTag.at(j).emplace_back(siz);
    }
}


int LocalComm::mapRealRank2Corrected(int rank, int root) 
{
  // auto result = rank - root ; 
  // if( result < 0  )
  //   return size() - result; 
  // else 
  //   return result; 

  assert(root == 0); 
  return rank; 
}

int LocalComm::mapCorrectedRank2Real(int rank, int root)
{

  // TODO this and the other function could be used to map the ranks
  // in case we use a root != 0.

  // since this is currently not needed and I do not have more time to
  // fiddle around, these functions simply return the original rank.
  
  assert(root == 0); 
  return rank; 

  // auto result = rank + root; 
  // if(result >= size())
  //   return result - size(); 
  // else 
  //   return result; 
}




void LocalComm::abort(int code, bool waitForAll)
{
  if(_masterThread != std::this_thread::get_id())
    {
       while (not _threadsDie)
	; 

       pthread_exit(NULL);

    }
  else  
    {
      _threadsDie = true; 
    }
}

