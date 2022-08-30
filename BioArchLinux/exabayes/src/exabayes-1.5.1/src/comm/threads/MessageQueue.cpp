#include "MessageQueue.hpp"
#include "GlobalVariables.hpp" 
#include <algorithm>
#include <numeric>

using std::move; 


MessageQueue::MessageQueue(int numThreads)
  : _first( new Node(std::vector<uint8_t>(), std::vector<int>() ))
  , _divider(_first)
  , _last{_first}
  , _consumerLock(false)
  , _numThreads(numThreads)
{
}



MessageQueue& MessageQueue::operator=(MessageQueue rhs) 
{
  swap(rhs, *this); 
  return *this; 
}


void swap(MessageQueue& lhs, MessageQueue& rhs)
{
  using std::swap; 

  swap(lhs._first, rhs._first); 
  swap(lhs._numThreads, rhs._numThreads); 

  rhs._divider = lhs._divider.exchange(rhs._divider); 
  rhs._last = lhs._last.exchange(rhs._last) ; 
  rhs._consumerLock = lhs._consumerLock.exchange(rhs._consumerLock); 
} 



MessageQueue::MessageQueue( MessageQueue&& rhs)  
  : _first(std::move(rhs._first))
  , _divider(rhs._divider.load())
  , _last(rhs._last.load())
  , _consumerLock(false)
  , _numThreads(std::move(rhs._numThreads))
{
  rhs._divider = nullptr; 
  rhs._last = nullptr; 
  rhs._first = nullptr;
}


MessageQueue::MessageQueue(const MessageQueue& rhs)  
  : _first(new Node(std::vector<uint8_t>(), std::vector<int>()))
  , _divider(_first)
  , _last(_first)
  , _consumerLock(false)
  , _numThreads(rhs._numThreads)
{
} 


MessageQueue::~MessageQueue()
{
  while(_first != nullptr)
    {
      auto tmp = _first; 
      _first = tmp->_next; 
      delete tmp; 
    }
}

 
MessageQueue::Node::Node(std::vector<byte> msg, std::vector<int> whoReads )
  : _message(msg)
  , _numConsume(std::accumulate(begin(whoReads), end(whoReads),0))
  , _next(nullptr)
  , _whoReads(whoReads)
{
}


auto  MessageQueue::Node::getConsumed(int threadId)
  -> std::tuple<int, std::vector<byte> >
{
  // assert(_hasRead.at(threadId) == 0);
  assert(_whoReads.at(threadId) > 0 ); 
  --_whoReads[threadId]; 
    // _hasRead.at(threadId) = 1;
  auto result = --_numConsume;
  assert(result >= 0 ); 
  return std::make_tuple(result, _message);
}
