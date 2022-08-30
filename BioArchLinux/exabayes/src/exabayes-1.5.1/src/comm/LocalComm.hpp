#ifndef _LOCAL_COMM_HPP
#define _LOCAL_COMM_HPP

#include <unordered_map>
#include "threadDefs.hpp"
#include <iostream>
#include <numeric>
#include <functional>

#include "MessageQueue.hpp"
#include "MessageQueueSingle.hpp"

#define DATA_COMBINE_FUN std::function<void(std::vector<T>& acc,  typename std::vector<T>::const_iterator,  typename std::vector<T>::const_iterator)> 
// 1st argument: accumulator
// 2nd argument: start of donator
// 3rd argument: end of donator 


class LocalComm
{
  typedef LocalComm SELF; 

public: 
  /** 
      @param tid2rank contains absolute ranks of all threads 
   */ 
  LocalComm(std::unordered_map<tid_t,int> tid2rank); 
  LocalComm(const LocalComm& rhs); 
  LocalComm(LocalComm &&rhs) ; 
  LocalComm& operator=(LocalComm rhs ); 

  friend void swap(LocalComm& lhs, LocalComm& rhs ); 

  friend std::ostream& operator<<(std::ostream& out, const LocalComm& rhs); 

  std::unordered_map<tid_t,int> getTid2Ranking  () const {return _tid2LocCommIdx; }
  void setColors(std::vector<int> colors) { _colors = colors; }
  void setRanks(std::vector<int> ranks) {_ranks = ranks; }
  
  #include "CommCore.hpp"

  int getIdx() const {return _tid2LocCommIdx.at(MY_TID); }
  size_t getNumThreads() const ; 

  int getIdx(int col, int rank) const ; 
  int getColor() const {return _colors.at(_tid2LocCommIdx.at(MY_TID)); }

  template<typename T>
  void postAsyncMessage(const std::vector<T> &message, int tag, int runBatch); 
  template<typename T>
  std::tuple<bool,std::vector<T> > readAsyncMessage(int tag, int runBatch); 
  
  void initializeAsyncQueue(size_t size, size_t numSlots);

private: 
  template<typename T>
  void produceWrapper(std::vector<T> msg, int idx, const std::vector<int> &who) ; 

  int mapRealRank2Corrected(int rank, int root) ; 
  int mapCorrectedRank2Real(int rank, int root); 
  /** 
      @brief communicates data from tips to root in a deterministic way 
   */ 
  template<typename T>
  std::vector<T> commTreeUp(std::vector<T> data, int root, DATA_COMBINE_FUN fun); 
  /** 
      @brief communicates data from tips to root in an asynchronous way 
   */ 
  template<typename T>
  std::vector<T> commTreeUpAsync(std::vector<T> data, int root, DATA_COMBINE_FUN fun); 
  /** 
      @brief communicates data from root to tips in an asynchronous way  
   */ 
  template<typename T>
  std::vector<T> commTreeDownAsync(std::vector<T> data, int root); 

private: 			// ATTRIBUTES
  std::unordered_map<tid_t,int> _tid2LocCommIdx;

  std::vector<int> _colors; 
  std::vector<int> _ranks; 
  size_t _size; 

  // for each color (here runid), we have an array that is indexed by
  // a tag that is composed of two chain ids (cantor pair)
  std::vector< std::vector<MessageQueue> > _mgsPerTag; 
  std::vector<std::vector<MessageQueueSingle>> _newMessages;
}; 


#include "LocalComm.tpp"

#endif
